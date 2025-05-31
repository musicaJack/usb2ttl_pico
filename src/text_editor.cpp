#include "text_editor.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>

namespace usb2ttl {

TextEditor::TextEditor(std::shared_ptr<DisplayDriver> display)
    : lines_({""}), cursor_row_(0), cursor_col_(0), insert_mode_(true),
      max_lines_(80), last_updated_row_(-1),
      unsaved_changes_(false), input_frozen_(false), display_(display) {
    // 动态计算每行最大字符数
    if (display_) {
        int available_width = display_->get_width() - 2 * 5; // 减去左右边距
        max_length_ = available_width / display_->get_font_width();
        // 确保至少有10个字符的空间，但不超过38
        if (max_length_ < 10) {
            max_length_ = 10;
        } else if (max_length_ > 38) {
            max_length_ = 38;
        }
    } else {
        max_length_ = 38; // 默认值
    }
}

TextEditor::~TextEditor() = default;

bool TextEditor::initialize() {
    if (!display_ || !display_->initialize()) {
        return false;
    }
    
    clear_screen();
    return true;
}

void TextEditor::clear_screen() {
    display_->clear_screen();
    lines_.clear();
    lines_.push_back("");
    cursor_row_ = 0;
    cursor_col_ = 0;
    last_updated_row_ = -1;
    unsaved_changes_ = false;
    input_frozen_ = false;
    refresh_display();
}

void TextEditor::insert_char(char ch) {
    // 检查输入是否被冻结
    if (input_frozen_) {
        return;
    }
    
    if (ch == '\n') {
        newline();
        return;
    }
    
    ensure_line_exists(cursor_row_);
    std::string& line = lines_[cursor_row_];
    
    // 统一处理所有字符（包括空格）
    if (insert_mode_) {
        // 检查是否需要自动换行
        if (cursor_col_ >= max_length_) {
            // 自动换行：创建新行并将字符插入到新行
            if (lines_.size() < static_cast<size_t>(max_lines_)) {
                lines_.insert(lines_.begin() + cursor_row_ + 1, "");
                cursor_row_++;
                cursor_col_ = 0;
                
                // 在新行插入字符
                lines_[cursor_row_].insert(0, 1, ch);
                cursor_col_ = 1;
                
                // 检查是否需要冻结输入
                check_and_freeze_input();
                
                // 局部刷新：只绘制新字符
                draw_char_at_position(ch, cursor_row_, 0);
                unsaved_changes_ = true;
                return;
            } else {
                // 已达到最大行数，冻结输入
                input_frozen_ = true;
                return;
            }
        }
        
        // 正常插入字符
        line.insert(cursor_col_, 1, ch);
        
        // 局部刷新：只绘制新字符和后续字符
        auto pos = display_->calculate_text_position(cursor_col_, cursor_row_);
        
        // 清除从当前位置到行末的区域
        int remaining_width = (max_length_ - cursor_col_) * display_->get_font_width();
        display_->fill_rect(pos.first, pos.second, 
                           remaining_width, 
                           display_->get_font_height(), 
                           ili9488_colors::rgb666::BLACK);
        
        // 重新绘制从当前位置到行末的文本
        std::string remaining_text = line.substr(cursor_col_);
        if (!remaining_text.empty()) {
            display_->draw_text(remaining_text, pos.first, pos.second);
        }
        
        cursor_col_++;
        
        // 检查是否需要冻结输入
        check_and_freeze_input();
    }
    
    unsaved_changes_ = true;
}

void TextEditor::handle_control_key(const std::string& key) {
    if (key == "Enter") {
        newline();
    } else if (key == "Backspace") {
        backspace();
    } else if (key == "space") {
        insert_char(' ');
    } else if (key == "Tab") {
        for (int i = 0; i < 4; ++i) {
            insert_char(' ');
        }
    } else if (key == "ESC") {
        std::cout << "Clearing screen and buffer..." << std::endl;
        clear_screen();
    } else if (key == "F10") {
        std::cout << "F10 pressed, saving to file..." << std::endl;
        save_to_file();
    }
}

void TextEditor::newline() {
    // 检查输入是否被冻结
    if (input_frozen_) {
        return;
    }
    
    if (lines_.size() >= static_cast<size_t>(max_lines_)) {
        input_frozen_ = true;
        return;
    }
    
    ensure_line_exists(cursor_row_);
    std::string& current_line = lines_[cursor_row_];
    
    // 将光标后的内容移到新行
    std::string remaining = current_line.substr(cursor_col_);
    current_line = current_line.substr(0, cursor_col_);
    
    // 插入新行
    lines_.insert(lines_.begin() + cursor_row_ + 1, remaining);
    cursor_row_++;
    cursor_col_ = 0;
    
    // 检查是否需要冻结输入
    check_and_freeze_input();
    
    // 局部刷新：重新绘制当前行和新行
    refresh_line(cursor_row_ - 1); // 刷新上一行
    refresh_line(cursor_row_);     // 刷新当前行
    
    unsaved_changes_ = true;
}

void TextEditor::backspace() {
    if (cursor_col_ > 0) {
        ensure_line_exists(cursor_row_);
        std::string& line = lines_[cursor_row_];
        line.erase(cursor_col_ - 1, 1);
        cursor_col_--;
        last_updated_row_ = cursor_row_;
        unsaved_changes_ = true;
        refresh_line(cursor_row_);
    } else if (cursor_row_ > 0) {
        // 合并到上一行
        std::string current_line = lines_[cursor_row_];
        lines_.erase(lines_.begin() + cursor_row_);
        cursor_row_--;
        cursor_col_ = lines_[cursor_row_].length();
        lines_[cursor_row_] += current_line;
        last_updated_row_ = -1; // 需要刷新所有行
        unsaved_changes_ = true;
        refresh_display();
    }
}

bool TextEditor::save_to_file(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& line : lines_) {
        file << line << '\n';
    }
    
