#ifndef RowStruction_HPP
#define RowStruction_HPP

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "TypeEnum.hpp"

// 定义 一行输入命令 Row 的结构

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 256

using Row = struct Row {
    uint32_t id{};
    std::array<char, COLUMN_USERNAME_SIZE + 1> username{};
    std::array<char, COLUMN_EMAIL_SIZE + 1> email{};
} __attribute__((aligned(128)));

inline PrepareResult sscanf_command(const std::string& str, Row& row) {
    std::string cmd{};
    int32_t id = 0;
    std::string username{};
    std::string email{};
    std::string remaining{};

    // split string
    std::istringstream iss(str);
    iss >> cmd >> id >> username >> email >> remaining;

    if (cmd != "insert" || !remaining.empty() || typeid(id) != typeid(std::int32_t) || typeid(username) != typeid(std::string) || typeid(email) != typeid(std::string)) {
        return PREPARE_SYNTAX_ERROR;
    }

    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }

    row.id = id;

    if (!username.empty()) {
        // 限制字符最多32个
        if (username.size() > COLUMN_USERNAME_SIZE) {
            return PREPARE_STRING_TOO_LONG;
        }
        int len = std::min(static_cast<int>(username.size()), COLUMN_USERNAME_SIZE);
        std::copy(username.begin(), username.begin() + len, row.username.begin());
        // std::cout << "username: " << row.username.data() << std::endl;
    }
    if (!email.empty()) {
        // 限制字符最多256个
        if (email.size() > COLUMN_EMAIL_SIZE) {
            return PREPARE_STRING_TOO_LONG;
        }
        int len = std::min(static_cast<int>(email.size()), COLUMN_EMAIL_SIZE);
        std::copy(email.begin(), email.begin() + len, row.email.begin());
        // std::cout << "email: " << row.email.data() << std::endl;
    }
    return PREPARE_SUCCESS;
}

#endif // RowStruction_HPP