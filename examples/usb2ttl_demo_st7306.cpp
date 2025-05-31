/**
 * @file usb2ttl_demo_st7306.cpp
 * @brief TTL键盘演示程序 - 集成ST7306反射式LCD显示和TTL键盘输入
 * @author usb2ttl_pico项目
 * @version 2.1.0
 * 
 * 功能说明：
 * - 使用ST7306 4.2英寸反射式LCD显示屏作为输出设备 (300x400像素，4级灰度)
 * - 通过UART1串口接收键盘输入 (GPIO 8 TX, GPIO 9 RX, 115200波特率)
 * - 键盘通过USB2TTL模块转换为串口信号，与USB协议无关
 * - 提供键盘命令界面
 * - 支持文本编辑功能
 * - 按回车键进入文本编辑模式
 * 
 * 硬件连接：
 * - 键盘 → USB2TTL模块 → Pico UART1 (GPIO 8/9)
 * - ST7306显示屏 → Pico SPI0 (GPIO 17-20)
 * - 完全基于串口通信，无USB HID协议
 */

#include <cstdio>
#include <cstring>
#include <memory>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

// 项目头文件
#include "ttl_keyboard.hpp"
#include "text_editor.hpp"
#include "display_driver.hpp"

// ST7306驱动头文件
#include "st7306_driver.hpp"
#include "pico_display_gfx.hpp"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"

using namespace st7306;
using namespace usb2ttl;  // 使用项目命名空间

// 硬件配置
namespace HardwareConfig {
    // ST7306 SPI配置
    constexpr std::uint8_t PIN_DC   = 20;
    constexpr std::uint8_t PIN_RST  = 15;
    constexpr std::uint8_t PIN_CS   = 17;
    constexpr std::uint8_t PIN_SCLK = 18;
    constexpr std::uint8_t PIN_SDIN = 19;
    
    // TTL键盘UART配置
    uart_inst_t* const UART_INSTANCE = uart1;
    constexpr std::uint8_t PIN_TX = 8;
    constexpr std::uint8_t PIN_RX = 9;
    constexpr std::uint32_t UART_BAUD = 115200;
    
    // 状态LED
    constexpr std::uint8_t PIN_LED = PICO_DEFAULT_LED_PIN;
}

// 应用程序状态
enum class AppState {
    COMMAND_MODE,    // 键盘命令界面
    EDIT_MODE       // 文本编辑模式
};

// ST7306显示驱动适配器类
class ST7306DisplayAdapter : public DisplayDriver {
private:
    std::unique_ptr<ST7306Driver> st7306_driver_;
    std::unique_ptr<pico_gfx::PicoDisplayGFX<ST7306Driver>> gfx_;
    
    // 颜色映射：将RGB666颜色映射到4级灰度
    uint8_t rgb666_to_gray4(std::uint32_t rgb666_color) {
        // 简单的亮度映射
        if (rgb666_color == 0x000000) return ST7306Driver::COLOR_BLACK;  // 黑色
        else if (rgb666_color == 0x3F3F3F) return ST7306Driver::COLOR_WHITE; // 白色
        else if ((rgb666_color & 0x3F3F3F) < 0x151515) return ST7306Driver::COLOR_GRAY2; // 深灰
        else return ST7306Driver::COLOR_GRAY1; // 浅灰
    }
    
    bool color_to_bool(std::uint32_t color) {
        // 将颜色转换为布尔值，用于单色绘制
        return (color != 0x000000); // 非黑色为true
    }
    
public:
    ST7306DisplayAdapter() {
        st7306_driver_ = std::make_unique<ST7306Driver>(
            HardwareConfig::PIN_DC,
            HardwareConfig::PIN_RST,
            HardwareConfig::PIN_CS,
            HardwareConfig::PIN_SCLK,
            HardwareConfig::PIN_SDIN
        );
        
        gfx_ = std::make_unique<pico_gfx::PicoDisplayGFX<ST7306Driver>>(
            *st7306_driver_, ST7306Driver::LCD_WIDTH, ST7306Driver::LCD_HEIGHT
        );
        
        width_ = ST7306Driver::LCD_WIDTH;
        height_ = ST7306Driver::LCD_HEIGHT;
        font_width_ = font::FONT_WIDTH;
        font_height_ = font::FONT_HEIGHT;
        text_offset_x_ = 5;
        text_offset_y_ = 5;
    }
    
