/**
 * @file ili9488_driver.cpp
 * @brief Modern C++ Implementation of ILI9488 Driver
 */

#include "ili9488_driver.hpp"
#include "ili9488_colors.hpp"
#include "ili9488_font.hpp"

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h" 
#include "hardware/pwm.h"
#include "hardware/dma.h"

namespace ili9488 {

// ILI9488 Commands
namespace Commands {
    constexpr uint8_t SWRESET = 0x01;
    constexpr uint8_t SLPOUT  = 0x11;
    constexpr uint8_t INVON   = 0x21;
    constexpr uint8_t DISPON  = 0x29;
    constexpr uint8_t CASET   = 0x2A;
    constexpr uint8_t PASET   = 0x2B;
    constexpr uint8_t RAMWR   = 0x2C;
    constexpr uint8_t MADCTL  = 0x36;
    constexpr uint8_t PIXFMT  = 0x3A;
    constexpr uint8_t PTLON   = 0x12;
    constexpr uint8_t PTLOFF  = 0x13;
    constexpr uint8_t PTLAR   = 0x30;
}

struct ILI9488Driver::Impl {
    // Hardware configuration
    spi_inst_t* spi_inst_;
    uint8_t pin_dc_;
    uint8_t pin_rst_;
    uint8_t pin_cs_;
    uint8_t pin_sck_;
    uint8_t pin_mosi_;
    uint8_t pin_bl_;
    uint32_t spi_speed_hz_;
    
    // Driver state
    bool is_initialized_ = false;
    Rotation current_rotation_ = Rotation::Portrait_0;
    FontLayout font_layout_ = FontLayout::Vertical;
    bool partial_mode_ = false;
    
    // DMA support
    int dma_channel_ = -1;
    volatile bool dma_busy_ = false;
    
    // Static instance pointer for DMA callback
    static Impl* dma_instance_;
    
    // Display dimensions (considering rotation)
    uint16_t display_width_ = LCD_WIDTH;
    uint16_t display_height_ = LCD_HEIGHT;
    
    // Constructor
    Impl(spi_inst_t* spi_inst, uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
         uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_bl, uint32_t spi_speed_hz)
        : spi_inst_(spi_inst), pin_dc_(pin_dc), pin_rst_(pin_rst), pin_cs_(pin_cs),
          pin_sck_(pin_sck), pin_mosi_(pin_mosi), pin_bl_(pin_bl), spi_speed_hz_(spi_speed_hz) {
    }
    
    // Hardware control methods
    void setCS(bool level) {
        gpio_put(pin_cs_, level ? 1 : 0);
    }
    
    void setDC(bool level) {
        gpio_put(pin_dc_, level ? 1 : 0);
    }
    
    void writeCommand(uint8_t cmd) {
        setCS(false);
        setDC(false);  // Command mode
        spi_write_blocking(spi_inst_, &cmd, 1);
        setCS(true);
    }
    
    void writeData(uint8_t data) {
        setCS(false);
        setDC(true);   // Data mode
        spi_write_blocking(spi_inst_, &data, 1);
        setCS(true);
    }
    
    void writeDataBuffer(const uint8_t* data, size_t length) {
        if (!data || length == 0) return;
        
        setCS(false);
        setDC(true);   // Data mode
        
        // Write in chunks for better performance
        size_t remaining = length;
        const uint8_t* ptr = data;
        
        while (remaining > 0) {
            size_t chunk_size = std::min(remaining, size_t(4096));
            spi_write_blocking(spi_inst_, ptr, chunk_size);
            ptr += chunk_size;
            remaining -= chunk_size;
        }
        
        setCS(true);
    }
    
    // DMA completion callback
    void dmaCompleteHandler() {
        setCS(true);
        dma_busy_ = false;
        dma_channel_acknowledge_irq0(dma_channel_);
    }
    
    // Static callback wrapper
    static void dmaCallback() {
        if (dma_instance_ && dma_instance_->dma_channel_ >= 0) {
            dma_instance_->dmaCompleteHandler();
        }
    }
    
