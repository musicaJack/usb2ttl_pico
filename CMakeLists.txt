# TTL Keyboard CMakeLists.txt for USB2TTL Pico Demo

cmake_minimum_required(VERSION 3.13)

# 设置项目名称和版�?
set(PROJECT_NAME usb2ttl_pico)
set(PROJECT_VERSION 2.0.0)

# 包含 Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# 设置 C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 声明项目
project(${PROJECT_NAME} C CXX ASM)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -Wno-unused-parameter -Wno-pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -Wno-unused-parameter")

# 初始�?Pico SDK
pico_sdk_init()

# 包含头文件目�?
include_directories(include)
include_directories(include/ili9488)

# 收集源文�?
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# 创建TTL键盘演示程序
add_executable(usb2ttl_demo
    examples/usb2ttl_demo.cpp
    ${SOURCES}
)

# 链接必要的库
target_link_libraries(usb2ttl_demo
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_timer
    hardware_dma
)

# 包含项目头文件目�?
target_include_directories(usb2ttl_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/ili9488
)

# 启用 USB 输出用于调试
pico_enable_stdio_usb(usb2ttl_demo 1)
pico_enable_stdio_uart(usb2ttl_demo 0)

# 创建 map/bin/hex/uf2 文件
pico_add_extra_outputs(usb2ttl_demo)

# 添加编译定义
target_compile_definitions(usb2ttl_demo PRIVATE
    # TTL键盘UART配置
    PICO_DEFAULT_UART=1
    PICO_DEFAULT_UART_TX_PIN=8
    PICO_DEFAULT_UART_RX_PIN=9
    TTL_KEYBOARD_BAUD=115200
    # ILI9488相关定义
    ILI9488_SPI_SPEED=40000000
    ILI9488_WIDTH=320
    ILI9488_HEIGHT=480
)

# 创建UART测试程序
add_executable(test_uart
    examples/test_uart.cpp
)

# 链接UART测试程序的库
target_link_libraries(test_uart
    pico_stdlib
    hardware_uart
    hardware_gpio
)

# 启用 USB 输出用于调试
pico_enable_stdio_usb(test_uart 1)
pico_enable_stdio_uart(test_uart 0)

# 创建 map/bin/hex/uf2 文件
pico_add_extra_outputs(test_uart)

# 设置编译定义
target_compile_definitions(test_uart PRIVATE
    # 使用UART1作为TTL键盘输入
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=8
    PICO_DEFAULT_UART_RX_PIN=9
)

# 添加调试UART程序
add_executable(debug_uart
    examples/debug_uart.cpp
)

# 链接必要的库
target_link_libraries(debug_uart
    pico_stdlib
    hardware_uart
    hardware_gpio
)

# 启用USB输出用于调试
pico_enable_stdio_usb(debug_uart 1)
pico_enable_stdio_uart(debug_uart 0)

# 生成UF2文件
pico_add_extra_outputs(debug_uart) 
