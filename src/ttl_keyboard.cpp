/**
 * @file ttl_keyboard.cpp
 * @brief TTL键盘类实现 - 严格按照debug_uart.cpp的数据处理方式
 */

#include "ttl_keyboard.hpp"
#include <cstdio>
#include <cstring>
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

namespace usb2ttl {

TTLKeyboard::TTLKeyboard() 
    : uart_instance_(nullptr)
    , key_callback_(nullptr)
    , keyboard_connected_(false)
    , last_key_("")
    , last_key_time_(0)
    , last_activity_time_(0)
    , buffer_pos_(0) {
    
    // 清空接收缓冲区
    std::memset(rx_buffer_, 0, BUFFER_SIZE);
    
    // 初始化按键映射
    init_key_map();
}

TTLKeyboard::~TTLKeyboard() {
    // 清理资源
    if (uart_instance_) {
        uart_deinit(uart_instance_);
    }
}

bool TTLKeyboard::initialize(uart_inst_t* uart_instance, 
                           std::uint32_t baud_rate,
                           std::uint8_t tx_pin, 
                           std::uint8_t rx_pin) {
    
    printf("Initializing TTL keyboard on UART%d (TX:%d, RX:%d, Baud:%lu)...\n", 
           uart_get_index(uart_instance), tx_pin, rx_pin, baud_rate);
    
    uart_instance_ = uart_instance;
    
    // 初始化UART
    uint actual_baud = uart_init(uart_instance_, baud_rate);
    printf("UART actual baud rate: %u\n", actual_baud);
    
    // 设置GPIO引脚功能
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    printf("GPIO %d set to UART TX function\n", tx_pin);
    printf("GPIO %d set to UART RX function\n", rx_pin);
    
    // 配置UART参数：8位数据，1位停止位，无校验
    uart_set_hw_flow(uart_instance_, false, false);
    uart_set_format(uart_instance_, 8, 1, UART_PARITY_NONE);
    
    // 启用UART FIFO
    uart_set_fifo_enabled(uart_instance_, true);
    
    printf("TTL keyboard initialized successfully\n");
    printf("Waiting for keyboard input on UART%d RX (GPIO %d)...\n", 
           uart_get_index(uart_instance), rx_pin);
    
    return true;
}

void TTLKeyboard::set_key_callback(KeyboardCallback callback) {
    key_callback_ = std::move(callback);
}

void TTLKeyboard::process_events() {
    if (!uart_instance_) {
        return;
    }
    
    // 处理接收到的数据
    process_received_data();
    
    // 更新连接状态
    update_connection_status();
}

bool TTLKeyboard::is_keyboard_connected() const {
    return keyboard_connected_;
}

std::string TTLKeyboard::get_last_key() const {
    return last_key_;
}

void TTLKeyboard::init_key_map() {
    // 基于MicroPython代码的按键映射
    key_map_[0x08] = "Backspace";  // BS
    key_map_[0x09] = "Tab";        // HT
    key_map_[0x0A] = "Enter";      // LF
    key_map_[0x0D] = "Enter";      // CR
    key_map_[0x1B] = "ESC";        // ESC
    key_map_[0x20] = "space";      // Space
    key_map_[0x7F] = "Delete";     // DEL
}

void TTLKeyboard::process_received_data() {
    if (!uart_is_readable(uart_instance_)) {
        return;
    }
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 读取所有可用数据 (严格遵循debug_uart.cpp的方式)
    char temp_buffer[64];
    size_t bytes_read = 0;
    
    while (uart_is_readable(uart_instance_) && bytes_read < sizeof(temp_buffer)) {
        temp_buffer[bytes_read] = uart_getc(uart_instance_);
        bytes_read++;
    }
    
    if (bytes_read > 0) {
        printf("\n--- TTL键盘接收到数据 (时间: %lu ms) ---\n", current_time);
        printf("字节数: %zu\n", bytes_read);
        
        // 十六进制数据显示 (完全按照debug_uart.cpp)
        printf("十六进制数据 (%zu字节): ", bytes_read);
        for (size_t i = 0; i < bytes_read; ++i) {
            printf("%02X ", (unsigned char)temp_buffer[i]);
        }
        printf("\n");
        
        // ASCII表示 (完全按照debug_uart.cpp)
        printf("ASCII表示: ");
        for (size_t i = 0; i < bytes_read; ++i) {
            char ch = temp_buffer[i];
            if (ch >= 32 && ch <= 126) {
                printf("%c", ch);
            } else {
                printf(".");
            }
        }
        printf("\n");
        
        // 特殊字符分析 (完全按照debug_uart.cpp)
        printf("特殊字符分析:\n");
        for (size_t i = 0; i < bytes_read; ++i) {
            unsigned char ch = (unsigned char)temp_buffer[i];
            switch (ch) {
                case 0x08: printf("  [%zu] Backspace\n", i); break;
                case 0x09: printf("  [%zu] Tab\n", i); break;
                case 0x0A: printf("  [%zu] Line Feed\n", i); break;
                case 0x0D: printf("  [%zu] Carriage Return\n", i); break;
                case 0x1B: printf("  [%zu] Escape\n", i); break;
                case 0x20: printf("  [%zu] Space\n", i); break;
                case 0x7F: printf("  [%zu] Delete\n", i); break;
                default:
                    if (ch < 32) {
                        printf("  [%zu] 控制字符 (0x%02X)\n", i, ch);
                    } else if (ch >= 32 && ch <= 126) {
                        printf("  [%zu] 可打印字符 '%c'\n", i, ch);
                    } else {
                        printf("  [%zu] 扩展字符 (0x%02X)\n", i, ch);
                    }
                    break;
            }
        }
        
        // 过滤噪声数据 - 只有有效数据才进行按键处理和更新活动时间
        bool has_valid_data = false;
        for (size_t i = 0; i < bytes_read; ++i) {
            unsigned char ch = (unsigned char)temp_buffer[i];
            // 过滤掉0xFF噪声和其他明显的噪声数据
            if (ch != 0xFF && ch != 0x00) {
                has_valid_data = true;
                break;
            }
        }
        
        if (has_valid_data) {
            // 只有在有有效数据时才更新活动时间
            last_activity_time_ = current_time;
            printf("检测到有效数据，进行按键解析\n");
            
            // 智能重复按键过滤 - 先收集所有唯一按键，然后只处理一次
            std::map<std::string, bool> unique_keys;
            
            // 第一遍：收集所有唯一按键
            for (size_t i = 0; i < bytes_read; ++i) {
                unsigned char ch = (unsigned char)temp_buffer[i];
                
                // 跳过噪声数据
                if (ch == 0xFF || ch == 0x00) {
                    continue;
                }
                
                std::string key = "";
                
                // 查找按键映射
                auto it = key_map_.find(ch);
                if (it != key_map_.end()) {
                    key = it->second;
                } else if (ch >= 32 && ch <= 126) {
                    // 可打印ASCII字符
                    key = std::string(1, (char)ch);
                }
                
                if (!key.empty()) {
                    unique_keys[key] = true;
                }
            }
            
            // 第二遍：处理唯一按键，应用时间阈值过滤
            for (const auto& key_pair : unique_keys) {
                const std::string& key = key_pair.first;
                
                // 防重复按键检测 - 检查是否与上次按键相同且在时间阈值内
                bool is_duplicate = (key == last_key_ && 
                                   (current_time - last_key_time_) <= DUPLICATE_KEY_THRESHOLD);
                
                if (!is_duplicate) {
                    last_key_ = key;
                    last_key_time_ = current_time;
                    
                    // 调用回调函数
                    if (key_callback_) {
                        key_callback_(key);
                    }
                    
                    printf("解析的按键: %s\n", key.c_str());
                } else {
                    printf("忽略重复按键: %s (时间间隔: %lu ms)\n", 
                           key.c_str(), current_time - last_key_time_);
                }
            }
        } else {
            printf("检测到噪声数据，忽略处理\n");
        }
        
        printf("--- TTL键盘数据处理完成 ---\n\n");
    }
}

std::string TTLKeyboard::parse_key_sequence(const char* data, std::size_t length) {
    // 简化版本，暂时不处理复杂序列
    return "";
}

std::string TTLKeyboard::process_ascii_char(char ch) {
    // 简化版本
    return "";
}

std::string TTLKeyboard::process_escape_sequence(const char* seq, std::size_t length) {
    // 简化版本
    return "";
}

bool TTLKeyboard::is_printable_ascii(char ch) {
    return ch >= 32 && ch <= 126;
}

void TTLKeyboard::update_connection_status() {
    std::uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 修改连接判断逻辑 - 只有在接收到有效数据时才认为连接
    if ((current_time - last_activity_time_) < CONNECTION_TIMEOUT && last_activity_time_ > 0) {
        if (!keyboard_connected_) {
            keyboard_connected_ = true;
            printf("TTL keyboard connected\n");
        }
    } else {
        if (keyboard_connected_) {
            keyboard_connected_ = false;
            printf("TTL keyboard disconnected (timeout)\n");
        }
    }
}

} // namespace usb2ttl