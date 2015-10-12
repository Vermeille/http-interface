#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <future>
#include <memory>

#include "displayer.h"
#include "html.h"

// Represent the arguments a job can take in order to define them.
// TODO: type validator
class Arg {
    std::string name_;  // formal name
    std::string type_;  // from now, it takes the possibles values of an HTML's input tag
    std::string desc_;  // short text describing the argument
  public:

    Arg(const std::string& name, const std::string& type, const std::string& desc)
        : name_(name),
        type_(type),
        desc_(desc) {
    }

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }

    Html ArgToForm() const {
        return Html() <<
            Div().AddClass("form-group") <<
                Tag("label").Attr("for", name_) << desc_ << Close() <<
                Input().Attr("name", name_).Attr("type", type_).AddClass("form-control")
                    .Id(name_).Attr("placeholder", name_) <<
            Close();
    }
};

// Describe a job, generate the HTML for jobs.
class JobDesc {
    std::vector<Arg> args_;  // the job's prototype
    std::string name_;  // short name of the job shown in lists etc
    std::string url_;  // url on which this job will be mapped. Must start with /
    std::string desc_;  // text describing what the job does
    bool synchronous_;  // if synchronous, result will be given immediately. If not, queue a job.
    bool reentrant_;  // if not reentrant, only one running instance of the job is allowed
    std::function<Html(const std::vector<std::string>&)> exec_;  // the actual function called
  public:

    typedef std::function<Html(const std::vector<std::string>&)> function_type;

    const std::vector<Arg>& args() const { return args_; }
    const std::string& name() const { return name_; }
    const std::string& url() const { return url_; }
    const std::string& description() const { return desc_; }
    bool IsSynchronous() const { return synchronous_; }
    function_type function() const { return exec_; }

    JobDesc() = default;
    JobDesc(const std::vector<Arg>& args, const std::string& name, const std::string& url,
            const std::string& desc, bool synchronous, bool reentrant,
            const std::function<Html(const std::vector<std::string>&)> fun)
        : args_(args), name_(name), url_(url), desc_(desc), synchronous_(synchronous),
        reentrant_(reentrant), exec_(fun) {
    }

    // return true and a vector of parameters if all the arguments are present in vs
    // return false and an error page if they're not
    std::tuple<bool, Html, std::vector<std::string>> ValidateParams(const POSTValues& vs) {
        std::vector<std::string> args_values;
        bool error = false;
        Html html;

        for (auto& a : args_) {
            auto arg_value = vs.find(a.name());
            if (arg_value == vs.end()) {
                html << Div().AddClass("alert alert-danger")
                    << a.name() << " was not provided." <<
                Close();
                error = true;
            } else {
                args_values.push_back(arg_value->second);
            }
        }

        return std::make_tuple(error, html, args_values);
    }

    Html MakeForm() const {
        auto html = Html() <<
            H1() << name_ << Close() <<
            P() << desc_ << Close() <<
            Form("POST", url_);

        for (auto& a : args_) {
            html << a.ArgToForm();
        }

        html << Submit() <<
            Close();
        return html;
    }

    Html DisplayResult(const Html& res) const {
        return Html() <<
            H1() << name_ << Close() <<
            res;
    }
};

// Describe a running instance of a job
class JobStatus {
    std::chrono::system_clock::time_point start_;
    mutable std::future<Html> job_;
    std::vector<std::string> args_;
    mutable Html result_;
    mutable bool finished_;
    const JobDesc* const desc_;
  public:

    std::string start_time() const {
        auto time = std::chrono::system_clock::to_time_t(start_);
        return std::ctime(&time);
    }

    const JobDesc* description() const { return desc_; }

    JobStatus(JobStatus&&) = default;
    JobStatus(const JobDesc* desc, const std::vector<std::string>& args)
        : start_(std::chrono::system_clock::now()),
        job_(std::async(std::launch::async, desc->function(), args)),
        args_(args),
        finished_(false),
        desc_(desc) {
    }

    Html result() const {
        IsFinished();
        return result_;
    }

    bool IsFinished() const {
        if (finished_ == false) {
            bool ended = job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            finished_ = ended;
            if (finished_ && job_.valid()) {
                result_ = job_.get();
            }
        }
        return finished_;
    }
};

// pool keeping all the running instances, allowing to start a new job, check their statuses, etc
struct RunningJobs {
    std::map<size_t, JobStatus> statuses;
    std::map<std::string, JobDesc> descriptors_;

    RunningJobs() = default;
    RunningJobs(RunningJobs&&) = default;

    JobDesc* FindDescriptor(const std::string& url) {
        auto desc = descriptors_.find(url);

        if (desc == descriptors_.end()) {
            return nullptr;
        } else {
            return &desc->second;
        }
    }

    void AddDescriptor(const JobDesc& jd) {
        descriptors_[jd.url()] = jd;
    }

    Html RenderTableOfRunningJobs() const {
        Html html;
        html <<
            Tag("table").AddClass("table") <<
                Tag("tr") <<
                    Tag("th") << "Job" << Close() <<
                    Tag("th") << "Started" << Close() <<
                    Tag("th") << "Finished" << Close() <<
                    Tag("th") << "Details" << Close() <<
                Close();

        for (const auto& rj : statuses) {
            html <<
                Tag("tr") <<
                    Tag("td") << rj.second.description()->name() << Close() <<
                    Tag("td") << rj.second.start_time() << Close() <<
                    Tag("td") << (rj.second.IsFinished() ? "true" : "false") << Close() <<
                    Tag("td") <<
                        A().Attr("href", "/job?id=" + rj.first) <<
                            "See" <<
                        Close() <<
                    Close() <<
                Close();
        }

        html << Close();
        return html;
    }

    Html RenderListOfDescriptors() const {
        Html html;

        html << Ul();
        for (auto& j : descriptors_)
            html <<
                Li()
                    << A().Attr("href", j.second.url()) << j.second.name() << Close() <<
                Close();

        html << Close();
        return html;
    }

    JobStatus* FindJobWithId(size_t id) {
        auto job = statuses.find(id);

        if (job == statuses.end()) {
            return nullptr;
        } else {
            return &job->second;
        }
    }

    Html Exec(const std::string& url, const POSTValues& vs) {
        auto descriptor = descriptors_.find(url);

        if (descriptor == descriptors_.end()) {
            LOG(FATAL) << url << " doesn't exist in the jobs pool";
        }
        JobDesc* desc = &descriptor->second;

        bool error;
        Html html;
        std::vector<std::string> args;
        std::tie(error, html, args) = descriptor->second.ValidateParams(vs);

        if (error) {
            return html;
        }

        // TODO: reentrance

        if (desc->IsSynchronous()) {
            html << desc->DisplayResult(desc->function()(args));
            return html;
        } else {
            statuses.emplace(std::make_pair(statuses.size(), JobStatus(desc, args)));

            html <<
                H2() << "Your job have started" << Close() <<
                H3() << desc->name() << Close();

            html << Ul();
            for (size_t i = 0; i < args.size(); ++i) {
                html << Li() << desc->args()[i].name() << " = " << args[i] << Close();
            }
            html << Close();
            return html;
        }
    }
};

