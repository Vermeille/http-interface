#pragma once

#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace httpi {
namespace html {

// Chart description. Use it when you want to render a chart on a page. It does
// not hold actual
// data. Provide the data you want to show with LogData, and describe the chart
// here.
class Chart {
    // the key of the logged data that will be the label / x axis
    std::string label_;
    // they keys of the y
    std::vector<std::string> values_;
    // a unique token identifying the chart
    std::string name_;

    typedef std::map<std::string, std::vector<std::string>> logs_type;
    logs_type logged_values_;

    std::string BuildSeries(
        const std::vector<logs_type::const_iterator>& values) const;

   public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) {
        label_ = l;
        return *this;
    }
    Chart& Value(const std::string& v) {
        values_.push_back(v);
        return *this;
    }

    template <class T>
    Chart& Log(const std::string& k, const T& v) {
        if
            constexpr(std::is_same<T, std::string>::value ||
                      std::is_same<T, const char*>::value) {
                logged_values_[k].push_back("\"" + v + "\"");
            }
        else {
            logged_values_[k].push_back(std::to_string(v));
        }
        return *this;
    }

    Chart& MostRecent(int cap) {
        for (auto& v : logged_values_)
            if (static_cast<int>(v.second.size()) > cap)
                v.second.erase(v.second.begin(),
                               v.second.begin() + v.second.size() - cap);
        return *this;
    }

    std::string Get() const;
};

}  // html
}  // httpi