    // Convert RGB565 to RGB666 bytes
    void rgb565ToRGB666Bytes(uint16_t color, uint8_t* bytes) {
        // 提取RGB565的各个分量
        uint8_t r5 = (color >> 11) & 0x1F;  // 5位红色 (0-31)
        uint8_t g6 = (color >> 5) & 0x3F;   // 6位绿色 (0-63)
        uint8_t b5 = color & 0x1F;          // 5位蓝色 (0-31)
        
        // 将5位扩展到8位：复制高位到低位以获得更好的精度
        uint8_t r8 = (r5 << 3) | (r5 >> 2);  // 5位→8位
        uint8_t g8 = (g6 << 2) | (g6 >> 4);  // 6位→8位
        uint8_t b8 = (b5 << 3) | (b5 >> 2);  // 5位→8位
        
        // ILI9488使用RGB666格式，每个分量6位，左对齐到8位
        bytes[0] = r8 & 0xFC;  // 保留高6位，清除低2位
        bytes[1] = g8 & 0xFC;  // 保留高6位，清除低2位
        bytes[2] = b8 & 0xFC;  // 保留高6位，清除低2位
    }
    
    // Convert RGB888 to RGB666 bytes
    void rgb888ToRGB666Bytes(uint32_t color, uint8_t* bytes) {
        // 提取RGB888的各个分量
        uint8_t r8 = (color >> 16) & 0xFF;  // 8位红色
        uint8_t g8 = (color >> 8) & 0xFF;   // 8位绿色
        uint8_t b8 = color & 0xFF;          // 8位蓝色
        
        // ILI9488使用RGB666格式，每个分量6位，左对齐到8位
        bytes[0] = r8 & 0xFC;  // 保留高6位，清除低2位
        bytes[1] = g8 & 0xFC;  // 保留高6位，清除低2位
        bytes[2] = b8 & 0xFC;  // 保留高6位，清除低2位
    }
    
    // Set drawing window
    void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        // Column address
        writeCommand(Commands::CASET);
        writeData(x0 >> 8);
        writeData(x0 & 0xFF);
        writeData(x1 >> 8);
        writeData(x1 & 0xFF);
        
        // Row address
        writeCommand(Commands::PASET);
        writeData(y0 >> 8);
        writeData(y0 & 0xFF);
        writeData(y1 >> 8);
        writeData(y1 & 0xFF);
        
        // Write to RAM
        writeCommand(Commands::RAMWR);
    }
    
    // Initialize hardware
    bool initializeHardware() {
        // Initialize SPI
        spi_init(spi_inst_, spi_speed_hz_);
        
        // Configure SPI pins
        gpio_set_function(pin_sck_, GPIO_FUNC_SPI);
        gpio_set_function(pin_mosi_, GPIO_FUNC_SPI);
        
        // Configure control pins
        gpio_init(pin_cs_);
        gpio_init(pin_dc_);
        gpio_init(pin_rst_);
        
        gpio_set_dir(pin_cs_, GPIO_OUT);
        gpio_set_dir(pin_dc_, GPIO_OUT);
        gpio_set_dir(pin_rst_, GPIO_OUT);
        
        gpio_put(pin_cs_, 1);    // CS high (inactive)
        gpio_put(pin_dc_, 1);    // DC high (data mode)
        gpio_put(pin_rst_, 1);   // Reset high (inactive)
        
        // Configure backlight with PWM
        if (pin_bl_ != 255) {  // Valid pin
            gpio_set_function(pin_bl_, GPIO_FUNC_PWM);
            uint slice_num = pwm_gpio_to_slice_num(pin_bl_);
            uint channel = pwm_gpio_to_channel(pin_bl_);
            
            pwm_config config = pwm_get_default_config();
            pwm_config_set_clkdiv(&config, 4.0f);
            pwm_config_set_wrap(&config, 255);
            pwm_init(slice_num, &config, true);
            
            pwm_set_chan_level(slice_num, channel, 255);
        }
        
        return true;
    }
    
    // Hardware reset
    void hardwareReset() {
        gpio_put(pin_rst_, 1);
        sleep_ms(10);
        gpio_put(pin_rst_, 0);
        sleep_ms(10);
        gpio_put(pin_rst_, 1);
        sleep_ms(150);
    }
    
