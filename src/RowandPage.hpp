#ifndef RowConvert_HPP
#define RowConvert_HPP

#include "RowStruction.hpp"
#include <algorithm>
#include <array>
#include <compare>
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

// page可能在内存中，也可能在磁盘中，增加一层抽象来泛指
using Pager = struct Pager {
    int file_descriptor{-1}; // 文件描述符
    uint32_t file_length{0}; // 文件长度
    // 一个页表，每个元素是一个指针，指向一个页
    std::array<std::unique_ptr<page>, TABLE_MAX_PAGES> pages{nullptr};
} __attribute__((aligned(128)));

// 定义一个表的结构
using Table = struct Table {
    uint32_t num_rows{0}; // 记录表中的行数
    std::unique_ptr<Pager> pager{nullptr};
} __attribute__((aligned(16)));

// 定义游标
using Cursor = struct Cursor {
    std::shared_ptr<Table> table{};
    uint32_t row_num{0};
    bool end_of_table{false};
} __attribute__((aligned(32)));

inline auto table_start(std::shared_ptr<Table>& table) {
    auto cursor = std::make_unique<Cursor>();
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);
    return cursor;
}

inline auto table_end(std::shared_ptr<Table>& table) {
    auto cursor = std::make_unique<Cursor>();
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;
    return cursor;
}

inline auto cursor_advance(std::unique_ptr<Cursor>& cursor) {
    cursor->row_num += 1;
    if (cursor->row_num >= cursor->table->num_rows) {
        cursor->end_of_table = true;
    }
}


// 需要判断在内存中还是磁盘
inline auto
get_page(std::unique_ptr<Pager>& pager, uint32_t& page_num) {
    if (page_num > TABLE_MAX_PAGES) {
        std::cout << "Tried to fetch page number out of bounds. "
                  << page_num << " > " << TABLE_MAX_PAGES << std::endl;
        _exit(EXIT_FAILURE);
    }
    if (pager->pages[page_num] == nullptr) {
        // 不在内存中，需要分配内存
        auto page_temp = std::make_unique<page>();
        // 确定文件中的页数
        uint32_t num_pages = (pager->file_length % PAGE_SIZE == 0) ? (pager->file_length / PAGE_SIZE) : ((pager->file_length / PAGE_SIZE) + 1);
        // 判断是否在磁盘中
        if (page_num <= num_pages) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            auto bytes_read = read(pager->file_descriptor, page_temp.get(), PAGE_SIZE);
            if (bytes_read == -1) {
                std::cout << "Error reading file: " << errno << std::endl;
                _exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = std::move(page_temp);
    }
}

// 从 row_num 得到内存中读取或写入的地址
// 由于存在 在内存中 和 在磁盘中 两种可能
inline compactRow* cursor_value(std::unique_ptr<Cursor>& cursor) {
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    get_page(cursor->table->pager, page_num);
    return cursor->table->pager->pages[page_num]->data() + (row_num % ROWS_PER_PAGE);
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