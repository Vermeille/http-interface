#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

#include "displayer.h"
#include "html.h"

struct Arg {
    std::string name;
    std::string type;
    std::string desc;

    Html ArgToForm() const {
        return Html() <<
            Div().AddClass("form-group") <<
                Tag("label").Attr("for", name) << desc << Close() <<
                InputNumber().Name(name).AddClass("form-control")
                    .Id(name).Attr("placeholder", name) <<
            Close();
    }
};

struct JobDesc {
    std::vector<Arg> args;
    std::string name;
    std::string url;
    std::string desc;
    bool synchronous;
    std::function<std::string(const std::vector<std::string>&)> exec;


    Html TaskToForm() const {
        auto html = Html() <<
            H1() << name << Close() <<
            P() << desc << Close() <<
            Form("POST", "/compute");

        for (auto& a : args) {
            html << a.ArgToForm();
        }

        html << Submit() <<
            Close();
        return html;
    }
};

struct JobStatus {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
};
