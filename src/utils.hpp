#ifndef UTILS_HPP
#include <algorithm>
#include <array>
#include <bits/stdc++.h>
#include <bits/types/stack_t.h>

#include <atomic>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <ostream>
#include <queue>
#include <random>
#include <stack>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

using Row = struct Row {
    uint32_t id{};
    std::array<char, 32> username{};
    std::array<char, 256> email{};
} __attribute__((aligned(128)));


inline int sscanf_command(const std::string& str, Row& row) {
    std::string cmd{};
    uint32_t id = 0;
    std::string username{};
    std::string email{};
    std::string remaining{};

    // split string
    std::istringstream iss(str);
    iss >> cmd >> id >> username >> email >> remaining;

    if (cmd != "insert" || !remaining.empty() || typeid(id) != typeid(uint32_t) || typeid(username) != typeid(std::string) || typeid(email) != typeid(std::string)) {
        return -1;
    }
    int cnt = 0;
    if (id != 0) {
        row.id = id;
        ++cnt;
    }
    if (!username.empty()) {
        // 限制字符最多32个
        int len = std::min(static_cast<int>(username.size()), COLUMN_USERNAME_SIZE);
        std::copy(username.begin(), username.begin() + len, row.username.begin());
        ++cnt;
    }
    if (!email.empty()) {
        // 限制字符最多256个
        int len = std::min(static_cast<int>(email.size()), COLUMN_EMAIL_SIZE);
        std::copy(email.begin(), email.begin() + len, row.email.begin());
        ++cnt;
    }
    return cnt;
}

#endif // UTILS_HPP