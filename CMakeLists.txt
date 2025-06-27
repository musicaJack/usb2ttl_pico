# TTL Keyboard CMakeLists.txt for USB2TTL Pico Demo

cmake_minimum_required(VERSION 3.13)

# 设置项目名称和版本
set(PROJECT_NAME usb2ttl_pico)
set(PROJECT_VERSION 2.1.0)

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

# 初始化Pico SDK
pico_sdk_init()

# 包含头文件目录
include_directories(include)
include_directories(include/ili9488)
include_directories(include/st73xx)

# 收集源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# 收集ST73XX专用源文件
file(GLOB_RECURSE ST73XX_SOURCES
    "src/st73xx/*.cpp"
    "src/st73xx/*.c"
    "src/ttl_keyboard.cpp"
    "src/text_editor.cpp"
    "src/display_driver.cpp"
)

# 创建TTL键盘演示程序 (ILI9488版本)
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

# 包含项目头文件目录
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
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    TTL_KEYBOARD_BAUD=115200
    # ILI9488相关定义
    ILI9488_SPI_SPEED=40000000
    ILI9488_WIDTH=320
    ILI9488_HEIGHT=480
)

# 创建TTL键盘演示程序 (ST7306版本)
add_executable(usb2ttl_demo_st7306
    examples/usb2ttl_demo_st7306.cpp
    ${SOURCES}
)

# 链接必要的库
target_link_libraries(usb2ttl_demo_st7306
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_spi
    hardware_pwm
    hardware_timer
    hardware_dma
)

# 包含项目头文件目录
target_include_directories(usb2ttl_demo_st7306 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/st73xx
)

# 启用 USB 输出用于调试
pico_enable_stdio_usb(usb2ttl_demo_st7306 1)
pico_enable_stdio_uart(usb2ttl_demo_st7306 0)

# 创建 map/bin/hex/uf2 文件
pico_add_extra_outputs(usb2ttl_demo_st7306)

# 添加编译定义
target_compile_definitions(usb2ttl_demo_st7306 PRIVATE
    # TTL键盘UART配置
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    TTL_KEYBOARD_BAUD=115200
    # ST7306相关定义
    ST7306_WIDTH=300
    ST7306_HEIGHT=400
)

# 创建ST7306测试程序
add_executable(st7306_test
    examples/st7306_test.cpp
    ${ST73XX_SOURCES}
)

# 链接必要的库
target_link_libraries(st7306_test
    pico_stdlib
    hardware_gpio
    hardware_spi
)

# 包含项目头文件目录
target_include_directories(st7306_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/st73xx
)

# 启用 USB 输出用于调试
pico_enable_stdio_usb(st7306_test 1)
pico_enable_stdio_uart(st7306_test 0)

# 创建 map/bin/hex/uf2 文件
pico_add_extra_outputs(st7306_test)

# 添加编译定义
target_compile_definitions(st7306_test PRIVATE
    # ST7306相关定义
    ST7306_WIDTH=300
    ST7306_HEIGHT=400
)

