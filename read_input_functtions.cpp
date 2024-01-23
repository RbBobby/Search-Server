#include "read_input_functions.h"

inline std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

inline int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}