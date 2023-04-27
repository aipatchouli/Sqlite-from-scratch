#ifndef utils_HPP
#define utils_HPP

#include "RowStruction.hpp"
#include <iostream>

inline void print_version() {
    std::cout << "my_sqlite_db version 0.1.1" << std::endl;
    std::cout << "Compile Time: 2023-04-27" << std::endl;
}

inline void print_prompt() {
    std::cout << "my_sqlite_db > ";
}

inline void print_row(Row& row) {
    std::cout << "(" << row.id << ", " << row.username.data() << ", " << row.email.data()
              << ")" << std::endl;
}

#endif