    bool initialize() override {
        printf("Initializing ST7306 display...\n");
        
        st7306_driver_->initialize();
        st7306_driver_->setRotation(0); // 默认方向
        st7306_driver_->clearDisplay();
        st7306_driver_->display();
        
        printf("ST7306 display initialized successfully!\n");
        return true;
    }
    
    void clear_screen(std::uint32_t color = 0x000000) override {
        st7306_driver_->clearDisplay();
        st7306_driver_->display();
    }
    
    void fill_rect(int x, int y, int width, int height, std::uint32_t color) override {
        bool fill_color = color_to_bool(color);
        gfx_->drawFilledRectangle(x, y, width, height, fill_color);
        st7306_driver_->display();
    }
    
    void draw_text(const std::string& text, int x, int y, 
                   std::uint32_t fg_color = 0x3F3F3F, 
                   std::uint32_t bg_color = 0x000000) override {
        bool text_color = color_to_bool(fg_color);
        st7306_driver_->drawString(x, y, text.c_str(), text_color);
        st7306_driver_->display();
    }
    
    void draw_char(char ch, int x, int y, std::uint32_t fg_color = 0x3F3F3F, std::uint32_t bg_color = 0x000000) {
        bool text_color = color_to_bool(fg_color);
        st7306_driver_->drawChar(x, y, ch, text_color);
        st7306_driver_->display();
    }
    
    void draw_rect(int x, int y, int width, int height, std::uint32_t color) {
        bool line_color = color_to_bool(color);
        gfx_->drawRectangle(x, y, width, height, line_color);
        st7306_driver_->display();
    }
    
    void set_backlight(float brightness) override {
        // ST7306是反射式LCD，无背光控制
        // 可以通过高/低功耗模式来调节显示效果
        if (brightness > 0.5f) {
            st7306_driver_->highPowerMode();
        } else {
            st7306_driver_->lowPowerMode();
        }
    }
    
    void refresh() override {
        st7306_driver_->display();
    }
    
    int get_width() const override { return ST7306Driver::LCD_WIDTH; }
    int get_height() const override { return ST7306Driver::LCD_HEIGHT; }
    int get_font_width() const override { return font_width_; }
    int get_font_height() const override { return font_height_; }
    
    pico_gfx::PicoDisplayGFX<ST7306Driver>* getGFX() { return gfx_.get(); }
    ST7306Driver* getDriver() { return st7306_driver_.get(); }
    
private:
    void show_initialization_screen() {
        clear_screen(0x000000);
        draw_text("TTL Keyboard System", 60, 180, 0x3F3F3F, 0x000000);
        draw_text("Initializing...", 100, 220, 0x3F3F3F, 0x000000);
        sleep_ms(1000);
    }
};

// 全局变量
std::shared_ptr<ST7306DisplayAdapter> g_display;
std::unique_ptr<TTLKeyboard> g_keyboard;
std::unique_ptr<TextEditor> g_text_editor;
AppState g_app_state = AppState::COMMAND_MODE;
bool g_keyboard_connected = false;

// 函数声明
void init_hardware();
void init_display();
void init_keyboard();
void init_text_editor();
void show_command_screen();
void show_edit_mode();
void handle_keyboard_input(const std::string& key);
void handle_command_mode_input(const std::string& key);
void handle_edit_mode_input(const std::string& key);
void update_status_display();

/**
 * @brief 主函数
 */
