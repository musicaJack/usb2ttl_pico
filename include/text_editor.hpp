#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "display_driver.hpp"
// 使用ILI9488颜色系统
#include "ili9488/ili9488_colors.hpp"

namespace usb2ttl {

/**
 * @brief 文本编辑器类
 * @details 提供基本的文本编辑功能，包括字符输入、删除、保存和加载等
 * 使用ILI9488的RGB666颜色系统
 */
class TextEditor {
public:
    /**
     * @brief 构造函数
     * @param display 显示驱动共享指针
     */
    explicit TextEditor(std::shared_ptr<DisplayDriver> display);
    
    /**
     * @brief 析构函数
     */
    ~TextEditor();
    
    // 禁用拷贝构造和赋值操作
    TextEditor(const TextEditor&) = delete;
    TextEditor& operator=(const TextEditor&) = delete;
    
    // 允许移动构造和赋值
    TextEditor(TextEditor&&) = default;
    TextEditor& operator=(TextEditor&&) = default;
    
    /**
     * @brief 初始化编辑器
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 清屏并重置编辑器状态
     */
    void clear_screen();
    
    /**
     * @brief 字符输入处理
     * @param ch 输入的字符
     */
    void insert_char(char ch);
    
    /**
     * @brief 控制键处理
     * @param key 控制键名称
     */
    void handle_control_key(const std::string& key);
    
    /**
     * @brief 换行操作
     */
    void newline();
    
    /**
     * @brief 退格操作
     */
    void backspace();
    
    /**
     * @brief 保存到文件
     * @param filename 文件名，默认为"saved1.txt"
     * @return true 保存成功，false 保存失败
     */
    bool save_to_file(const std::string& filename = "saved1.txt");
    
    /**
     * @brief 从文件加载
     * @param filename 文件名
     * @return true 加载成功，false 加载失败
     */
    bool load_from_file(const std::string& filename);
    
    /**
     * @brief 刷新显示
     */
    void refresh_display();
    
    /**
     * @brief 获取当前光标位置
     * @return 光标位置对 (行, 列)
     */
    std::pair<int, int> get_cursor_position() const;
    
    /**
     * @brief 设置光标位置
     * @param row 行号
     * @param col 列号
     */
    void set_cursor_position(int row, int col);
    
    /**
     * @brief 获取文本内容
     * @return 文本行向量的常量引用
     */
    const std::vector<std::string>& get_lines() const;
    
    /**
     * @brief 获取当前行内容
     * @return 当前行的字符串
     */
    std::string get_current_line() const;
    
    /**
     * @brief 检查是否有未保存的更改
     * @return true 有未保存更改，false 无未保存更改
     */
    bool has_unsaved_changes() const;
    
    /**
     * @brief 检查输入是否被冻结
     * @return true 输入被冻结，false 输入正常
     */
    bool is_input_frozen() const;
    
    /**
     * @brief 获取状态信息
     * @return 状态信息字符串
     */
    std::string get_status_info() const;

private:
    // 私有成员变量
    std::vector<std::string> lines_;           ///< 文本缓冲区
    int cursor_row_;                           ///< 光标行
    int cursor_col_;                           ///< 光标列
    bool insert_mode_;                         ///< 插入模式
    int max_lines_;                            ///< 最大行数
    int max_length_;                           ///< 每行最大字符数
    int last_updated_row_;                     ///< 最后更新的行
    bool unsaved_changes_;                     ///< 未保存更改标志
    bool input_frozen_;                        ///< 输入冻结标志
    
    std::shared_ptr<DisplayDriver> display_;  ///< 显示驱动
    
    // 私有方法
    void refresh_line(int line_num);
    void refresh_all_lines();
    void draw_char_at_position(char ch, int row, int col);
    bool is_valid_position(int row, int col) const;
    void ensure_line_exists(int row);
    void update_display_line(int line_num);
    void check_and_freeze_input();
};

} // namespace usb2ttl 