    file.close();
    unsaved_changes_ = false;
    std::cout << "Content saved to " << filename << std::endl;
    return true;
}

bool TextEditor::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    lines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        lines_.push_back(line);
    }
    
    if (lines_.empty()) {
        lines_.push_back("");
    }
    
    cursor_row_ = 0;
    cursor_col_ = 0;
    last_updated_row_ = -1;
    unsaved_changes_ = false;
    refresh_display();
    
    file.close();
    return true;
}

void TextEditor::refresh_display() {
    if (last_updated_row_ >= 0) {
        refresh_line(last_updated_row_);
    } else {
        refresh_all_lines();
    }
}

std::pair<int, int> TextEditor::get_cursor_position() const {
    return {cursor_row_, cursor_col_};
}

void TextEditor::set_cursor_position(int row, int col) {
    cursor_row_ = std::max(0, std::min(row, static_cast<int>(lines_.size()) - 1));
    ensure_line_exists(cursor_row_);
    cursor_col_ = std::max(0, std::min(col, static_cast<int>(lines_[cursor_row_].length())));
}

const std::vector<std::string>& TextEditor::get_lines() const {
    return lines_;
}

std::string TextEditor::get_current_line() const {
    if (cursor_row_ >= 0 && cursor_row_ < static_cast<int>(lines_.size())) {
        return lines_[cursor_row_];
    }
    return "";
}

bool TextEditor::has_unsaved_changes() const {
    return unsaved_changes_;
}

void TextEditor::refresh_line(int line_num) {
    if (line_num < 0 || line_num >= static_cast<int>(lines_.size())) {
        return;
    }
    
    auto pos = display_->calculate_text_position(0, line_num);
    
    // 计算需要清除的宽度：最大行宽度 + 一个字符的宽度（用于清除光标位置）
    int max_text_width = max_length_ * display_->get_font_width();
    
    // 清除整行的最大可能宽度，但不超过显示器边界
    int available_width = display_->get_width() - pos.first;
    int clear_width = std::min(max_text_width + display_->get_font_width(), available_width);
    
    display_->fill_rect(pos.first, pos.second, 
                       clear_width, 
                       display_->get_font_height(), 
                       ili9488_colors::rgb666::BLACK);
    
    // 只有在有文本时才绘制
    if (!lines_[line_num].empty()) {
        display_->draw_text(lines_[line_num], pos.first, pos.second);
    }
}

void TextEditor::refresh_all_lines() {
    display_->clear_screen();
    
    for (int i = 0; i < static_cast<int>(lines_.size()) && i < max_lines_; ++i) {
        auto pos = display_->calculate_text_position(0, i);
        display_->draw_text(lines_[i], pos.first, pos.second);
    }
}

bool TextEditor::is_valid_position(int row, int col) const {
    return row >= 0 && row < static_cast<int>(lines_.size()) &&
           col >= 0 && col <= static_cast<int>(lines_[row].length());
}

void TextEditor::ensure_line_exists(int row) {
    while (static_cast<int>(lines_.size()) <= row) {
        lines_.push_back("");
    }
}

void TextEditor::update_display_line(int line_num) {
    refresh_line(line_num);
}

void TextEditor::check_and_freeze_input() {
    if (lines_.size() >= static_cast<size_t>(max_lines_)) {
        input_frozen_ = true;
    }
}

void TextEditor::draw_char_at_position(char ch, int row, int col) {
    if (row < 0 || row >= static_cast<int>(lines_.size()) || col < 0) {
        return;
    }
    
    auto pos = display_->calculate_text_position(col, row);
    
    // 清除字符位置
    display_->fill_rect(pos.first, pos.second, 
                       display_->get_font_width(), 
                       display_->get_font_height(), 
                       ili9488_colors::rgb666::BLACK);
    
    // 绘制字符
    std::string char_str(1, ch);
    display_->draw_text(char_str, pos.first, pos.second);
}

bool TextEditor::is_input_frozen() const {
    return input_frozen_;
}

std::string TextEditor::get_status_info() const {
    if (input_frozen_) {
        return "INPUT FROZEN - Max 80 lines reached!";
    }
    
    char status[64];
    snprintf(status, sizeof(status), "Lines: %d/%d, Chars: %d/%d", 
             static_cast<int>(lines_.size()), max_lines_,
             cursor_col_, max_length_);
    return std::string(status);
}

}  // namespace usb2ttl 