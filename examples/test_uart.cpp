#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

int main() {
    stdio_init_all();
    
    printf("UART1 Test Program Starting...\n");
    printf("Testing UART1 on GPIO 8 (TX) and GPIO 9 (RX)\n");
    
    // 初始化UART1
    uint actual_baud = uart_init(uart1, 115200);
    printf("UART1 initialized with baud rate: %u\n", actual_baud);
    
    // 设置GPIO引脚功能
    gpio_set_function(8, GPIO_FUNC_UART);
    gpio_set_function(9, GPIO_FUNC_UART);
    printf("GPIO 8 set to UART TX\n");
    printf("GPIO 9 set to UART RX\n");
    
    // 配置UART参数
    uart_set_hw_flow(uart1, false, false);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart1, true);
    
    printf("UART1 configuration complete\n");
    printf("Waiting for data on GPIO 9 (RX)...\n");
    printf("Connect your USB2TTL module:\n");
    printf("  USB2TTL TX -> Pico GPIO 9 (RX)\n");
    printf("  USB2TTL RX -> Pico GPIO 8 (TX)\n");
    printf("  USB2TTL GND -> Pico GND\n\n");
    
    uint32_t byte_count = 0;
    
    while (true) {
        // 检查是否有数据可读
        if (uart_is_readable(uart1)) {
            char ch = uart_getc(uart1);
            byte_count++;
            
            printf("RX[%lu]: 0x%02X", byte_count, (unsigned char)ch);
            if (ch >= 32 && ch <= 126) {
                printf(" ('%c')", ch);
            }
            printf("\n");
            
            // 回显数据（发送回去）
            uart_putc(uart1, ch);
        }
        
        
        sleep_ms(1);
    }
    
    return 0;
} 
