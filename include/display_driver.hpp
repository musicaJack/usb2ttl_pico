#pragma once

#include <string>
#include <cstdint>
#include <utility>
// 使用ILI9488的颜色系统
#include "ili9488/ili9488_colors.hpp"

namespace usb2ttl {

/**
 * @brief 显示驱动抽象基类
 * @details 定义了显示设备的通用接口，支持各种显示器驱动的统一管理
 * 使用ILI9488的RGB666颜色系统作为标准
 */
class DisplayDriver {
public:
    /**
     * @brief 构造函数
     */
    DisplayDriver();
    
    /**
     * @brief 虚析构函数
     */
    virtual ~DisplayDriver();
    
    // 禁用拷贝构造和赋值操作
    DisplayDriver(const DisplayDriver&) = delete;
    DisplayDriver& operator=(const DisplayDriver&) = delete;
    
    // 允许移动构造和赋值
    DisplayDriver(DisplayDriver&&) = default;
    DisplayDriver& operator=(DisplayDriver&&) = default;
    
    /**
     * @brief 初始化显示器
     * @return true 初始化成功，false 初始化失败
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 清屏
     * @param color 清屏颜色，默认为黑色 (RGB666格式)
     */
    virtual void clear_screen(std::uint32_t color = ili9488_colors::rgb666::BLACK) = 0;
    
    /**
     * @brief 填充矩形区域
     * @param x 起始X坐标
     * @param y 起始Y坐标
     * @param width 矩形宽度
     * @param height 矩形高度
     * @param color 填充颜色 (RGB666格式)
     */
    virtual void fill_rect(int x, int y, int width, int height, std::uint32_t color) = 0;
    
    /**
     * @brief 显示文本
     * @param text 要显示的文本
     * @param x X坐标
     * @param y Y坐标
     * @param fg_color 前景色，默认为白色 (RGB666格式)
     * @param bg_color 背景色，默认为黑色 (RGB666格式)
     */
    virtual void draw_text(const std::string& text, int x, int y, 
                          std::uint32_t fg_color = ili9488_colors::rgb666::WHITE, 
                          std::uint32_t bg_color = ili9488_colors::rgb666::BLACK) = 0;
    
    /**
     * @brief 获取显示器宽度
     * @return 显示器宽度（像素）
     */
    virtual int get_width() const = 0;
    
    /**
     * @brief 获取显示器高度
     * @return 显示器高度（像素）
     */
    virtual int get_height() const = 0;
    
    /**
     * @brief 获取字体宽度
     * @return 字体宽度（像素）
     */
    virtual int get_font_width() const = 0;
    
    /**
     * @brief 获取字体高度
     * @return 字体高度（像素）
     */
    virtual int get_font_height() const = 0;
    
    /**
     * @brief 设置背光亮度
     * @param brightness 亮度值 (0.0 - 1.0)
     */
    virtual void set_backlight(float brightness) = 0;
    
    /**
     * @brief 刷新显示
     */
    virtual void refresh() = 0;
    
    /**
     * @brief 计算文本显示位置
     * @param col 列号
     * @param row 行号
     * @return 像素坐标对 (x, y)
     */
    std::pair<int, int> calculate_text_position(int col, int row) const;
    
    /**
     * @brief 获取最大文本列数
     * @return 最大列数
     */
    int get_max_text_cols() const;
    
    /**
     * @brief 获取最大文本行数
     * @return 最大行数
     */
    int get_max_text_rows() const;
    
    /**
     * @brief 检查文本位置是否有效
     * @param col 列号
     * @param row 行号
     * @return true 位置有效，false 位置无效
     */
    bool is_valid_text_position(int col, int row) const;

protected:
    // 显示器参数
    int width_;           ///< 显示器宽度
    int height_;          ///< 显示器高度
    int font_width_;      ///< 字体宽度
    int font_height_;     ///< 字体高度
    int text_offset_x_;   ///< 文本X偏移
    int text_offset_y_;   ///< 文本Y偏移
};

} // namespace usb2ttl 