#pragma once

#include <string>
#include <functional>
#include <map>
#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/uart.h"

namespace usb2ttl {

// 键盘事件回调函数类型
using KeyboardCallback = std::function<void(const std::string&)>;

/**
 * @brief TTL串口键盘输入处理类
 * @details 通过UART1串口接收键盘输入，处理串口数据并转换为键盘事件
 * 
 * 工作原理：
 * - 键盘连接到USB2TTL模块
 * - USB2TTL模块将键盘输入转换为串口数据
 * - Pico通过UART1 (GPIO 8/9) 接收串口数据
 * - 完全基于UART通信，与USB协议无关
 * 
 * 硬件配置：
 * - UART1: GPIO 8 (TX), GPIO 9 (RX)
 * - 波特率: 9600
 * - 数据位: 8, 停止位: 1, 无校验
 */
class TTLKeyboard {
public:
    /**
     * @brief 构造函数
     */
    TTLKeyboard();
    
    /**
     * @brief 析构函数
     */
    ~TTLKeyboard();
    
    // 禁用拷贝构造和赋值操作
    TTLKeyboard(const TTLKeyboard&) = delete;
    TTLKeyboard& operator=(const TTLKeyboard&) = delete;
    
    // 允许移动构造和赋值
    TTLKeyboard(TTLKeyboard&&) = default;
    TTLKeyboard& operator=(TTLKeyboard&&) = default;
    
    /**
     * @brief 初始化TTL串口键盘支持
     * @param uart_instance UART实例 (默认uart1)
     * @param baud_rate 波特率 (默认115200)
     * @param tx_pin TX引脚 (默认8)
     * @param rx_pin RX引脚 (默认9)
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize(uart_inst_t* uart_instance = uart1, 
                   std::uint32_t baud_rate = 115200,
                   std::uint8_t tx_pin = 8, 
                   std::uint8_t rx_pin = 9);
    
    /**
     * @brief 设置按键回调函数
     * @param callback 按键事件回调函数
     */
    void set_key_callback(KeyboardCallback callback);
    
    /**
     * @brief 处理串口事件 (需要在主循环中调用)
     */
    void process_events();
    
    /**
     * @brief 检查是否有键盘连接
     * @return true 键盘已连接，false 键盘未连接
     */
    bool is_keyboard_connected() const;
    
    /**
     * @brief 获取最后按下的键
     * @return 最后按下的键字符串
     */
    std::string get_last_key() const;

private:
    // 私有成员变量
    uart_inst_t* uart_instance_;
    KeyboardCallback key_callback_;
    bool keyboard_connected_;
    std::string last_key_;
    std::uint32_t last_key_time_;
    std::uint32_t last_activity_time_;
    
    // 串口接收缓冲区
    static constexpr std::size_t BUFFER_SIZE = 256;
    char rx_buffer_[BUFFER_SIZE];
    std::size_t buffer_pos_;
    
    // 按键映射表
    std::map<std::uint8_t, std::string> key_map_;
    
    // 私有方法
    void init_key_map();
    void process_received_data();
    std::string parse_key_sequence(const char* data, std::size_t length);
    std::string process_ascii_char(char ch);
    std::string process_escape_sequence(const char* seq, std::size_t length);
    bool is_printable_ascii(char ch);
    void update_connection_status();
    
    // 连接检测的时间阈值 (毫秒)
    static constexpr std::uint32_t CONNECTION_TIMEOUT = 5000;
    
    // 防重复按键的时间阈值 (毫秒)
    // 针对USB2TTL模块重复发送问题优化：
    // - USB2TTL模块经常在100-150ms内重复发送相同按键
    // - 真实的快速打字间隔通常>150ms (即使是专业打字员)
    // - 机械键盘抖动通常<50ms
    // 设置为200ms可以：
    // 1. 有效过滤USB2TTL模块的重复发送 (100-200ms)
    // 2. 仍然支持快速打字 (>200ms间隔)
    // 3. 过滤机械键盘的按键抖动 (<50ms)
    // 4. 避免误过滤真实的快速连击
    static constexpr std::uint32_t DUPLICATE_KEY_THRESHOLD = 200;
};

} // namespace usb2ttl 