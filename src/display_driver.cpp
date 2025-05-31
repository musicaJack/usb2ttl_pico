/**
 * @file display_driver.cpp
 * @brief 显示驱动基类实现
 */

#include "display_driver.hpp"

namespace usb2ttl {

DisplayDriver::DisplayDriver() 
    : width_(0)
    , height_(0)
    , font_width_(8)
    , font_height_(16)
    , text_offset_x_(0)
    , text_offset_y_(0) {
}

DisplayDriver::~DisplayDriver() = default;

std::pair<int, int> DisplayDriver::calculate_text_position(int col, int row) const {
    int x = text_offset_x_ + col * font_width_;
    int y = text_offset_y_ + row * font_height_;
    return std::make_pair(x, y);
}

int DisplayDriver::get_max_text_cols() const {
    return (width_ - 2 * text_offset_x_) / font_width_;
}

int DisplayDriver::get_max_text_rows() const {
    return (height_ - 2 * text_offset_y_) / font_height_;
}

bool DisplayDriver::is_valid_text_position(int col, int row) const {
    return col >= 0 && col < get_max_text_cols() && 
           row >= 0 && row < get_max_text_rows();
}

} // namespace usb2ttl 