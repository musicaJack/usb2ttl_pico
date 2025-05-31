# TTL Keyboard Pico项目当前状态

## 项目概述
基于树莓派Pico的TTL键盘系统，通过UART1串口接收键盘输入，配备ILI9488 3.5英寸TFT-LCD显示屏。

**重要说明：** 
- 本项目与USB协议完全无关
- 键盘输入通过USB2TTL模块转换为串口信号
- Pico通过UART1 (GPIO 8/9) 接收数据
- 完全基于串口通信，波特率9600

## 硬件配置

### 信号流程
```
键盘 → USB2TTL模块 → UART1串口 → 树莓派Pico
     (USB转串口)    (GPIO 8/9)
```

### 主要连接
```
USB2TTL模块    →    树莓派Pico
TX             →    GPIO 9 (RX) - UART1
RX             →    GPIO 8 (TX) - UART1  
GND            →    GND
```

**重要说明：**
- 树莓派Pico的GPIO 8配置为UART1的TX功能
- 树莓派Pico的GPIO 9配置为UART1的RX功能
- USB2TTL模块的TX连接到Pico的RX (GPIO 9)
- USB2TTL模块的RX连接到Pico的TX (GPIO 8)
- 通信协议：9600波特率，8数据位，1停止位，无校验

### 调试输出（可选）
```
GPIO 0 (TX)    →    另一个USB2TTL模块（用于调试输出）
```

### 显示屏连接
- ILI9488 3.5英寸TFT-LCD
- 使用RGB666颜色格式
- SPI接口连接

## 软件特性

### C++17现代化
- 统一使用.hpp头文件扩展名
- `#pragma once`替代传统include guards
- 完整的Doxygen文档注释
- 移动语义支持
- 命名空间组织（usb2ttl）

### 键盘输入处理
- **波特率**: 9600
- **UART**: UART1 (GPIO 8/9)
- **按键映射**: 基于MicroPython参考配置
- **重复按键过滤**: 200毫秒阈值
- **修饰键过滤**: 自动忽略Shift、Alt、Ctrl键

### 支持的按键类型
- 可打印ASCII字符 (32-126)
- 控制键: Enter, Backspace, Tab, ESC, Space
- 功能键: F1-F12
- 方向键: Up, Down, Left, Right
- 特殊键: Home, End, Insert, Delete, PageUp, PageDown

### 界面模式
1. **命令模式**: 显示"KEYBOARD COMMANDS"
2. **编辑模式**: 按Enter进入文本编辑

### 文本编辑功能
- 字符输入和删除
- 自动换行
- Tab键支持（4个空格）
- F10保存到文件
- ESC返回命令模式

## 构建结果

### 内存使用
- **FLASH**: 373,316 B (17.80%)
- **RAM**: 15,224 B (5.81%)

### 生成文件
- `usb2ttl_demo.uf2` (726KB) - 主程序
- `test_uart.uf2` (40KB) - UART测试程序

## 调试功能

### UART测试程序
- 简单的串口回显测试
- 使用UART0 (GPIO 0/1)作为调试输出
- 独立于主程序的测试工具

### 调试输出
- 详细的UART初始化信息
- 原始字节数据显示（十六进制和字符）
- 按键映射和重复检测日志
- 连接状态监控

## 使用方法

### 1. 烧录固件
```bash
# 主程序
cp usb2ttl_demo.uf2 /path/to/pico/drive/

# 或UART测试程序
cp test_uart.uf2 /path/to/pico/drive/
```

### 2. 硬件连接
按照上述硬件配置连接USB2TTL模块和键盘

### 3. 操作流程
1. 上电后显示"KEYBOARD COMMANDS"
2. 按任意键测试连接
3. 按Enter进入文本编辑模式
4. 输入文本，F10保存，ESC返回

## 技术特点

### 颜色系统
- 原生RGB666支持
- 智能颜色转换（RGB666↔RGB888↔RGB565）
- 优化的显示性能

### 错误处理
- 缓冲区溢出保护
- 连接超时检测（5秒）
- 重复按键过滤
- 修饰键自动忽略

### 代码质量
- 现代C++17特性
- RAII资源管理
- 异常安全设计
- 完整的文档注释

## 下一步计划
- 根据实际key_map.json文件调整按键映射
- 添加更多文本编辑功能（光标移动、选择等）
- 优化显示性能
- 添加配置文件支持 