    // Initialization sequence
    void initializationSequence() {
        // Software reset
        writeCommand(Commands::SWRESET);
        sleep_ms(200);
        
        // Exit sleep mode
        writeCommand(Commands::SLPOUT);
        sleep_ms(200);
        
        // Memory access control
        writeCommand(Commands::MADCTL);
        writeData(0x48);
        
        // Pixel format (18-bit RGB666)
        writeCommand(Commands::PIXFMT);
        writeData(0x66);
        
        // VCOM control
        writeCommand(0xC5);
        writeData(0x00);
        writeData(0x36);
        writeData(0x80);
        
        // Power control
        writeCommand(0xC2);
        writeData(0xA7);
        
        // Positive gamma correction
        writeCommand(0xE0);
        const uint8_t gamma_pos[] = {
            0xF0, 0x01, 0x06, 0x0F, 0x12, 0x1D, 0x36, 0x54,
            0x44, 0x0C, 0x18, 0x16, 0x13, 0x15
        };
        for (uint8_t data : gamma_pos) {
            writeData(data);
        }
        
        // Negative gamma correction
        writeCommand(0xE1);
        const uint8_t gamma_neg[] = {
            0xF0, 0x01, 0x05, 0x0A, 0x0B, 0x07, 0x32, 0x44,
            0x44, 0x0C, 0x18, 0x17, 0x13, 0x16
        };
        for (uint8_t data : gamma_neg) {
            writeData(data);
        }
        
        // Invert display
        writeCommand(Commands::INVON);
        
        // Turn display on
        writeCommand(Commands::DISPON);
        sleep_ms(50);
    }
    
    // Update display dimensions based on rotation
    void updateDimensions() {
        switch (current_rotation_) {
            case Rotation::Portrait_0:
            case Rotation::Portrait_180:
                display_width_ = LCD_WIDTH;
                display_height_ = LCD_HEIGHT;
                break;
            case Rotation::Landscape_90:
            case Rotation::Landscape_270:
                display_width_ = LCD_HEIGHT;
                display_height_ = LCD_WIDTH;
                break;
        }
    }
    
    // Initialize DMA
    void initializeDMA() {
        dma_channel_ = dma_claim_unused_channel(false);
        if (dma_channel_ >= 0) {
            dma_instance_ = this;  // Set static instance pointer
            dma_channel_set_irq0_enabled(dma_channel_, true);
            irq_set_exclusive_handler(DMA_IRQ_0, dmaCallback);
            irq_set_enabled(DMA_IRQ_0, true);
        }
    }
};

// Static member definition
ILI9488Driver::Impl* ILI9488Driver::Impl::dma_instance_ = nullptr;

// Constructor
ILI9488Driver::ILI9488Driver(spi_inst_t* spi_inst, uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
                             uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_bl, uint32_t spi_speed_hz)
    : pImpl_(std::make_unique<Impl>(spi_inst, pin_dc, pin_rst, pin_cs, pin_sck, pin_mosi, pin_bl, spi_speed_hz)) {
}

// Destructor
ILI9488Driver::~ILI9488Driver() = default;

// Initialize the display
bool ILI9488Driver::initialize() {
    if (pImpl_->is_initialized_) {
        return true;
    }
    
    printf("Initializing ILI9488 Modern C++ Driver...\n");
    
    if (!pImpl_->initializeHardware()) {
        printf("Hardware initialization failed!\n");
        return false;
    }
    
    pImpl_->hardwareReset();
    pImpl_->initializationSequence();
    pImpl_->initializeDMA();
    
    setRotation(pImpl_->current_rotation_);
    
    pImpl_->is_initialized_ = true;
    printf("ILI9488 initialization completed successfully!\n");
    
    return true;
}

// Reset the display hardware
void ILI9488Driver::reset() {
    pImpl_->hardwareReset();
}

// Clear the display buffer
void ILI9488Driver::clear() {
    fillScreen(ili9488_colors::rgb565::BLACK);
}

// Clear the display and fill with color
void ILI9488Driver::clearDisplay() {
    clear();
}

// Draw a single pixel (RGB565)
void ILI9488Driver::drawPixel(uint16_t x, uint16_t y, uint16_t color565) {
    if (x >= pImpl_->display_width_ || y >= pImpl_->display_height_) {
        return;
    }
    
    pImpl_->setWindow(x, y, x, y);
    
    uint8_t rgb666_bytes[3];
    pImpl_->rgb565ToRGB666Bytes(color565, rgb666_bytes);
    pImpl_->writeDataBuffer(rgb666_bytes, 3);
}

// Draw a single pixel (RGB888/24-bit)
void ILI9488Driver::drawPixelRGB24(uint16_t x, uint16_t y, uint32_t color24) {
    if (x >= pImpl_->display_width_ || y >= pImpl_->display_height_) {
        return;
    }
    
    pImpl_->setWindow(x, y, x, y);
    
    uint8_t rgb666_bytes[3];
    pImpl_->rgb888ToRGB666Bytes(color24, rgb666_bytes);
    pImpl_->writeDataBuffer(rgb666_bytes, 3);
}