int main() {
    // 初始化标准库
    stdio_init_all();
    
    printf("\n=== TTL Keyboard Demo (ST7306) Starting ===\n");
    printf("Version: 2.1.0\n");
    printf("Hardware: Raspberry Pi Pico + ST7306 + TTL Keyboard via UART1\n");
    printf("UART Config: GPIO 8 (TX), GPIO 9 (RX), 115200 baud\n");
    printf("Display: ST7306 300x400 4-level grayscale reflective LCD\n");
    printf("Note: No USB HID protocol - pure UART communication\n\n");
    
    // 初始化硬件组件
    init_hardware();
    init_display();
    init_keyboard();
    init_text_editor();
    
    // 显示命令界面
    show_command_screen();
    
    printf("System ready! Waiting for TTL keyboard input...\n");
    
    // 主循环
    std::uint32_t last_status_update = 0;
    
    while (true) {
        std::uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // 处理键盘事件
        if (g_keyboard) {
            g_keyboard->process_events();
        }
        
        // 更新状态显示 (每秒一次)
        if (now - last_status_update >= 1000) {
            update_status_display();
            last_status_update = now;
        }
        
        sleep_ms(10);
    }
    
    return 0;
}

/**
 * @brief 初始化硬件
 */
void init_hardware() {
    gpio_init(HardwareConfig::PIN_LED);
    gpio_set_dir(HardwareConfig::PIN_LED, GPIO_OUT);
    gpio_put(HardwareConfig::PIN_LED, 1);
    printf("Hardware initialized\n");
}

/**
 * @brief 初始化显示器
 */
void init_display() {
    printf("Initializing ST7306 display...\n");
    
    g_display = std::make_shared<ST7306DisplayAdapter>();
    
    if (!g_display->initialize()) {
        printf("Failed to initialize display!\n");
        while (1) {
            gpio_put(HardwareConfig::PIN_LED, 1);
            sleep_ms(100);
            gpio_put(HardwareConfig::PIN_LED, 0);
            sleep_ms(100);
        }
    }
    
    printf("Display initialized successfully\n");
}

/**
 * @brief 初始化TTL键盘
 */
void init_keyboard() {
    printf("Initializing TTL keyboard...\n");
    
    g_keyboard = std::make_unique<TTLKeyboard>();
    
    if (!g_keyboard->initialize(HardwareConfig::UART_INSTANCE, 
                               HardwareConfig::UART_BAUD,
                               HardwareConfig::PIN_TX, 
                               HardwareConfig::PIN_RX)) {
        printf("Failed to initialize TTL keyboard!\n");
        g_display->draw_text("TTL Keyboard Init Failed!", 10, 50, 0x3F3F3F, 0x000000);
        return;
    }
    
    g_keyboard->set_key_callback(handle_keyboard_input);
    printf("TTL keyboard initialized\n");
}

/**
 * @brief 初始化文本编辑器
 */
void init_text_editor() {
    printf("Initializing text editor...\n");
    
    g_text_editor = std::make_unique<TextEditor>(g_display);
    
    if (!g_text_editor->initialize()) {
        printf("Failed to initialize text editor!\n");
        g_display->draw_text("Text Editor Init Failed!", 10, 70, 0x3F3F3F, 0x000000);
        return;
    }
    
    printf("Text editor initialized\n");
}

/**
 * @brief 显示键盘命令界面
 */
