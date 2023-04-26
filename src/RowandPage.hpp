#ifndef RowConvert_HPP
#define RowConvert_HPP

#include "RowStruction.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>


// 定义 一行Row如何紧凑存储
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// 定义一个连续的内存，用来存储一行数据
// 一行数据的大小为 ROW_SIZE
// 一行数据的内存布局为 id + username + email
using compactRow = std::array<std::byte, ROW_SIZE>;

const uint32_t PAGE_SIZE = 4096;      // 一页的大小为 4KB
const uint32_t TABLE_MAX_PAGES = 100; // 对于 array 来说，最大的页数
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

// 定义一个页的结构 页的大小为 4KB
// 按 ROWS_PER_PAGE 来划分
// 一次统一分配 4KB 的内存
using page = std::array<compactRow, ROWS_PER_PAGE>;

// 定义一个表的结构
using Table = struct Table {
    uint32_t num_rows{0}; // 记录表中的行数
    // 一个页表，每个元素是一个指针，指向一个页
    std::array<std::unique_ptr<page>, TABLE_MAX_PAGES> pages{nullptr};
} __attribute__((aligned(128)));

// 从 row_num 得到内存中读取或写入的地址
// row_num 为行号 从 0 开始
inline compactRow* row_slot(Table& table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    // 得到这个页的地址
    if (table.pages[page_num] == nullptr) {
        // 为这个页分配内存
        table.pages[page_num] = std::make_unique<page>();
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    auto* placeholder = &((*table.pages[page_num])[row_offset]);
    return placeholder;
}

// 将一行数据 序列化为 一块连续的内存
inline void serialize_row(Row& source, compactRow* destination) {
    // 将一行数据 序列化为 一块连续的内存
    // 一行数据的大小为 ROW_SIZE
    // 一行数据的内存布局为 id + username + email
    std::memcpy(destination->data() + ID_OFFSET, &(source.id), ID_SIZE);
    std::memcpy(destination->data() + USERNAME_OFFSET, source.username.data(),
                USERNAME_SIZE);
    std::memcpy(destination->data() + EMAIL_OFFSET, source.email.data(),
                EMAIL_SIZE);
}

inline void deserialize_row(compactRow* source, Row& destination) {
    // 将一块连续的内存 反序列化为 一行数据
    // 一行数据的大小为 ROW_SIZE
    // 一行数据的内存布局为 id + username + email
    std::memcpy(&(destination.id), source->data() + ID_OFFSET, ID_SIZE);
    std::memcpy(destination.username.data(), source->data() + USERNAME_OFFSET,
                USERNAME_SIZE);
    std::memcpy(destination.email.data(), source->data() + EMAIL_OFFSET,
                EMAIL_SIZE);
}

#endif // RowConvert_HPP