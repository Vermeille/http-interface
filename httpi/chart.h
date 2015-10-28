#pragma once

#include <vector>
#include <string>

// Chart description. Use it when you want to render a chart on a page. It does not hold actual
// data. Provide the data you want to show with LogData, and describe the chart here.
class Chart {
    std::string label_;  // the key of the logged data that will be the label / x axis
    std::vector<std::string> values_;  // they keys of the y
    std::string name_;  // a unique token identifying the chart

    public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) { label_ = l; return *this; }
    Chart& Value(const std::string& v) { values_.push_back(v); return *this; }
    std::string Get(size_t job_id) const;
};

