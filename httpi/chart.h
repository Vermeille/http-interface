#pragma once

#include <vector>
#include <string>
#include <map>

#include "displayer.h"

// Chart description. Use it when you want to render a chart on a page. It does not hold actual
// data. Provide the data you want to show with LogData, and describe the chart here.
class Chart {
    std::string label_;  // the key of the logged data that will be the label / x axis
    std::vector<std::string> values_;  // they keys of the y
    std::string name_;  // a unique token identifying the chart

    std::map<std::string, std::vector<std::string>> logged_values_;

    public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) { label_ = l; return *this; }
    Chart& Value(const std::string& v) { values_.push_back(v); return *this; }

    template <class T>
    Chart& Log(const std::string& k, T v) { logged_values_[k].push_back(ToString(v)); return *this; }
    Chart& MostRecent(int cap) {
        for (auto& v : logged_values_)
            if (v.second.size() > cap)
                v.second.erase(v.second.begin(), v.second.begin() + v.second.size() - cap);
        return *this;
    }

    std::string Get() const;
};

