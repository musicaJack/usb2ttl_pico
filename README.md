# USB2TTL Pico Keyboard System
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-brightgreen.svg)
![Version](https://img.shields.io/badge/version-2.1.0-orange.svg)
English | [中文](README.zh.md)

## Project Overview

This is a TTL keyboard input system based on Raspberry Pi Pico that receives keyboard input through a USB2TTL module and provides text editing functionality on display screens. The system now supports both ILI9488 TFT and ST7306 reflective LCD displays.

<p align="center">
  <img src="imgs/img_of_material.jpg" alt="affect1" width="300"/>
</p>

### Core Features
- **Pure Serial Communication**: Completely based on UART communication, independent of USB protocol
- **Dual Display Support**: Compatible with ILI9488 TFT and ST7306 reflective LCD
- **Dual-Mode Interface**: Command mode and text editing mode
- **Smart Key Filtering**: 200ms duplicate key filtering, effectively handles USB2TTL module repeat transmissions
- **Modern C++**: Uses C++17 features with complete object-oriented design
- **Adaptive Color System**: RGB666 for ILI9488, 4-level grayscale for ST7306

### Version Information
- **Version**: 2.1.0
- **Hardware**: Raspberry Pi Pico + Display + USB2TTL Module
- **Supported Displays**: 
  - ILI9488 3.5" TFT (320x480, RGB666)
  - ST7306 4.2" Reflective LCD (300x400, 4-level grayscale)
- **Development Language**: C++17
- **Memory Usage**: FLASH 783KB (17.9%), RAM 15KB (5.8%)

## Hardware Connections

### Signal Flow
```
Keyboard → USB2TTL Module → UART1 Serial → Raspberry Pi Pico → Display
         (USB to Serial)   (GPIO 8/9)                        (SPI Interface)
```

### TTL Keyboard Connection (UART1)
```
USB2TTL Module    →    Raspberry Pi Pico
TX                →    GPIO 9 (RX)
RX                →    GPIO 8 (TX)  
GND               →    GND
VCC               →    3.3V (Optional)
```

**Important Notes**:
- Baud Rate: **115200**
- Data Bits: 8 bits, Stop Bits: 1 bit, No Parity
- TX/RX cross-connection ensures proper communication

### ILI9488 Display Connection (SPI0)
```
ILI9488 Pin       →    Raspberry Pi Pico Pin
CS                →    GPIO 17
DC                →    GPIO 20
RST               →    GPIO 15
SCK               →    GPIO 18
MOSI              →    GPIO 19
BL                →    GPIO 10 (Backlight)
VCC               →    3.3V
GND               →    GND
```

### ST7306 Display Connection (SPI0)
```
ST7306 Pin        →    Raspberry Pi Pico Pin
CS                →    GPIO 17
DC                →    GPIO 20
RST               →    GPIO 15
SCK               →    GPIO 18
MOSI              →    GPIO 19
VCC               →    3.3V
GND               →    GND
```

**ST7306 Features**:
- **Resolution**: 300x400 pixels
- **Display Type**: Reflective LCD (no backlight needed)
- **Color Depth**: 4-level grayscale (2-bit per pixel)
- **Power Consumption**: Ultra-low power with sleep mode support

### Debug Output (Optional)
```
GPIO 0 (TX)       →    USB2TTL Module (115200 baud rate)
```

## Software Architecture

### Core Class Design

#### 1. TTLKeyboard (`include/ttl_keyboard.hpp`)
- **Function**: Handles UART1 serial communication and key parsing
- **Features**: 
  - Smart duplicate key filtering (200ms threshold)
  - Noise data filtering (0xFF/0x00 bytes)
  - Connection status detection (5-second timeout)
  - Complete key mapping table

#### 2. TextEditor (`include/text_editor.hpp`)
- **Function**: Text editing and display management
- **Features**:
  - Adaptive layout for different screen sizes
  - Smart auto-wrap
  - Partial refresh to reduce flicker
  - Input freeze protection

#### 3. Display Adapters
- **ILI9488DisplayAdapter**: RGB666 color support, backlight control
- **ST7306DisplayAdapter**: 4-level grayscale, power mode control

### Application State Management
- **COMMAND_MODE**: Keyboard command interface
- **EDIT_MODE**: Text editing mode

## Usage Instructions

### Quick Start

1. **Hardware Connection**: Connect all hardware according to the connection diagram above
2. **Flash Firmware**: Drag `usb2ttl_demo.uf2` to Pico's USB drive
3. **Connect Keyboard**: Connect keyboard to USB2TTL module
4. **Start Using**: System displays command interface after startup

### Operation Guide

#### Command Mode
| Key | Function |
|-----|----------|
| Enter | Enter text editing mode |
| ESC | Refresh command interface |

#### Edit Mode
| Key | Function |
|-----|----------|
| ESC | Return to command mode |
| Enter | New line |
| Backspace | Delete character |
| Tab | Insert tab |
| Letters/Numbers/Symbols | Input character |

### Status Display

Real-time display at bottom of screen:
- **TTL-KB**: Keyboard connection status (Connected/Waiting...)
- **Mode**: Current mode (COMMAND/EDIT)
- **Runtime**: System runtime
- **Cursor Position**: L:line C:column (edit mode)
- **Line Count**: Current lines/Maximum lines

## Build Instructions

### Environment Requirements
- **Pico SDK**: v1.5.1+
- **CMake**: 3.13+
- **Compiler**: GCC ARM toolchain
- **Operating System**: Windows/Linux/macOS

### Build Steps
```bash
# Create build directory
mkdir build && cd build

# Configure project
cmake ..

# Compile (Windows uses ninja)
ninja

# Or use make on Linux/macOS
make -j4
```

### Output Files
- `usb2ttl_demo.uf2` (785KB) - ILI9488 main program
- `usb2ttl_demo_st7306.uf2` (783KB) - ST7306 main program
- `st7306_test.uf2` (734KB) - ST7306 display test program
- `debug_uart.uf2` (78KB) - UART debug tool

## Technical Features

### Communication Protocol
- **UART Instance**: UART1
- **GPIO Configuration**: TX=8, RX=9
- **Baud Rate**: 115200 bps
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)

