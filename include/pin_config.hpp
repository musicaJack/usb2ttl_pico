/**
 * @file pin_config.hpp
 * @brief USB2TTL Pico项目统一引脚配置文件
 * 
 * 本文件定义了整个项目中使用的硬件引脚配置，包括：
 * - SPI显示接口引脚配置
 * - UART通信引脚配置
 * - 各种显示屏的硬件配置模板
 * 
 * 所有引脚配置都应通过此文件统一管理，避免在代码中硬编码引脚号。
 */

#ifndef PIN_CONFIG_HPP
#define PIN_CONFIG_HPP

#include <cstdint>

// 声明硬件类型
struct spi_inst;
typedef struct spi_inst spi_inst_t;
struct uart_inst;
typedef struct uart_inst uart_inst_t;

namespace pin_config {
    
    // =================================================================
    // SPI显示接口统一配置（所有显示屏共用的SPI接口）
    // =================================================================
    
    namespace display_spi_pins {
        /// @brief 获取SPI实例
        spi_inst_t* get_spi_instance();
        
        /// @brief SPI速度 (40MHz)
        constexpr std::uint32_t SPI_SPEED = 40 * 1000 * 1000;
        
        /// @brief SPI引脚定义
        constexpr std::uint8_t PIN_SCK = 18;   ///< SPI时钟线 (SCL)
        constexpr std::uint8_t PIN_MOSI = 19;  ///< SPI数据线 (SDA)
        constexpr std::uint8_t PIN_CS = 17;    ///< SPI片选信号
        constexpr std::uint8_t PIN_DC = 20;    ///< 数据/命令选择
        constexpr std::uint8_t PIN_RST = 15;   ///< 重置信号
        constexpr std::uint8_t PIN_BL = 16;    ///< 背光控制（仅部分显示屏使用）
        constexpr std::uint8_t PIN_LED = 25;   ///< LED指示灯
        constexpr std::uint8_t PIN_TE = 21;    ///< 撕裂效应信号（保留接口）
    }
    
    // =================================================================
    // UART通信接口配置
    // =================================================================
    
    namespace uart_config {
        
        /// @brief 获取UART实例
        uart_inst_t* get_uart_instance();
        
        /// @brief UART引脚定义
        constexpr std::uint8_t PIN_TX = 0;   ///< UART发送引脚
        constexpr std::uint8_t PIN_RX = 1;   ///< UART接收引脚
        
        /// @brief UART波特率
        constexpr std::uint32_t BAUD_RATE = 115200;
        
        /// @brief 调试UART配置（与uart0相同，用于调试输出）
        struct debug_uart {
            /// @brief UART实例
            static uart_inst_t* UART_INSTANCE() { return get_uart_instance(); }
            
            /// @brief UART引脚定义
            static constexpr std::uint8_t PIN_TX = 0;   ///< UART发送引脚
            static constexpr std::uint8_t PIN_RX = 1;   ///< UART接收引脚
            
            /// @brief UART波特率
            static constexpr std::uint32_t BAUD_RATE = 115200;
        };
    }
    
    // =================================================================
    // 显示屏硬件配置模板
    // =================================================================
    
    /// @brief ILI9488显示屏配置
    struct ILI9488Config {
        // SPI接口配置
        static spi_inst_t* spi_instance() { return display_spi_pins::get_spi_instance(); }
        static constexpr std::uint32_t spi_speed = display_spi_pins::SPI_SPEED;
        static constexpr std::uint8_t pin_sck = display_spi_pins::PIN_SCK;
        static constexpr std::uint8_t pin_mosi = display_spi_pins::PIN_MOSI;
        static constexpr std::uint8_t pin_cs = display_spi_pins::PIN_CS;
        static constexpr std::uint8_t pin_dc = display_spi_pins::PIN_DC;
        static constexpr std::uint8_t pin_rst = display_spi_pins::PIN_RST;
        static constexpr std::uint8_t pin_bl = display_spi_pins::PIN_BL;  // 背光控制
        
        // UART接口配置
        static uart_inst_t* uart_instance() { return uart_config::get_uart_instance(); }
        static constexpr std::uint8_t uart_tx = uart_config::PIN_TX;
        static constexpr std::uint8_t uart_rx = uart_config::PIN_RX;
        static constexpr std::uint32_t uart_baud = uart_config::BAUD_RATE;
        
        // 显示屏规格
        static constexpr std::uint16_t width = 320;
        static constexpr std::uint16_t height = 480;
        
        // LED指示（使用背光引脚作为LED）
        static constexpr std::uint8_t pin_led = display_spi_pins::PIN_BL;
    };
    
    /// @brief ST7306显示屏配置  
    struct ST7306Config {
        // SPI接口配置
        static spi_inst_t* spi_instance() { return display_spi_pins::get_spi_instance(); }
        static constexpr std::uint32_t spi_speed = display_spi_pins::SPI_SPEED;
        static constexpr std::uint8_t pin_sclk = display_spi_pins::PIN_SCK;  // ST7306使用SCLK命名
        static constexpr std::uint8_t pin_sdin = display_spi_pins::PIN_MOSI; // ST7306使用SDIN命名
        static constexpr std::uint8_t pin_cs = display_spi_pins::PIN_CS;
        static constexpr std::uint8_t pin_dc = display_spi_pins::PIN_DC;
        static constexpr std::uint8_t pin_rst = display_spi_pins::PIN_RST;
        
        // UART接口配置
        static uart_inst_t* uart_instance() { return uart_config::get_uart_instance(); }
        static constexpr std::uint8_t uart_tx = uart_config::PIN_TX;
        static constexpr std::uint8_t uart_rx = uart_config::PIN_RX;
        static constexpr std::uint32_t uart_baud = uart_config::BAUD_RATE;
        
        // 显示屏规格
        static constexpr std::uint16_t width = 240;
        static constexpr std::uint16_t height = 240;
        
        // LED指示灯
        static constexpr std::uint8_t pin_led = display_spi_pins::PIN_LED;
    };
    
    /// @brief ST7305显示屏配置（与ST7306相同）
    struct ST7305Config {
        // SPI接口配置
        static spi_inst_t* spi_instance() { return display_spi_pins::get_spi_instance(); }
        static constexpr std::uint32_t spi_speed = display_spi_pins::SPI_SPEED;
        static constexpr std::uint8_t pin_sclk = display_spi_pins::PIN_SCK;  
        static constexpr std::uint8_t pin_sdin = display_spi_pins::PIN_MOSI; 
        static constexpr std::uint8_t pin_cs = display_spi_pins::PIN_CS;
        static constexpr std::uint8_t pin_dc = display_spi_pins::PIN_DC;
        static constexpr std::uint8_t pin_rst = display_spi_pins::PIN_RST;
        
        // UART接口配置
        static uart_inst_t* uart_instance() { return uart_config::get_uart_instance(); }
        static constexpr std::uint8_t uart_tx = uart_config::PIN_TX;
        static constexpr std::uint8_t uart_rx = uart_config::PIN_RX;
        static constexpr std::uint32_t uart_baud = uart_config::BAUD_RATE;
        
        // 显示屏规格
        static constexpr std::uint16_t width = 240;
        static constexpr std::uint16_t height = 240;
        
        // LED指示灯
        static constexpr std::uint8_t pin_led = display_spi_pins::PIN_LED;
    };
    
    // =================================================================
    // 便捷别名定义
    // =================================================================
    
    /// @brief 调试UART配置别名
    using DebugConfig = uart_config::debug_uart;
    
} // namespace pin_config

#endif // PIN_CONFIG_HPP 