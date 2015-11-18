#pragma once

#include <type_traits>
#include <vector>
#include <string>
#include <map>

namespace httpi {
namespace html {

// Chart description. Use it when you want to render a chart on a page. It does not hold actual
// data. Provide the data you want to show with LogData, and describe the chart here.
class Chart {
    std::string label_;  // the key of the logged data that will be the label / x axis
    std::vector<std::string> values_;  // they keys of the y
    std::string name_;  // a unique token identifying the chart

    typedef std::map<std::string, std::vector<std::string>> logs_type;
    logs_type logged_values_;

    std::string BuildSeries(const std::vector<logs_type::const_iterator>& values) const;
    public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) { label_ = l; return *this; }
    Chart& Value(const std::string& v) { values_.push_back(v); return *this; }

    template <class T>
    typename std::enable_if<!std::is_same<T, std::string>::value, Chart&>::type
        Log(const std::string& k, const T& v) {
            logged_values_[k].push_back(std::to_string(v));
            return *this;
        }

    Chart& Log(const std::string& k, const char* v) {
        logged_values_[k].push_back("\"" + std::string(v) + "\"");
        return *this;
    }
    Chart& Log(const std::string& k, const std::string& v) {
        logged_values_[k].push_back("\"" + v + "\"");
        return *this;
    }

    Chart& MostRecent(int cap) {
        for (auto& v : logged_values_)
            if (v.second.size() > cap)
                v.second.erase(v.second.begin(), v.second.begin() + v.second.size() - cap);
        return *this;
    }

    std::string Get() const;

};

} // html
} // httpi