### Key Processing
- **Duplicate Key Filtering**: 200ms threshold, effectively filters USB2TTL repeat transmissions
- **Noise Filtering**: Automatically filters 0xFF/0x00 noise bytes
- **Connection Detection**: 5-second timeout mechanism based on valid data
- **Supported Keys**: ASCII characters, control keys, function keys

### Display Systems

#### ILI9488 TFT Display
- **Color Format**: RGB666 (18-bit, 262,144 colors)
- **Resolution**: 320x480 pixels
- **Features**: Backlight control, high color accuracy
- **Text Layout**: 40 characters/line, 30 lines

#### ST7306 Reflective LCD
- **Color Format**: 4-level grayscale (2-bit per pixel)
- **Resolution**: 300x400 pixels
- **Features**: No backlight needed, ultra-low power consumption
- **Text Layout**: 37 characters/line, 25 lines
- **Power Modes**: High performance / Low power modes

### Memory Optimization
- **FLASH Usage**: 783KB / 2MB (38.3%)
- **RAM Usage**: 15KB / 264KB (5.8%)
- **Buffer Management**: Smart buffer reuse
- **Color Conversion**: Optimized grayscale mapping for ST7306

## Display-Specific Features

### ST7306 Advantages
- **Power Efficiency**: Reflective display requires no backlight
- **Sunlight Readable**: Excellent visibility in bright environments
- **Low Power Consumption**: Sleep mode support for battery applications
- **Eye Comfort**: No blue light emission, suitable for long-term reading

