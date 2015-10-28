#include "chart.h"
#include "html.h"
#include "displayer.h"

std::string Chart::Get(size_t job_id) const {
    auto dl = GetDataLog(job_id);
    auto labels = dl.find(label_);
    std::vector<decltype (dl.find(""))> values;
    bool error = false;

    for (auto& v : dl) {
        std::cout << "key: " << v.first << std::endl;
    }

    for (auto& v : values_)
        values.push_back(dl.find(v));

    Html html;
    if (labels == dl.end() || labels->second.empty()) {
        html <<
            Div().AddClass("alert alert-warning") <<
                "Can't find labels " << label_ << " for " << name_ <<
            Close();
        error = true;
    }

    for (auto& v : values) {
        if (v == dl.end() || v->second.empty()) {
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
                "labels: [" + ToCSV(labels->second) + "].reverse(),"
                "series: [" +
                MAP_INTERSPERSE(values, ", ", v, "[" + ToCSV(v->second) + "].reverse()") +
                "]"
            "});"
        "</script>";
}

