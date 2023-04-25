#include <array>
#include <cstddef>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

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

// 判断输入的命令 以 ‘.’ 开始的 是否为 meta-commands
MetaCommandResult do_meta_command(std::unique_ptr<InputBuffer>& input_buffer) {
    if (*input_buffer == ".exit") {
        _exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// 判断输入命令是否合法
PrepareResult
prepare_statement(std::unique_ptr<InputBuffer>& input_buffer,
                  Statement& statement) {
    if ((*input_buffer).substr(0, 6) == "insert") {
        statement.type = STATEMENT_INSERT;
        int args_assigned = sscanf_command(*input_buffer, statement.row_to_insert);
        if (args_assigned != 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
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

// 初始化 table
auto new_table() -> std::unique_ptr<Table> {
    auto table = std::make_unique<Table>();
    table->num_rows = 0;
    for (auto& page : table->pages) {
        page = nullptr;
    }
    return table;
}

void free_table(std::unique_ptr<Table>& table) {
    for (auto& page : table->pages) {
        page.reset();
    }
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

// 执行 insert 语句 并 返回执行结果
ExecuteResult execute_insert(Statement& statement, Table& table) {
    if (table.num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    auto row_to_insert = statement.row_to_insert;
    serialize_row(row_to_insert, row_slot(table, table.num_rows));
    table.num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement& statement, Table& table) {
    Row row;
    for (uint32_t i = 0; i < table.num_rows; ++i) {
        deserialize_row(row_slot(table, i), row);
        print_row(row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement& statement, Table& table) {
    switch (statement.type) {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
    }
}


int main(int argc, char* argv[]) {
    // 输出版本信息
    print_version();
    // 初始化 table
    auto table = new_table();
    // 初始化 状态缓冲区
    auto input_buffer = new_input_buffer();
    while (true) {
        print_prompt(); // 打印提示符
        read_input(input_buffer);

        // 判断输入的命令 以 ‘.’ 开始的为 meta-commands
        if ((*input_buffer)[0] == '.') {
            switch (do_meta_command(input_buffer)) {
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
        case (PrepareResult::PREPARE_SYNTAX_ERROR):
            std::cout << "Syntax error. Could not parse statement."
                      << std::endl;
            continue;
        case (PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized keyword at start of '" << *input_buffer << "'"
                      << std::endl;
            continue;
        }

        switch (execute_statement(statement, *table)) {
        case (ExecuteResult::EXECUTE_SUCCESS):
            std::cout << "Executed." << std::endl;
            break;
        case (ExecuteResult::EXECUTE_TABLE_FULL):
            std::cout << "Error: Table full." << std::endl;
            break;
        }
    }
}