void show_command_screen() {
    g_display->clear_screen(0x000000);
    
    // 绘制边框
    auto* gfx = g_display->getGFX();
    if (gfx) {
        gfx->drawRectangle(5, 5, g_display->get_width() - 10, g_display->get_height() - 10, true);
    }
    
    // 标题
    g_display->draw_text("KEYBOARD COMMANDS", 70, 15, 0x3F3F3F, 0x000000);
    g_display->draw_text("=================", 70, 35, 0x3F3F3F, 0x000000);
    
    // 命令列表
    const struct {
        const char* key;
        const char* desc;
    } commands[] = {
        {"Enter", "Enter text edit mode"},
        {"ESC", "Clear screen & reset"},
        {"F10", "Save current text"},
        {"Backspace", "Delete character"},
        {"Tab", "Insert 4 spaces"}
    };
    
    int y_pos = 70;
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        g_display->draw_text(commands[i].key, 20, y_pos, 0x3F3F3F, 0x000000);
        g_display->draw_text("-", 120, y_pos, 0x3F3F3F, 0x000000);
        g_display->draw_text(commands[i].desc, 140, y_pos, 0x3F3F3F, 0x000000);
        y_pos += 25;
    }
    
    // 状态信息区域
    if (gfx) {
        gfx->drawRectangle(10, 220, g_display->get_width() - 20, 120, true);
    }
    
    g_display->draw_text("System Status:", 20, 230, 0x3F3F3F, 0x000000);
    g_display->draw_text("TTL UART: Ready", 20, 250, 0x3F3F3F, 0x000000);
    g_display->draw_text("Display: ST7306 Reflective", 20, 270, 0x3F3F3F, 0x000000);
    g_display->draw_text("Text Editor: Ready", 20, 290, 0x3F3F3F, 0x000000);
    
    // 使用说明
    g_display->draw_text("Connect keyboard via USB2TTL", 30, 350, 0x3F3F3F, 0x000000);
    g_display->draw_text("Press ENTER to start editing", 40, 370, 0x3F3F3F, 0x000000);
    
    g_app_state = AppState::COMMAND_MODE;
}

/**
 * @brief 显示文本编辑模式
 */
void show_edit_mode() {
    g_display->clear_screen(0x000000);
    
    // 标题栏
    g_display->fill_rect(0, 0, g_display->get_width(), 25, 0x3F3F3F);
    g_display->draw_text("TEXT EDITOR - Press ESC to exit", 10, 5, 0x000000, 0x3F3F3F);
    
    // 清空编辑器内容并刷新显示
    if (g_text_editor) {
        g_text_editor->clear_screen();  // 清空编辑器内容
    }
    
    g_app_state = AppState::EDIT_MODE;
}

/**
 * @brief 处理键盘输入
 */
void handle_keyboard_input(const std::string& key) {
    printf("Key pressed: %s (Mode: %s)\n", key.c_str(), 
           g_app_state == AppState::COMMAND_MODE ? "COMMAND" : "EDIT");
    
    switch (g_app_state) {
        case AppState::COMMAND_MODE:
            handle_command_mode_input(key);
            break;
        case AppState::EDIT_MODE:
            handle_edit_mode_input(key);
            break;
    }
}

/**
 * @brief 处理命令模式输入
 */
void handle_command_mode_input(const std::string& key) {
    if (key == "Enter") {
        // 进入文本编辑模式
        show_edit_mode();
    } else if (key == "ESC") {
        // 重新显示命令界面
        show_command_screen();
    }
}

/**
 * @brief 处理编辑模式输入
 */
void handle_edit_mode_input(const std::string& key) {
    if (!g_text_editor) {
        return;
    }
    
    if (key == "ESC") {
        // 返回命令模式
        show_command_screen();
        return;
    }
    
    // 处理控制键
    if (key == "Enter" || key == "Backspace" || key == "F10" || 
        key == "Tab" || key == "space") {
        g_text_editor->handle_control_key(key);
    }
    // 处理可打印字符
    else if (key.length() == 1) {
        char ch = key[0];
        if (ch >= 32 && ch <= 126) { // 可打印ASCII字符
            g_text_editor->insert_char(ch);
        }
    }
}

/**
 * @brief 更新状态显示
 */
