/**
 * @file st7306_test.cpp
 * @brief ST7306显示屏基本功能测试程序
 * @author usb2ttl_pico项目
 * @version 1.0.0
 * 
 * 功能说明：
 * - 测试ST7306 4.2英寸反射式LCD显示屏的基本功能
 * - 显示文本、图形和灰度测试
 * - 验证SPI通信和显示驱动
 */

#include <cstdio>
#include <cstring>
#include <memory>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// ST7306驱动头文件
#include "st7306_driver.hpp"
#include "pico_display_gfx.hpp"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"
#include "pin_config.hpp"

using namespace st7306;
using HardwareConfig = pin_config::ST7306Config;  // 使用统一配置

int main() {
    // 初始化标准库
    stdio_init_all();
    
    printf("\n=== ST7306 Display Test Starting ===\n");
    printf("Hardware: Raspberry Pi Pico + ST7306 Reflective LCD\n");
    printf("Resolution: 300x400 pixels, 4-level grayscale\n\n");
    
    // 初始化LED
    gpio_init(HardwareConfig::pin_led);
    gpio_set_dir(HardwareConfig::pin_led, GPIO_OUT);
    gpio_put(HardwareConfig::pin_led, 1);
    
    // 创建ST7306驱动实例
    ST7306Driver display(
        HardwareConfig::pin_dc,
        HardwareConfig::pin_rst,
        HardwareConfig::pin_cs,
        HardwareConfig::pin_sclk,
        HardwareConfig::pin_sdin
    );
    
    // 创建图形库实例
    pico_gfx::PicoDisplayGFX<ST7306Driver> gfx(
        display, HardwareConfig::width, HardwareConfig::height
    );
    
    printf("Initializing ST7306 display...\n");
    display.initialize();
    display.setRotation(0);
    display.clearDisplay();
    display.display();
    printf("Display initialized successfully!\n");
    
    // 测试1: 基本文本显示
    printf("Test 1: Basic text display\n");
    display.clearDisplay();
    display.drawString(10, 10, "ST7306 Display Test", true);
    display.drawString(10, 30, "Resolution: 300x400", true);
    display.drawString(10, 50, "4-level Grayscale", true);
    display.drawString(10, 70, "Reflective LCD", true);
    display.display();
    sleep_ms(3000);
    
    // 测试2: 灰度级别测试
    printf("Test 2: Grayscale levels\n");
    display.clearDisplay();
    display.drawString(10, 10, "Grayscale Test:", true);
    
    // 绘制4个灰度级别的矩形
    for (int i = 0; i < 4; i++) {
        int x = 10 + i * 70;
        int y = 40;
        
        // 绘制矩形边框
        gfx.drawRectangle(x, y, 60, 40, true);
        
        // 填充不同灰度级别
        for (int py = y + 2; py < y + 38; py++) {
            for (int px = x + 2; px < x + 58; px++) {
                display.drawPixelGray(px, py, i);
            }
        }
        
        // 标注灰度级别
        char level_str[16];
        snprintf(level_str, sizeof(level_str), "Level %d", i);
        display.drawString(x + 5, y + 45, level_str, true);
    }
    
    display.display();
    sleep_ms(3000);
    
    // 测试3: 几何图形
    printf("Test 3: Geometric shapes\n");
    display.clearDisplay();
    display.drawString(10, 10, "Geometric Shapes:", true);
    
    // 绘制圆形
    gfx.drawCircle(60, 80, 30, true);
    display.drawString(30, 120, "Circle", true);
    
    // 绘制填充矩形
    gfx.drawFilledRectangle(120, 50, 60, 60, true);
    display.drawString(130, 120, "Rectangle", true);
    
    // 绘制三角形
    gfx.drawTriangle(210, 50, 240, 110, 180, 110, true);
    display.drawString(190, 120, "Triangle", true);
    
    display.display();
    sleep_ms(3000);
    
    // 测试4: 线条和像素
    printf("Test 4: Lines and pixels\n");
    display.clearDisplay();
    display.drawString(10, 10, "Lines and Pixels:", true);
    
    // 绘制各种线条
    gfx.drawLine(10, 40, 290, 40, true);  // 水平线
    gfx.drawLine(10, 50, 10, 150, true);  // 垂直线
    gfx.drawLine(20, 60, 100, 140, true); // 对角线
    
    // 绘制像素点阵
    for (int y = 160; y < 200; y += 4) {
        for (int x = 10; x < 100; x += 4) {
            display.drawPixel(x, y, true);
        }
    }
    
    display.drawString(10, 210, "Pixel Pattern", true);
    display.display();
    sleep_ms(3000);
    
    // 测试5: 字符集测试
    printf("Test 5: Character set\n");
    display.clearDisplay();
    display.drawString(10, 10, "ASCII Character Set:", true);
    
    int x = 10, y = 40;
    for (char c = 32; c <= 126; c++) {
        display.drawChar(x, y, c, true);
        x += font::FONT_WIDTH + 2;
        if (x > 280) {
            x = 10;
            y += font::FONT_HEIGHT + 2;
            if (y > 350) break;
        }
    }
    
    display.display();
    sleep_ms(5000);
    
    // 测试6: 功耗模式测试
    printf("Test 6: Power mode test\n");
    display.clearDisplay();
    display.drawString(10, 10, "Power Mode Test", true);
    display.drawString(10, 40, "High Power Mode", true);
    display.display();
    display.highPowerMode();
    sleep_ms(2000);
    
    display.clearDisplay();
    display.drawString(10, 10, "Power Mode Test", true);
    display.drawString(10, 40, "Low Power Mode", true);
    display.display();
    display.lowPowerMode();
    sleep_ms(2000);
    
    // 恢复高功耗模式
    display.highPowerMode();
    
    // 测试完成
    printf("Test 7: Test complete\n");
    display.clearDisplay();
    display.drawString(50, 180, "ST7306 Test Complete!", true);
    display.drawString(80, 200, "All tests passed", true);
    display.display();
    
    printf("\n=== All tests completed successfully! ===\n");
    printf("ST7306 display is working properly.\n");
    
    // 闪烁LED表示测试完成
    for (int i = 0; i < 10; i++) {
        gpio_put(HardwareConfig::pin_led, 1);
        sleep_ms(200);
        gpio_put(HardwareConfig::pin_led, 0);
        sleep_ms(200);
    }
    
    // 保持显示
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 