### ILI9488 Advantages
- **Rich Colors**: Full RGB666 color support
- **High Brightness**: Adjustable backlight for various lighting conditions
- **Fast Refresh**: Optimized for dynamic content display

## Troubleshooting

### Common Issues

#### 1. Keyboard Not Responding
**Symptoms**: Display shows "TTL-KB: Waiting..."
**Solutions**:
1. Check USB2TTL module connection
2. Confirm baud rate is set to 115200
3. Verify TX/RX cross-connection
4. Check keyboard to USB2TTL connection

#### 2. ST7306 Display Issues
**Symptoms**: Blank screen or incorrect display
**Solutions**:
1. Verify SPI connections (no backlight pin needed)
2. Check power supply stability
3. Ensure correct firmware (`usb2ttl_demo_st7306.uf2`)
4. Test with `st7306_test.uf2` for hardware verification

#### 3. Text Layout Issues
**Symptoms**: Text appears cut off or misaligned
**Solutions**:
- Use correct firmware for your display type
- ST7306: 37 chars/line, 25 lines
- ILI9488: 40 chars/line, 30 lines

#### 4. Compilation Errors
**Common errors and solutions**:
```bash
# Pico SDK path issue
export PICO_SDK_PATH=/path/to/pico-sdk

# Clean build cache
rm -rf build && mkdir build

# Check CMake version
cmake --version  # Requires >= 3.13
```

### Debug Tools

#### Using debug_uart.uf2
1. Flash debug program to Pico
2. Connect debug serial port (GPIO 0, 115200 baud rate)
3. Observe detailed data reception logs
4. Analyze key mapping and timing intervals

#### Debug Output Example
```
--- TTL Keyboard received data (Time: 91418 ms) ---
Byte count: 1
Hex data (1 byte): 65 
ASCII representation: e
Special character analysis:
  [0] Printable character 'e'
Valid data detected, proceeding with key parsing
Parsed key: e
--- TTL Keyboard data processing complete ---
```

## Development Guide

### Adding New Features

#### Extending Key Support
```cpp
// Add in ttl_keyboard.cpp's init_key_map()
key_map_[0xSpecificByte] = "NewKeyName";
```

#### Modifying Text Editor
```cpp
// Extend functionality in text_editor.cpp
void TextEditor::new_feature() {
    // Implement new feature
}
```

### Code Standards
- Use 4-space indentation
- Functions and variables use snake_case naming
- Class names use PascalCase naming
- Complete Doxygen documentation comments

### Commit Standards
```bash
git commit -m "feat: add new feature description"
git commit -m "fix: fix specific issue"
git commit -m "docs: update documentation content"
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) file for details.

## Changelog

### v2.1.0 (Current Version)
- ✅ Added ST7306 reflective LCD support
- ✅ Implemented adaptive layout for different screen sizes
- ✅ Added ST7306 test program with 7 comprehensive tests
- ✅ Optimized duplicate key filtering to 200ms threshold
- ✅ Fixed text editor content retention issue
- ✅ Corrected row-column coordinate system parameter order
- ✅ Unified baud rate to 115200
- ✅ Cleaned up compilation warnings and unused variables

### v2.0.0
- 🔄 Refactored to TTL keyboard input system
- ❌ Removed USB HID dependency
- ✅ Simplified to dual-mode interface
- ✅ Optimized RGB666 color system
- ✅ Improved error handling and status display

### v1.0.0
- 🎉 Initial version release
- ✅ Basic USB keyboard functionality
- ✅ ILI9488 display support

## Contributing

Contributions of any kind are welcome! Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.

### Ways to Contribute
- 🐛 Report Bugs
- 💡 Suggest New Features  
- 📝 Improve Documentation
- 🔧 Submit Code

---

**Project Repository**: [GitHub Repository]
**Technical Support**: Please submit issues through GitHub Issues
**Developers**: USB2TTL Pico Team 