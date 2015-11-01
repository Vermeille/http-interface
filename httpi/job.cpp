#include "job.h"

///////////////// ARG //////////////

Arg::Arg(const std::string& name, const std::string& type, const std::string& desc)
    : name_(name),
    type_(type),
    desc_(desc) {
}

Html Arg::ArgToForm() const {
    return Html() <<
        Div().AddClass("form-group") <<
            Tag("label").Attr("for", name_) << desc_ << Close() <<
            Input().Attr("name", name_).Attr("type", type_).AddClass("form-control")
                .Id(name_).Attr("placeholder", name_) <<
        Close();
}

//////////////// JOBDESC /////////////////

JobDesc::JobDesc(const std::vector<Arg>& args, const std::string& name, const std::string& url,
        const std::string& desc, bool synchronous, bool reentrant,
        const function_type& fun)
    : args_(args), name_(name), url_(url), desc_(desc), synchronous_(synchronous),
    reentrant_(reentrant), exec_(fun) {
}


std::tuple<bool, Html, std::vector<std::string>> JobDesc::ValidateParams(const POSTValues& vs) {
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

Html JobDesc::MakeForm() const {
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

Html JobDesc::DisplayResult(const std::string& res) const {
    return Html() <<
        H1() << name_ << Close() <<
        res;
}

/////////////// JOBSTATUS /////////////////

JobStatus::JobStatus(const JobDesc* desc, const std::vector<std::string>& args, size_t id)
    : start_(std::chrono::system_clock::now()),
    job_(std::async(std::launch::async, desc->function(), args, std::ref(res_))),
    args_(args),
    finished_(false),
    desc_(desc),
    id_(id) {
}

JobResult JobStatus::result() const {
    IsFinished();
    return res_;
}

bool JobStatus::IsFinished() const {
    if (finished_ == false) {
        bool ended = job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        finished_ = ended;
    }
    return finished_;
}

/////////////// RUNNINGJOBS //////////////////


JobDesc* RunningJobs::FindDescriptor(const std::string& url) {
    auto desc = descriptors_.find(url);

    if (desc == descriptors_.end()) {
        return nullptr;
    } else {
        return &desc->second;
    }
}

Html RunningJobs::RenderTableOfRunningJobs() const {
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
                    A().Attr("href", "/job?id=" + std::to_string(rj.second.id())) <<
                        "See" <<
                    Close() <<
                Close() <<
            Close();
    }

    html << Close();
    return html;
}

Html RunningJobs::RenderListOfDescriptors() const {
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

JobStatus* RunningJobs::FindJobWithId(size_t id) {
    auto job = statuses.find(id);

    if (job == statuses.end()) {
        return nullptr;
    } else {
        return &job->second;
    }
}

Html RunningJobs::Exec(const std::string& url, const POSTValues& vs) {
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
        JobResult res;
        desc->function()(args, res);
        html << desc->DisplayResult(*res.Get());
        ++next_id_;
        return html;
    } else {
        statuses.emplace(std::piecewise_construct,
                std::forward_as_tuple(next_id_),
                std::forward_as_tuple(desc, args, next_id_));
        ++next_id_;

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

size_t RunningJobs::StartJob(const JobDesc* desc, const std::vector<std::string>& args) {
    size_t id = next_id_;
    statuses.emplace(std::piecewise_construct,
            std::forward_as_tuple(next_id_),
            std::forward_as_tuple(desc, args, next_id_));
    ++next_id_;
    return id;
}
