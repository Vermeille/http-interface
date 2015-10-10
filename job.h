#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <future>
#include <memory>

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
                Input().Attr("name", name).Attr("type", type).AddClass("form-control")
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
    std::function<Html(const std::vector<std::string>&)> exec;

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

    Html DisplayResult(const Html& res) const {
        return Html() <<
            H1() << name << Close() <<
            res;
    }
};

struct JobStatus {
    std::chrono::system_clock::time_point start;
    std::future<Html> job;

    JobStatus(std::chrono::system_clock::time_point s, std::future<Html>&& future) :
        start(s), job(std::move(future)) {}
    JobStatus(JobStatus&&) = default;
};

struct RunningJobs {
    const JobDesc desc;
    std::vector<JobStatus> statuses;

    RunningJobs(const JobDesc& d) : desc(d) {}
    RunningJobs(RunningJobs&&) = default;

    std::tuple<bool, Html, std::vector<std::string>> ValidateParams(const POSTValues& vs) {
        std::vector<std::string> args_values;
        bool error = false;
        Html html;

        for (auto& a : desc.args) {
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

        return std::make_tuple(error, html, args_values);
    }

    Html Exec(const POSTValues& vs) {
        bool error;
        Html html;
        std::vector<std::string> args;
        std::tie(error, html, args) = ValidateParams(vs);

        if (error) {
            return html;
        }

        if (desc.synchronous) {
            html << desc.DisplayResult(desc.exec(args));
            return html;
        } else {
            statuses.emplace_back(JobStatus(std::chrono::system_clock::now(),
                    std::async(std::launch::async, desc.exec, args))
                );

            html <<
                H2() << "Your job have started" << Close() <<
                H3() << desc.name << Close();

            html << Ul();
            for (size_t i = 0; i < args.size(); ++i) {
                html << Li() << desc.args[i].name << " = " << args[i] << Close();
            }
            html << Close();
            return html;
        }
    }
};

