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

using InputBuffer = std::string;

// 判断输入的命令 以 ‘.’ 开始的 是否为 meta-commands
using MetaCommandResult = enum { META_COMMAND_SUCCESS,
                                 META_COMMAND_UNRECOGNIZED_COMMAND
};

MetaCommandResult do_meta_command(std::unique_ptr<InputBuffer>& input_buffer) {
    if (*input_buffer == ".exit") {
        _exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// 输入命令的合法性
using PrepareResult = enum { PREPARE_SUCCESS,
                             PREPARE_UNRECOGNIZED_STATEMENT
};

// 支持的两种操作：“插入” 和 “打印所有行”
using StatementType = enum { STATEMENT_INSERT,
                             STATEMENT_SELECT };

// 执行的命令类型
using Statement = struct {
    StatementType type;
};

// 判断输入命令是否合法
PrepareResult prepare_statement(std::unique_ptr<InputBuffer>& input_buffer,
                                Statement& statement) {
    if ((*input_buffer).substr(0, 6) == "insert") {
        statement.type = STATEMENT_INSERT;

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

void print_prompt() {
    std::cout << "my_sqlite_db > ";
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

void close_input_buffer(std::unique_ptr<InputBuffer>& input_buffer) {
    input_buffer.reset();
}

void execute_statement(Statement& statement) {
    switch (statement.type) {
    case (STATEMENT_INSERT):
        std::cout << "This is where we would do an insert."
                  << std::endl;
        break;
    case (STATEMENT_SELECT):
        std::cout << "This is where we would do a select."
                  << std::endl;
        break;
    }
}

int main(int argc, char* argv[]) {
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
        case (PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT):
            std::cout << "Unrecognized keyword at start of '" << *input_buffer << "'"
                      << std::endl;
            continue;
        }

        execute_statement(statement);
        std::cout << "Executed." << std::endl;
    }
}