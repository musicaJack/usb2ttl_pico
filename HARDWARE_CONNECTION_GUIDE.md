# USB2TTL Pico硬件连接指南

## 确认的硬件配置

### 树莓派Pico UART1配置
- **GPIO 8**: 配置为UART1 TX功能
- **GPIO 9**: 配置为UART1 RX功能
- **波特率**: 9600
- **数据位**: 8位
- **停止位**: 1位
- **校验位**: 无

### USB2TTL模块连接
```
USB2TTL模块引脚    →    树莓派Pico引脚
TX                →    GPIO 9 (RX)
RX                →    GPIO 8 (TX)
GND               →    GND
VCC               →    3.3V (可选，如果模块需要供电)
```

### 连接原理
- USB2TTL模块的TX输出连接到Pico的RX输入 (GPIO 9)
- USB2TTL模块的RX输入连接到Pico的TX输出 (GPIO 8)
- 这样形成正确的串口通信回路

### ILI9488显示屏连接
```
ILI9488引脚    →    树莓派Pico引脚
DC             →    GPIO 20
RST            →    GPIO 15
CS             →    GPIO 17
SCK            →    GPIO 18
MOSI           →    GPIO 19
BL             →    GPIO 10
VCC            →    3.3V
GND            →    GND
```

## 代码配置验证

### 头文件默认参数 (include/usb_keyboard.hpp)
```cpp
bool initialize(uart_inst_t* uart_instance = uart1, 
               std::uint32_t baud_rate = 9600,
               std::uint8_t tx_pin = 8, 
               std::uint8_t rx_pin = 9);
```

### Demo程序配置 (examples/usb2ttl_demo.cpp)
```cpp
namespace HardwareConfig {
    uart_inst_t* const UART_INSTANCE = uart1;
    constexpr std::uint8_t PIN_TX = 8;
    constexpr std::uint8_t PIN_RX = 9;
    constexpr std::uint32_t UART_BAUD = 9600;
}
```

## 测试步骤

### 1. 硬件连接检查
- [ ] 确认USB2TTL模块TX连接到Pico GPIO 9
- [ ] 确认USB2TTL模块RX连接到Pico GPIO 8
- [ ] 确认GND连接
- [ ] 确认ILI9488显示屏连接正确

### 2. 固件烧录
```bash
# 烧录主程序
cp usb2ttl_demo.uf2 /path/to/pico/drive/

# 或烧录测试程序
cp test_uart.uf2 /path/to/pico/drive/
```

### 3. 功能测试
1. 上电后ILI9488显示"KEYBOARD COMMANDS"界面
2. 通过USB2TTL模块连接键盘
3. 按键应该在显示屏上有响应
4. 按Enter进入文本编辑模式
5. 输入字符应该显示在屏幕上

### 4. 调试输出（可选）
如果需要查看调试信息：
- 连接另一个USB2TTL模块到GPIO 0 (TX)
- 波特率115200
- 可以看到详细的初始化和按键处理日志

## 故障排除

### 键盘无响应
1. 检查USB2TTL模块连接是否正确
2. 确认波特率设置为9600
3. 检查TX/RX是否交叉连接
4. 查看调试输出确认UART初始化成功

### 显示屏无显示
1. 检查ILI9488 SPI连接
2. 确认电源供应正常
3. 检查背光引脚连接

### 编译错误
1. 确认Pico SDK版本兼容
2. 检查CMakeLists.txt配置
3. 清理build目录重新编译

## 技术规格

### 内存使用
- **FLASH**: 373,316 B (17.80%)
- **RAM**: 15,224 B (5.81%)

### 性能特性
- 200毫秒重复按键过滤
- 5秒连接超时检测
- RGB666原生颜色支持
- 实时按键响应 