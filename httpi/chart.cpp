#include "chart.h"
#include "html.h"
#include "displayer.h"

std::string Chart::Get() const {
    std::vector<decltype (logged_values_.find(""))> values;
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
                "labels: [" + MapIntersperse(labels->second, ", ", [](const std::string& str) {
                        return "\"" + str + "\""; }) + "].reverse(),"
                "series: [" +
                MAP_INTERSPERSE(values, ", ", v, "[" + ToCSV(v->second) + "].reverse()") +
                "]"
            "});"
        "</script>";
}

