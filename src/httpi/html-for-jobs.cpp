#include "displayer.h"
#include "job.h"


using namespace httpi::html;

class JobsDisplayer : public RequestHandler {
    RunningJobs jobs_;

  public:
    std::string Get(const std::map<std::string, std::string>& args) override {
        auto url = args.find("url")->second;
        auto j_res = jobs_.FindDescriptor(url);
        return MakePage(j_res->MakeForm());
    }

    std::string Post(const std::map<std::string, std::string>& args) override {
        auto url = args.find("url")->second;
        auto j_res = jobs_.FindDescriptor(url);
        return MakePage(j_res.Exec(url, args));
    }

    std::string JobStatuses(const std::string& /* method */, const POSTValues&) {
        return (Html() << H2() << "Running jobs" << Close() << jobs_.RenderTableOfRunningJobs()).Get();
    }

    std::string JobDetails(const std::string& /* method */, const POSTValues& vs) {
        auto id = vs.find("id");

        if (id == vs.end()) {
            return "This job does not exist";
        }

        const JobStatus* rj = jobs_.FindJobWithId(std::atoi(id->second.c_str()));

        if (!rj) {
            return "job not found";
        }

        return (Html() <<
                H1() << "Job Details" << Close() <<
                Div() <<
                P() << "Started on: " << rj->start_time() << Close() <<
                *rj->result().Get() <<
                Close()).Get();
    }

    void RegisterJob(const JobDesc& jd) {
        jobs_.AddDescriptor(jd);
        RegisterUrl(jd.url(), JobCallback);
    }

};

