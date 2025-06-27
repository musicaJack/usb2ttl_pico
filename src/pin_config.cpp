/**
 * @file pin_config.cpp
 * @brief 引脚配置实现文件
 */

#include "pin_config.hpp"
#include "hardware/spi.h"
#include "hardware/uart.h"

namespace pin_config {
    
    namespace display_spi_pins {
        spi_inst_t* get_spi_instance() {
            return spi0;
        }
    }
    
    namespace uart_config {
        uart_inst_t* get_uart_instance() {
            return uart0;
        }
    }
    
} // namespace pin_config 