// Draw a single pixel (RGB666/18-bit native)
void ILI9488Driver::drawPixelRGB666(uint16_t x, uint16_t y, uint32_t color666) {
    drawPixelRGB24(x, y, ili9488_colors::rgb666_to_rgb888(color666));
}

// Write multiple pixels (RGB565)
void ILI9488Driver::writePixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                                const uint16_t* colors, size_t count) {
    if (!colors || count == 0) return;
    
    pImpl_->setWindow(x0, y0, x1, y1);
    
    // Convert and send in batches
    constexpr size_t BATCH_SIZE = 256;
    uint8_t batch_buffer[BATCH_SIZE * 3];
    
    size_t remaining = count;
    const uint16_t* color_ptr = colors;
    
    while (remaining > 0) {
        size_t batch_count = std::min(remaining, BATCH_SIZE);
        
        for (size_t i = 0; i < batch_count; ++i) {
            pImpl_->rgb565ToRGB666Bytes(color_ptr[i], &batch_buffer[i * 3]);
        }
        
        pImpl_->writeDataBuffer(batch_buffer, batch_count * 3);
        
        color_ptr += batch_count;
        remaining -= batch_count;
    }
}

// Fill rectangular area (RGB565)
void ILI9488Driver::fillArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    if (x0 > x1 || y0 > y1) return;
    
    pImpl_->setWindow(x0, y0, x1, y1);
    
    uint8_t rgb666_bytes[3];
    pImpl_->rgb565ToRGB666Bytes(color, rgb666_bytes);
    
    uint32_t pixel_count = (x1 - x0 + 1) * (y1 - y0 + 1);
    
    for (uint32_t i = 0; i < pixel_count; ++i) {
        pImpl_->writeDataBuffer(rgb666_bytes, 3);
    }
}

// Fill rectangular area (RGB666 native - no conversion needed)
void ILI9488Driver::fillAreaRGB666(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color666) {
    if (x0 > x1 || y0 > y1) return;
    
    pImpl_->setWindow(x0, y0, x1, y1);
    
    // 直接使用RGB666格式，无需转换
    uint8_t rgb666_bytes[3];
    rgb666_bytes[0] = (color666 >> 16) & 0xFC;  // 红色分量，保留高6位
    rgb666_bytes[1] = (color666 >> 8) & 0xFC;   // 绿色分量，保留高6位
    rgb666_bytes[2] = color666 & 0xFC;          // 蓝色分量，保留高6位
    
    uint32_t pixel_count = (x1 - x0 + 1) * (y1 - y0 + 1);
    
    for (uint32_t i = 0; i < pixel_count; ++i) {
        pImpl_->writeDataBuffer(rgb666_bytes, 3);
    }
}

// Fill entire screen (RGB565)
void ILI9488Driver::fillScreen(uint16_t color) {
    fillArea(0, 0, pImpl_->display_width_ - 1, pImpl_->display_height_ - 1, color);
}

// Fill entire screen (RGB666 native)
void ILI9488Driver::fillScreenRGB666(uint32_t color666) {
    fillAreaRGB666(0, 0, pImpl_->display_width_ - 1, pImpl_->display_height_ - 1, color666);
}

// Set display rotation
void ILI9488Driver::setRotation(Rotation rotation) {
    pImpl_->current_rotation_ = rotation;
    pImpl_->updateDimensions();
    
    uint8_t madctl_value = 0x48;  // Base value
    
    switch (rotation) {
        case Rotation::Portrait_0:
            madctl_value = 0x48;
            break;
        case Rotation::Landscape_90:
            madctl_value = 0x28;
            break;
        case Rotation::Portrait_180:
            madctl_value = 0x88;
            break;
        case Rotation::Landscape_270:
            madctl_value = 0xE8;
            break;
    }
    
    pImpl_->writeCommand(Commands::MADCTL);
    pImpl_->writeData(madctl_value);
}

// Get current rotation
Rotation ILI9488Driver::getRotation() const {
    return pImpl_->current_rotation_;
}

// Control backlight on/off
void ILI9488Driver::setBacklight(bool enable) {
    setBacklightBrightness(enable ? 255 : 0);
}