void update_status_display() {
    static bool last_keyboard_connected = false;
    static AppState last_app_state = AppState::COMMAND_MODE;
    static uint32_t last_uptime_sec = 0;
    static std::pair<int, int> last_cursor_pos = {-1, -1};
    static bool last_unsaved_changes = false;
    static bool first_update = true;
    
    int status_y = g_display->get_height() - 30;
    
    // 获取当前状态
    bool current_keyboard_connected = g_keyboard && g_keyboard->is_keyboard_connected();
    uint32_t current_uptime_sec = to_ms_since_boot(get_absolute_time()) / 1000;
    
    // 首次更新时清除整个状态区域
    if (first_update) {
        g_display->fill_rect(0, status_y, g_display->get_width(), 30, 0x000000);
        first_update = false;
    }
    
    // 只在连接状态变化时更新连接状态显示
    if (current_keyboard_connected != last_keyboard_connected) {
        // 清除连接状态区域
        g_display->fill_rect(10, status_y, 150, 15, 0x000000);
        
        if (current_keyboard_connected) {
            g_display->draw_text("TTL-KB: Connected", 10, status_y, 0x3F3F3F, 0x000000);
            g_keyboard_connected = true;
        } else {
            g_display->draw_text("TTL-KB: Waiting...", 10, status_y, 0x3F3F3F, 0x000000);
            g_keyboard_connected = false;
        }
        last_keyboard_connected = current_keyboard_connected;
    }
    
    // 只在模式变化时更新模式显示
    if (g_app_state != last_app_state) {
        // 清除模式显示区域
        g_display->fill_rect(200, status_y, 60, 15, 0x000000);
        
        const char* mode_text = (g_app_state == AppState::COMMAND_MODE) ? "COMMAND" : "EDIT";
        g_display->draw_text(mode_text, 200, status_y, 0x3F3F3F, 0x000000);
        last_app_state = g_app_state;
    }
    
    // 只在运行时间变化时更新时间显示
    if (current_uptime_sec != last_uptime_sec) {
        // 清除时间显示区域
        g_display->fill_rect(270, status_y, 30, 15, 0x000000);
        
        char uptime_str[32];
        snprintf(uptime_str, sizeof(uptime_str), "%lus", current_uptime_sec);
        g_display->draw_text(uptime_str, 270, status_y, 0x3F3F3F, 0x000000);
        last_uptime_sec = current_uptime_sec;
    }
    
    // 在编辑模式下更新光标位置和修改状态
    if (g_app_state == AppState::EDIT_MODE && g_text_editor) {
        auto current_cursor_pos = g_text_editor->get_cursor_position();
        bool current_unsaved_changes = g_text_editor->has_unsaved_changes();
        bool current_input_frozen = g_text_editor->is_input_frozen();
        
        // 只在光标位置变化时更新
        if (current_cursor_pos != last_cursor_pos) {
            // 清除光标位置显示区域
            g_display->fill_rect(10, status_y + 15, 120, 15, 0x000000);
            
            char cursor_str[32];
            snprintf(cursor_str, sizeof(cursor_str), "L:%d C:%d", current_cursor_pos.first + 1, current_cursor_pos.second + 1);
            g_display->draw_text(cursor_str, 10, status_y + 15, 0x3F3F3F, 0x000000);
            last_cursor_pos = current_cursor_pos;
        }
        
        // 显示编辑器状态信息
        static bool last_input_frozen = false;
        if (current_input_frozen != last_input_frozen) {
            // 清除状态信息区域
            g_display->fill_rect(150, status_y + 15, 120, 15, 0x000000);
            
            if (current_input_frozen) {
                g_display->draw_text("INPUT FROZEN!", 150, status_y + 15, 0x3F3F3F, 0x000000);
            } else {
                std::string status_info = g_text_editor->get_status_info();
                g_display->draw_text(status_info, 150, status_y + 15, 0x3F3F3F, 0x000000);
            }
            last_input_frozen = current_input_frozen;
        }
        
        // 只在修改状态变化时更新
        if (current_unsaved_changes != last_unsaved_changes) {
            // 在右侧显示修改状态
            g_display->fill_rect(270, status_y + 15, 30, 15, 0x000000);
            
            if (current_unsaved_changes) {
                g_display->draw_text("*MOD*", 270, status_y + 15, 0x3F3F3F, 0x000000);
            } else {
                g_display->draw_text("SAVED", 270, status_y + 15, 0x3F3F3F, 0x000000);
            }
            last_unsaved_changes = current_unsaved_changes;
        }
    } else {
        // 如果不在编辑模式，清除编辑模式相关显示
        if (last_cursor_pos.first != -1) {
            g_display->fill_rect(10, status_y + 15, 290, 15, 0x000000);
            last_cursor_pos = {-1, -1};
            last_unsaved_changes = false;
        }
    }
} 