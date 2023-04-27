#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "src/RowStruction.hpp"
#include "src/RowandPage.hpp"
#include "src/TypeEnum.hpp"
#include "utils.hpp"

using InputBuffer = std::string;

// 执行的命令类型
using Statement = struct {
    StatementType type;
    Row row_to_insert; // only used by insert statement
} __attribute__((aligned(128)));

// 判断输入命令是否合法
PrepareResult
prepare_statement(std::unique_ptr<InputBuffer>& input_buffer,
                  Statement& statement) {
    if ((*input_buffer).substr(0, 6) == "insert") {
        statement.type = STATEMENT_INSERT;
        return sscanf_command(*input_buffer, statement.row_to_insert);
    }
    if (*input_buffer == "select") {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// 初始化 状态缓冲区：与 getline() 交互
auto new_input_buffer() -> std::unique_ptr<InputBuffer> {
    auto input_buffer = std::make_unique<InputBuffer>();
    return input_buffer;
}

void close_input_buffer(std::unique_ptr<InputBuffer>& input_buffer) {
    input_buffer.reset();
}

// 初始化 Pager
auto pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        std::cout << "Unable to open file" << std::endl;
        _exit(EXIT_FAILURE);
    }
    off_t file_length = lseek(fd, 0, SEEK_END);
    auto pager = std::make_unique<Pager>();
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    for (auto& page : pager->pages) {
        page = nullptr;
    }
    return pager;
}

// 初始化 table 从文件中读取数据
auto db_open(const char* filename) {
    auto pager = pager_open(filename);
    // 记录文件中的行数
    uint32_t num_rows = pager->file_length / ROW_SIZE;
    auto table = std::make_unique<Table>();
    table->pager = std::move(pager);
    table->num_rows = num_rows;
    return table;
}

// 固化数据
auto pager_flush(std::unique_ptr<Pager>& pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == nullptr) {
        std::cout << "Tried to flush null page" << std::endl;
        _exit(EXIT_FAILURE);
    }
    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        std::cout << "Error seeking: " << errno << std::endl;
        _exit(EXIT_FAILURE);
    }
    ssize_t bytes_written =
        write(pager->file_descriptor, pager->pages[page_num].get(), size);
    if (bytes_written == -1) {
        std::cout << "Error writing: " << errno << std::endl;
        _exit(EXIT_FAILURE);
    }
}

// 关闭table 固化数据
auto db_close(std::unique_ptr<Table>& table) {
    // 计算文件中的页数
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < num_full_pages; ++i) {
        if (table->pager->pages[i] == nullptr) {
            continue;
        }
        pager_flush(table->pager, i, PAGE_SIZE);
        table->pager->pages[i].reset();
    }
    // 有多余的行数
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (table->pager->pages[page_num] != nullptr) {
            pager_flush(table->pager, page_num, num_additional_rows * ROW_SIZE);
            table->pager->pages[page_num].reset();
        }
    }

    int result = close(table->pager->file_descriptor);
    if (result == -1) {
        std::cout << "Error closing db file." << std::endl;
        _exit(EXIT_FAILURE);
    }
    for (auto& page : table->pager->pages) {
        page.reset();
    }
    table->pager.reset();
    table.reset();
}


void read_input(std::unique_ptr<InputBuffer>& input_buffer) {
    // getline
    // std::cin.ignore();
    std::getline(std::cin >> std::ws, *input_buffer);

    if (std::cin.fail() && !std::cin.eof()) {
        std::cout << "Error reading input"
                  << std::endl;
        _exit(EXIT_FAILURE);
    }
    // Ignore trailing newline
}

// 判断输入的命令 以 ‘.’ 开始的 是否为 meta-commands
MetaCommandResult do_meta_command(std::unique_ptr<InputBuffer>& input_buffer, std::unique_ptr<Table>& table) {
    if (*input_buffer == ".exit") {
        db_close(table);
        _exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// 执行 insert 语句 并 返回执行结果
ExecuteResult execute_insert(Statement& statement, std::unique_ptr<Table>& table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    auto row_to_insert = statement.row_to_insert;
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement& statement, std::unique_ptr<Table>& table) {
    Row row;
    for (uint32_t i = 0; i < table->num_rows; ++i) {
        deserialize_row(row_slot(table, i), row);
        print_row(row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement& statement, std::unique_ptr<Table>& table) {
    switch (statement.type) {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Must supply a database filename." << std::endl;
        _exit(EXIT_FAILURE);
    }
    // 输出版本信息
    print_version();
    // 初始化 table
    char* filename = argv[1];
    auto table = db_open(filename);

    // 初始化 状态缓冲区
    auto input_buffer = new_input_buffer();
    while (true) {
        print_prompt(); // 打印提示符
        read_input(input_buffer);

        // 判断输入的命令 以 ‘.’ 开始的为 meta-commands
        if ((*input_buffer)[0] == '.') {
            switch (do_meta_command(input_buffer, table)) {
            case (MetaCommandResult::META_COMMAND_SUCCESS):
                continue;
            case (MetaCommandResult::META_COMMAND_UNRECOGNIZED_COMMAND):
                std::cout << "Unrecognized command '" << *input_buffer << "'"
                          << std::endl;
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, statement)) {
        case (PrepareResult::PREPARE_SUCCESS):
            break;
        case (PrepareResult::PREPARE_STRING_TOO_LONG):
            std::cout << "String is too long." << std::endl;
            continue;
        case (PrepareResult::PREPARE_NEGATIVE_ID):
            std::cout << "ID must be positive." << std::endl;
            continue;
        case (PrepareResult::PREPARE_SYNTAX_ERROR):
            std::cout << "Syntax error. Could not parse statement."
                      << std::endl;
            continue;
        case (PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized keyword at start of '" << *input_buffer << "'"
                      << std::endl;
            continue;
        }

        switch (execute_statement(statement, table)) {
        case (ExecuteResult::EXECUTE_SUCCESS):
            std::cout << "Executed." << std::endl;
            break;
        case (ExecuteResult::EXECUTE_TABLE_FULL):
            std::cout << "Error: Table full." << std::endl;
            break;
        }
    }
}
