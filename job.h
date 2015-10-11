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
    mutable std::future<Html> job;
    std::vector<std::string> args;
    mutable Html result;
    mutable bool finished;

    JobStatus(JobStatus&&) = default;
    template <class F>
    JobStatus(F&& f, const std::vector<std::string>& args)
        : start(std::chrono::system_clock::now()),
        job(std::async(std::launch::async, f, args)),
        finished(false) {
    }

    bool IsFinished() const {
        if (finished == false) {
            bool ended = job.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            finished = ended;
            if (finished && job.valid()) {
                result = job.get();
            }
        }
        return finished;
    }
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
            statuses.emplace_back(JobStatus(desc.exec, args));

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

