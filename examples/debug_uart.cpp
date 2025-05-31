/**
 * @file debug_uart.cpp
 * @brief UART调试程序 - 专门用于诊断TTL键盘通信问题
 */

#include <cstdio>
#include <cstring>
#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// 硬件配置
uart_inst_t* const UART_INSTANCE = uart1;
constexpr uint8_t PIN_TX = 8;
constexpr uint8_t PIN_RX = 9;
constexpr uint32_t UART_BAUD = 115200;

// 调试缓冲区
constexpr size_t DEBUG_BUFFER_SIZE = 1024;
char debug_buffer[DEBUG_BUFFER_SIZE];
size_t debug_pos = 0;

// 统计信息
uint32_t total_bytes_received = 0;
uint32_t last_activity_time = 0;
uint32_t connection_checks = 0;

void init_uart_debug() {
    printf("\n=== UART调试程序启动 ===\n");
    printf("配置: UART%d, TX=GPIO%d, RX=GPIO%d, 波特率=%lu\n", 
           uart_get_index(UART_INSTANCE), PIN_TX, PIN_RX, UART_BAUD);
    
    // 初始化UART
    uint actual_baud = uart_init(UART_INSTANCE, UART_BAUD);
    printf("实际波特率: %u\n", actual_baud);
    
    // 设置GPIO功能
    gpio_set_function(PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_RX, GPIO_FUNC_UART);
    
    // 配置UART参数
    uart_set_hw_flow(UART_INSTANCE, false, false);
    uart_set_format(UART_INSTANCE, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_INSTANCE, true);
    
    printf("UART初始化完成\n");
    printf("等待键盘输入...\n");
    printf("提示: 请按键盘上的任意键进行测试\n\n");
}

void print_hex_dump(const char* data, size_t length) {
    printf("十六进制数据 (%zu字节): ", length);
    for (size_t i = 0; i < length; ++i) {
        printf("%02X ", (unsigned char)data[i]);
    }
    printf("\n");
    
    printf("ASCII表示: ");
    for (size_t i = 0; i < length; ++i) {
        char ch = data[i];
        if (ch >= 32 && ch <= 126) {
            printf("%c", ch);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

void process_uart_data() {
    if (!uart_is_readable(UART_INSTANCE)) {
        return;
    }
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    last_activity_time = current_time;
    
    // 读取所有可用数据
    char temp_buffer[64];
    size_t bytes_read = 0;
    
    while (uart_is_readable(UART_INSTANCE) && bytes_read < sizeof(temp_buffer)) {
        temp_buffer[bytes_read] = uart_getc(UART_INSTANCE);
        bytes_read++;
        total_bytes_received++;
    }
    
    if (bytes_read > 0) {
        printf("\n--- 接收到数据 (时间: %lu ms) ---\n", current_time);
        printf("字节数: %zu, 总计: %lu\n", bytes_read, total_bytes_received);
        
        print_hex_dump(temp_buffer, bytes_read);
        
        // 分析特殊字符
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
        printf("--- 数据处理完成 ---\n\n");
    }
}

void print_status() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    connection_checks++;
    
    printf("=== 状态报告 #%lu (运行时间: %lu秒) ===\n", 
           connection_checks, current_time / 1000);
    printf("总接收字节数: %lu\n", total_bytes_received);
    printf("最后活动时间: %lu ms前\n", 
           last_activity_time > 0 ? current_time - last_activity_time : 0);
    
    // UART状态检查
    printf("UART状态:\n");
    printf("  - 可读: %s\n", uart_is_readable(UART_INSTANCE) ? "是" : "否");
    printf("  - 可写: %s\n", uart_is_writable(UART_INSTANCE) ? "是" : "否");
    
    // 连接状态判断
    bool connected = (current_time - last_activity_time) < 5000 && total_bytes_received > 0;
    printf("连接状态: %s\n", connected ? "已连接" : "未连接");
    
    printf("=== 状态报告结束 ===\n\n");
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // 等待串口稳定
    
    init_uart_debug();
    
    while (true) {
        // 处理接收数据
        process_uart_data();
               
        sleep_ms(10);
    }
    
    return 0;
} 