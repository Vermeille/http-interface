#pragma once

#include <string>

namespace httpi {
namespace html {
namespace utils {

template <class FwdIterator>
typename std::enable_if<std::is_same<typename FwdIterator::value_type, std::string>::value, std::string>::type
ToCSV(FwdIterator begin, FwdIterator end) {
    if (begin == end) {
        return "";
    }
    std::string tmp = *begin;
    ++begin;
    for (; begin != end; ++begin) {
        tmp += "," + *begin;
    }
    return tmp;
}

template <class FwdIterator>
typename std::enable_if<!std::is_same<typename FwdIterator::value_type, std::string>::value, std::string>::type
ToCSV(FwdIterator begin, FwdIterator end) {
    if (begin == end) {
        return "";
    }
    std::string tmp = std::to_string(*begin);
    ++begin;
    for (; begin != end; ++begin) {
        tmp += "," + std::to_string(*begin);
    }
    return tmp;
}

std::string SurroundWithQuotes(const std::string& str) { return "\"" + str + "\""; }
std::string SurroundWithBrackets(const std::string& str) { return "[" + str + "]"; }

template <class FwdIterator>
std::string ToJSONList(FwdIterator begin, FwdIterator end) {
    return SurroundWithBrackets(ToCSV(begin, end));
}

} // utils
} // html
} // httpi
