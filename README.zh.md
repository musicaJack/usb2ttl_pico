# USB2TTL Pico 键盘系统
![许可证](https://img.shields.io/badge/许可证-MIT-blue.svg)
![平台](https://img.shields.io/badge/平台-Raspberry%20Pi%20Pico-brightgreen.svg)
![版本](https://img.shields.io/badge/版本-1.0.0-orange.svg)
[English](README.md) | 中文

## 项目概述

这是一个基于树莓派Pico的TTL键盘输入系统，通过USB2TTL模块接收键盘输入，并在ILI9488 3.5英寸TFT显示屏上提供文本编辑功能。

<p align="center">
  <img src="imgs/img_of_material.jpg" alt="affect1" width="300"/>
</p>
### 核心特性
- **纯串口通信**: 完全基于UART通信，与USB协议无关
- **双模式界面**: 命令模式和文本编辑模式
- **智能按键过滤**: 5ms重复按键过滤，有效处理USB2TTL模块重复发送
- **现代C++**: 使用C++17特性，完整的面向对象设计
- **RGB666显示**: 原生支持ILI9488的RGB666颜色系统

### 版本信息
- **版本**: 2.1.0
- **硬件**: Raspberry Pi Pico + ILI9488 + USB2TTL模块
- **开发语言**: C++17
- **内存使用**: FLASH 782KB (17.8%), RAM 15KB (5.8%)

## 硬件连接

### 信号流程
```
键盘 → USB2TTL模块 → UART1串口 → 树莓派Pico → ILI9488显示屏
     (USB转串口)    (GPIO 8/9)              (SPI接口)
```

### TTL键盘连接 (UART1)
```
USB2TTL模块    →    树莓派Pico
TX             →    GPIO 9 (RX)
RX             →    GPIO 8 (TX)  
GND            →    GND
VCC            →    3.3V (可选)
```

**重要说明**:
- 波特率: **115200**
- 数据位: 8位，停止位: 1位，无校验
- TX/RX交叉连接确保正确通信

### ILI9488显示屏连接 (SPI0)
```
ILI9488引脚    →    树莓派Pico引脚
CS             →    GPIO 17
DC             →    GPIO 20
RST            →    GPIO 15
SCK            →    GPIO 18
MOSI           →    GPIO 19
BL             →    GPIO 10 (背光)
VCC            →    3.3V
GND            →    GND
```

### 调试输出 (可选)
```
GPIO 0 (TX)    →    USB2TTL模块 (115200波特率)
```

## 软件架构

### 核心类设计

#### 1. TTLKeyboard (`include/ttl_keyboard.hpp`)
- **功能**: 处理UART1串口通信和按键解析
- **特性**: 
  - 智能重复按键过滤 (5ms阈值)
  - 噪声数据过滤 (0xFF/0x00字节)
  - 连接状态检测 (5秒超时)
  - 完整的按键映射表

#### 2. TextEditor (`include/text_editor.hpp`)
- **功能**: 文本编辑和显示管理
- **特性**:
  - 38字符/行，80行容量 (3040字符总容量)
  - 智能自动换行
  - 局部刷新减少闪烁
  - 输入冻结保护

#### 3. ILI9488DisplayAdapter (`examples/usb2ttl_demo.cpp`)
- **功能**: 显示驱动适配器
- **特性**:
  - RGB666原生颜色支持
  - 优化的图形绘制
  - 智能颜色转换

### 应用状态管理
- **COMMAND_MODE**: 键盘命令界面
- **EDIT_MODE**: 文本编辑模式

## 使用说明

### 快速开始

1. **硬件连接**: 按照上述连接图连接所有硬件
2. **烧录固件**: 将`usb2ttl_demo.uf2`拖拽到Pico的USB驱动器
3. **连接键盘**: 将键盘连接到USB2TTL模块
4. **开始使用**: 系统启动后显示命令界面

### 操作指南

#### 命令模式
| 按键 | 功能 |
|------|------|
| Enter | 进入文本编辑模式 |
| ESC | 刷新命令界面 |

#### 编辑模式
| 按键 | 功能 |
|------|------|
| ESC | 返回命令模式 |
| Enter | 换行 |
| Backspace | 删除字符 |
| Tab | 插入制表符 |
| 字母/数字/符号 | 输入字符 |

### 状态显示

屏幕底部实时显示：
- **TTL-KB**: 键盘连接状态 (Connected/Waiting...)
- **模式**: 当前模式 (COMMAND/EDIT)
- **运行时间**: 系统运行时间
- **光标位置**: L:行 C:列 (编辑模式)
- **行数统计**: 当前行数/最大行数

## 构建说明

### 环境要求
- **Pico SDK**: v1.5.1+
- **CMake**: 3.13+
- **编译器**: GCC ARM工具链
- **操作系统**: Windows/Linux/macOS

### 构建步骤
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译 (Windows使用ninja)
ninja

# 或在Linux/macOS使用make
make -j4
```

### 输出文件
- `usb2ttl_demo.uf2` (782KB) - 主程序
- `debug_uart.uf2` (68KB) - UART调试工具
- `test_uart.uf2` (66KB) - UART测试程序

## 技术特性

### 通信协议
- **UART实例**: UART1
- **GPIO配置**: TX=8, RX=9
- **波特率**: 115200 bps
- **数据格式**: 8N1 (8数据位，无校验，1停止位)

### 按键处理
- **重复按键过滤**: 5ms阈值，有效过滤USB2TTL重复发送
- **噪声过滤**: 自动过滤0xFF/0x00噪声字节
- **连接检测**: 基于有效数据的5秒超时机制
- **支持按键**: ASCII字符、控制键、功能键

### 显示系统
- **颜色格式**: RGB666 (18位，262,144色)
- **分辨率**: 320x480像素
- **字体**: 8x16像素点阵字体
- **刷新策略**: 局部刷新优化

### 内存优化
- **FLASH使用**: 782KB / 2MB (38.2%)
- **RAM使用**: 15KB / 264KB (5.8%)
- **缓冲区管理**: 智能缓冲区复用
- **颜色转换**: 零拷贝优化

## 故障排除

### 常见问题

#### 1. 键盘无响应
**症状**: 显示"TTL-KB: Waiting..."
**解决方案**:
1. 检查USB2TTL模块连接
2. 确认波特率设置为115200
3. 验证TX/RX交叉连接
4. 检查键盘到USB2TTL的连接

#### 2. 重复字符输入
**症状**: 按一个键出现多个相同字符
**解决方案**:
- 已通过5ms重复按键过滤解决
- 如仍有问题，可调整`DUPLICATE_KEY_THRESHOLD`值

#### 3. 显示屏无显示
**症状**: 屏幕黑屏或花屏
**解决方案**:
1. 检查SPI连接线
2. 确认电源供应稳定
3. 检查背光连接 (GPIO 10)
4. 验证ILI9488型号匹配

#### 4. 编译错误
**常见错误及解决**:
```bash
# Pico SDK路径问题
export PICO_SDK_PATH=/path/to/pico-sdk

# 清理构建缓存
rm -rf build && mkdir build

# 检查CMake版本
cmake --version  # 需要 >= 3.13
```

### 调试工具

#### 使用debug_uart.uf2
1. 烧录调试程序到Pico
2. 连接调试串口 (GPIO 0, 115200波特率)
3. 观察详细的数据接收日志
4. 分析按键映射和时间间隔

#### 调试输出示例
```
--- TTL键盘接收到数据 (时间: 91418 ms) ---
字节数: 1
十六进制数据 (1字节): 65 
ASCII表示: e
特殊字符分析:
  [0] 可打印字符 'e'
检测到有效数据，进行按键解析
解析的按键: e
--- TTL键盘数据处理完成 ---
```

## 开发指南

### 添加新功能

#### 扩展按键支持
```cpp
// 在ttl_keyboard.cpp的init_key_map()中添加
key_map_[0x特定字节] = "新按键名";
```

#### 修改文本编辑器
```cpp
// 在text_editor.cpp中扩展功能
void TextEditor::new_feature() {
    // 实现新功能
}
```

### 代码规范
- 使用4空格缩进
- 函数和变量使用snake_case命名
- 类名使用PascalCase命名
- 完整的Doxygen文档注释

### 提交规范
```bash
git commit -m "feat: 添加新功能描述"
git commit -m "fix: 修复具体问题"
git commit -m "docs: 更新文档内容"
```

## 许可证

本项目采用MIT许可证，详见[LICENSE](LICENSE)文件。

## 更新日志

### v2.1.0 (当前版本)
- ✅ 优化重复按键过滤为5ms阈值
- ✅ 修复文本编辑器内容保留问题
- ✅ 修正行列坐标系统参数顺序
- ✅ 统一波特率为115200
- ✅ 清理编译警告和未使用变量

### v2.0.0
- 🔄 重构为TTL键盘输入系统
- ❌ 移除USB HID依赖
- ✅ 简化为双模式界面
- ✅ 优化RGB666颜色系统
- ✅ 改进错误处理和状态显示

### v1.0.0
- 🎉 初始版本发布
- ✅ 基础USB键盘功能
- ✅ ILI9488显示支持

## 贡献

欢迎任何形式的贡献！请参考[CONTRIBUTING.md](CONTRIBUTING.md)了解详细的贡献指南。

### 贡献方式
- 🐛 报告Bug
- 💡 提出新功能建议  
- 📝 改进文档
- 🔧 提交代码

---

**项目地址**: [GitHub Repository]
**技术支持**: 请通过Issues提交问题
**开发者**: USB2TTL Pico Team 