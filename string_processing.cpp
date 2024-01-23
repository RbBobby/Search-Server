#include "string_processing.h"
/*
std::vector<std::string_view> SplitIntoWords( std::string_view text) {
    std::vector<std::string_view> words;
    std::string word;
    for (const char& c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word = "";
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(std::string_view(word));
    }

    return words;
}
*/
std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    int64_t pos = str.find_first_not_of(" ");

    const int64_t pos_end = str.npos;


    while (pos != pos_end) {
        str.remove_prefix(pos);
        int64_t space = str.find(' ');

        result.push_back(str.substr(0, space));

        if (space == pos_end) {
            break;
        }
        pos = str.find_first_not_of(" ", space);


    }

    return result;
}