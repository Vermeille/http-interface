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
    bool reentrant;
    std::function<std::string(const std::vector<std::string>&)> exec;

    Html MakeForm() const {
        auto html = Html() <<
            H1() << name << Close() <<
            P() << desc << Close() <<
            Form("POST", url);

        for (auto& a : args) {
            html << a.ArgToForm();
        }

        html << Submit() <<
            Close();
        return html;
    }

    Html DisplayResult(const std::string& res) const {
        return Html() <<
            H1() << name << Close() <<
            P() << res << Close();
    }

    Html Exec(const POSTValues& vs) const {
        std::vector<std::string> args_values;
        bool error = false;
        Html html;

        for (auto& a : args) {
            auto arg_value = vs.find(a.name);
            if (arg_value == vs.end()) {
                html << Div().AddClass("alert alert-danger")
                    << a.name << " was not provided." <<
                Close();
                error = true;
            } else {
                args_values.push_back(arg_value->second);
            }
        }

        if (error) {
            return html;
        } else {
            html << DisplayResult(exec(args_values));
            return html;
        }
    }
};

/*
struct JobStatus {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    const JobDesc& whoami;
    std::future<std::string> job;
};
*/
