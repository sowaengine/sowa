#include "arg_parser.hpp"


template<>
std::string ArgOptionValue::As() {
    return value;
}

template<>
int ArgOptionValue::As() {
    try {
        int k = std::stoi(value.c_str());
        return k;
    } catch(const std::exception&) {
        return 0;
    }
}

template<>
double ArgOptionValue::As() {
    try {
        int k = std::stod(value.c_str());
        return k;
    } catch(const std::exception&) {
        return 0;
    }
}

template<>
bool ArgOptionValue::As() {
    if(value == "true" || value == "1" || value == "True") {
        return true;
    }
    return false;
}