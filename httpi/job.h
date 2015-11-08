#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <future>
#include <memory>

#include "displayer.h"
#include "html/html.h"
#include "html/form-gen.h"

namespace htmli = httpi::html;
using htmli::Arg;
using htmli::Html;

// Represent the arguments a job can take in order to define them.
// TODO: type validator

class JobResult {
    std::shared_ptr<std::string> page_;

    public:
    JobResult() : page_(std::make_shared<std::string>("job just started")) {}

    void SetPage(const Html& page) {
        page_.reset(new std::string(page.Get()));
    }
    std::shared_ptr<std::string> Get() const { return page_; }
};

// Describe a job, generate the HTML for jobs.
class JobDesc {
    std::vector<Arg> args_;  // the job's prototype
    std::string name_;  // short name of the job shown in lists etc
    std::string url_;  // url on which this job will be mapped. Must start with /
    std::string desc_;  // text describing what the job does
    bool synchronous_;  // if synchronous, result will be given immediately. If not, queue a job.
    bool reentrant_;  // if not reentrant, only one running instance of the job is allowed
    std::function<void(const std::vector<std::string>&, JobResult&)> exec_;  // the actual function called
  public:

    typedef std::function<void(const std::vector<std::string>&, JobResult&)> function_type;

    const std::vector<Arg>& args() const { return args_; }
    const std::string& name() const { return name_; }
    const std::string& url() const { return url_; }
    const std::string& description() const { return desc_; }
    bool IsSynchronous() const { return synchronous_; }
    function_type function() const { return exec_; }

    JobDesc() = default;
    JobDesc(const std::vector<Arg>& args, const std::string& name, const std::string& url,
            const std::string& desc, bool synchronous, bool reentrant,
            const function_type& fun);

    // return true and a vector of parameters if all the arguments are present in vs
    // return false and an error page if they're not
    std::tuple<bool, htmli::Html, std::vector<std::string>> ValidateParams(const POSTValues& vs);

    htmli::Html MakeForm() const;
    htmli::Html DisplayResult(const std::string& res) const;
};

// Describe a running instance of a job
class JobStatus {
    std::chrono::system_clock::time_point start_;
    JobResult res_;
    mutable std::future<void> job_;
    std::vector<std::string> args_;
    mutable bool finished_;
    const JobDesc* const desc_;
    const size_t id_;
  public:

    size_t id() const { return id_; }
    std::string start_time() const {
        auto time = std::chrono::system_clock::to_time_t(start_);
        return std::ctime(&time);
    }

    const JobDesc* description() const { return desc_; }

    JobStatus(JobStatus&&) = default;
    JobStatus(const JobDesc* desc, const std::vector<std::string>& args, size_t id);

    JobResult result() const;

    bool IsFinished() const;
};

// pool keeping all the running instances, allowing to start a new job, check their statuses, etc
class RunningJobs {
    std::map<size_t, JobStatus> statuses;
    std::map<std::string, JobDesc> descriptors_;
    size_t next_id_ = 1;
  public:

    RunningJobs() = default;
    RunningJobs(RunningJobs&&) = default;

    void AddDescriptor(const JobDesc& jd) {
        descriptors_[jd.url()] = jd;
    }

    JobDesc* FindDescriptor(const std::string& url);
    htmli::Html RenderTableOfRunningJobs() const;
    htmli::Html RenderListOfDescriptors() const;
    JobStatus* FindJobWithId(size_t id);
    htmli::Html Exec(const std::string& url, const POSTValues& vs);
    size_t StartJob(const JobDesc* desc, const std::vector<std::string>& args);
};

void RegisterJob(const JobDesc& jd);  // maps the job to its url etc
