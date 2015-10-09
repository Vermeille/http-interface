#pragma once

#include <vector>
#include <string>

class Chart {
    std::string label_;
    std::vector<std::string> values_;
    const std::string& name_;

    public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) { label_ = l; return *this; }
    Chart& Value(const std::string& v) { values_.push_back(v); return *this; }
    std::string Get();
};