// Set backlight brightness (0-255)
void ILI9488Driver::setBacklightBrightness(uint8_t brightness) {
    if (pImpl_->pin_bl_ == 255) return;  // Invalid pin
    
    uint slice_num = pwm_gpio_to_slice_num(pImpl_->pin_bl_);
    uint channel = pwm_gpio_to_channel(pImpl_->pin_bl_);
    pwm_set_chan_level(slice_num, channel, brightness);
}

// Enable/disable partial display mode
void ILI9488Driver::setPartialMode(bool enable) {
    pImpl_->partial_mode_ = enable;
    pImpl_->writeCommand(enable ? Commands::PTLON : Commands::PTLOFF);
}

// Set partial display area
void ILI9488Driver::setPartialArea(uint16_t /* x0 */, uint16_t y0, uint16_t /* x1 */, uint16_t y1) {
    pImpl_->writeCommand(Commands::PTLAR);
    pImpl_->writeData(y0 >> 8);
    pImpl_->writeData(y0 & 0xFF);
    pImpl_->writeData(y1 >> 8);
    pImpl_->writeData(y1 & 0xFF);
}

// Write data using DMA (non-blocking)
bool ILI9488Driver::writeDMA(const uint8_t* data, size_t length) {
    if (!data || length == 0 || pImpl_->dma_channel_ < 0 || pImpl_->dma_busy_) {
        return false;
    }
    
    pImpl_->dma_busy_ = true;
    
    // Configure DMA transfer
    dma_channel_config config = dma_channel_get_default_config(pImpl_->dma_channel_);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(pImpl_->spi_inst_, true));
    
    pImpl_->setCS(false);
    pImpl_->setDC(true);
    
    dma_channel_configure(
        pImpl_->dma_channel_,
        &config,
        &spi_get_hw(pImpl_->spi_inst_)->dr,
        data,
        length,
        true
    );
    
    return true;
}

// Check if DMA transfer is busy
bool ILI9488Driver::isDMABusy() const {
    return pImpl_->dma_busy_;
}

// Wait for DMA transfer to complete
void ILI9488Driver::waitDMAComplete() {
    while (pImpl_->dma_busy_) {
        tight_loop_contents();
    }
}

// Get display width (considering rotation)
uint16_t ILI9488Driver::getWidth() const {
    return pImpl_->display_width_;
}

// Get display height (considering rotation)
uint16_t ILI9488Driver::getHeight() const {
    return pImpl_->display_height_;
}

// Check if coordinates are within display bounds
bool ILI9488Driver::isValidCoordinate(uint16_t x, uint16_t y) const {
    return (x < pImpl_->display_width_ && y < pImpl_->display_height_);
}

// Set font layout
void ILI9488Driver::setFontLayout(FontLayout layout) {
    pImpl_->font_layout_ = layout;
}

// Get current font layout
FontLayout ILI9488Driver::getFontLayout() const {
    return pImpl_->font_layout_;
}

// Get string width in pixels
uint16_t ILI9488Driver::getStringWidth(std::string_view str) const {
    return static_cast<uint16_t>(str.length() * font::FONT_WIDTH);
}

// Draw a character
void ILI9488Driver::drawChar(uint16_t x, uint16_t y, char c, uint32_t color, uint32_t bg_color) {
    using namespace font;
    
    const uint8_t* char_data = get_char_data(c);
    
    for (uint8_t row = 0; row < FONT_HEIGHT; ++row) {
        uint8_t byte = char_data[row];
        for (uint8_t col = 0; col < FONT_WIDTH; ++col) {
            bool pixel_is_set = (byte >> (7 - col)) & 0x01;
            if (pixel_is_set) {
                drawPixelRGB24(x + col, y + row, color);
            } else {
                drawPixelRGB24(x + col, y + row, bg_color);
            }
        }
    }
}

// Draw a string (C-style)
void ILI9488Driver::drawString(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color) {
    drawString(x, y, std::string_view(str), color, bg_color);
}

// Draw a string (string_view)
void ILI9488Driver::drawString(uint16_t x, uint16_t y, std::string_view str, uint32_t color, uint32_t bg_color) {
    using namespace font;
    
    uint16_t current_x = x;
    
    for (char c : str) {
        if (c >= 32 && c <= 126) {  // Printable ASCII
            drawChar(current_x, y, c, color, bg_color);
        }
        current_x += FONT_WIDTH;
        
        // Break if we exceed display width
        if (current_x >= pImpl_->display_width_) {
            break;
        }
    }
}

} // namespace ili9488 