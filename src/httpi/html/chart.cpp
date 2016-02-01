#include "chart.h"

#include <algorithm>
#include "html.h"
#include "utils.h"

namespace httpi {
namespace html {

std::string Chart::BuildSeries(const std::vector<logs_type::const_iterator>& values) const {
    std::vector<std::string> lists;
    for (auto v : values) {
        lists.push_back(utils::ToJSONList(v->second.begin(), v->second.end()));
    }
    return utils::ToJSONList(lists.begin(), lists.end());
}

std::string Chart::Get() const {
    std::vector<logs_type::const_iterator> values;
    auto labels = logged_values_.find(label_);
    bool error = false;

    for (auto& v : values_)
        values.push_back(logged_values_.find(v));

    Html html;
    if (labels == logged_values_.end() || labels->second.empty()) {
        html <<
            Div().AddClass("alert alert-warning") <<
                "Can't find labels " << label_ << " for " << name_ <<
            Close();
        error = true;
    }

    for (auto& v : values) {
        if (v == logged_values_.end() || v->second.empty()) {
            html <<
                Div().AddClass("alert alert-warning") <<
                    "Can't find values for " << name_ <<
                Close();
            error = true;
        }
    }

    if (error) {
        return html.Get();
    }

    return (html <<
        Div().AddClass("col-md-6") <<
            H3() << name_ << Close() <<
            Div().AddClass("ct-chart ct-golden-section").Id(name_) << Close() <<
        Close()).Get() +
        "<script>"
            "new Chartist.Line('#" + name_ + "', {"
                "labels: " + utils::ToJSONList(labels->second.begin(), labels->second.end()) + ", " +
                "series: " + BuildSeries(values) +
            "});"
        "</script>";
}

} // html
} // httpi
