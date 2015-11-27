#include <utility>

#include "displayer.h"
#include "monitoring.h"
#include "job.h"
#include "html/chart.h"
#include "html/form-gen.h"

// a demo file for a toy app

using namespace httpi::html;

static const FormDescriptor<std::string> permute_form_desc = {
    "POST", "/permute",
    "Permutations",
    "Permute a string",
    { { "str", "text", "String to permute" } }
};

static const std::string permutation_form = permute_form_desc
        .MakeForm().Get();

class PermutationJob : public WebJob {
    int factorial(int n) {
        return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
    }

    std::string str_;
  public:

    PermutationJob(const std::string& str) : str_(str) {}

    std::string name() const { return "Permute"; }

    void Do() override {
        Chart progression("progression");
        progression.Label("iter").Value("iter").Value("max");

        std::string str = str_;
        std::sort(str.begin(), str.end());

        int i = 0;
        size_t total = factorial(str.size());

        Html html;
        html << H1() << name();
        do {
            html << Li() << str << Close();
            if (i % 10 == 0) {
                progression.Log("iter", std::to_string(i))
                    .Log("max", std::to_string(total))
                    .MostRecent(30);
                SetPage(Html() << progression.Get() << Ul() << html << Close());
            }
            ++i;
        } while(std::next_permutation(str.begin(), str.end()));
    }
};

template <class F, class... Args, size_t... I>
auto CurryCallImpl(
        F&& f,
        const std::tuple<Args...>& args,
        std::index_sequence<I...>) {
    return f(std::get<I>(args)...);
}

template <class F, class... Args>
auto CurryCall(F&& f, const std::tuple<Args...>& args) {
    return CurryCallImpl(
            f,
            args,
            std::index_sequence_for<Args...>());
}

std::string MakePage(const std::string& content) {
    return (httpi::html::Html() <<
        "<!DOCTYPE html>"
        "<html>"
           "<head>"
                R"(<meta charset="utf-8">)"
                R"(<meta http-equiv="X-UA-Compatible" content="IE=edge">)"
                R"(<meta name="viewport" content="width=device-width, initial-scale=1">)"
                R"(<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css">)"
                R"(<link rel="stylesheet" href="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.css">)"
                R"(<script src="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.js"></script>)"
            "</head>"
            "<body lang=\"en\">"
                "<div class=\"container\">"
                    "<div class=\"col-md-9\">" <<
                        content <<
                    "</div>"
                    "<div class=\"col-md-3\">" <<
                        "JOBS"
                    "</div>"
                "</div>"
            "</body>"
        "</html>").Get();
}

class RestResource {
    class ResourceAccessor {
      public:
        virtual std::string HtmlProcess(const POSTValues&) const = 0;
        virtual std::string JsonProcess(const POSTValues&) const = 0;
        virtual std::string MakeForm() const = 0;
    };

    template <class Serializer, class Exec, class HtmlRenderer, class JsonRenderer>
    class ResourceImpl : public ResourceAccessor {
        Serializer serializer_;
        Exec exec_;
        HtmlRenderer html_;
        JsonRenderer json_;

      public:
        // FIXME: I don't belong here
        Html ErrorsToHtml(const std::vector<std::string>& errs) const {
            Html html;
            for (const auto& e : errs) {
                html << Div().AddClass("alert") << e << Close();
            }
            return html;
        }

        ResourceImpl(
                Serializer&& s,
                Exec&& e,
                HtmlRenderer&& html,
                JsonRenderer&& json)
            : serializer_(s), exec_(e), html_(html), json_(json) {}

        virtual std::string HtmlProcess(const POSTValues& post_args) const {
            Html html;
            auto args = serializer_.Validate(post_args);
            if (!args.second.empty()) {
                html << ErrorsToHtml(args.second);
            } else {
                html << html_(CurryCall(exec_, args.first));
            }
            return html.Get();
        }

        virtual std::string JsonProcess(const POSTValues& post_args) const {
            auto args = serializer_.Validate(post_args);
            if (!args.second.empty()) {
                return ErrorsToHtml(args.second).Get();
            } else {
                return json_(CurryCall(exec_, args.first));
            }
        }

        virtual std::string MakeForm() const {
            return serializer_.MakeForm().Get();
        }
    };

    std::shared_ptr<ResourceAccessor> rs_accessor_;

  public:
    std::string Process(const POSTValues& args) const {
        auto content_type = args.find("Content-Type");
        if (content_type != args.end()
                && content_type->second == "application/json") {
            return rs_accessor_->JsonProcess(args);
        } else {
            return rs_accessor_->HtmlProcess(args);
        }
    }

    std::string MakeForm() const {
        return rs_accessor_->MakeForm();
    }

    template <class Serializer, class Exec, class HtmlRenderer, class JsonRenderer>
    RestResource(
            Serializer&& s,
            Exec&& e,
            HtmlRenderer&& html,
            JsonRenderer&& json)
        : rs_accessor_(
                std::make_unique<ResourceImpl<Serializer,Exec, HtmlRenderer, JsonRenderer>>(
                    std::forward<Serializer>(s),
                    std::forward<Exec>(e),
                    std::forward<HtmlRenderer>(html),
                    std::forward<JsonRenderer>(json))) {
    }
};

class RestPageMaker {
    std::map<std::string, RestResource> resources_;
    std::function<std::string(const std::string&)> theme_;

    public:
    RestPageMaker(const decltype(theme_)& theme) : theme_(theme) {}

    RestPageMaker(const RestPageMaker& theme) = default;

    RestPageMaker& AddResource(const std::string& method, const RestResource& res) {
        resources_.emplace(method, res);
        return *this;
    }

    std::string operator()(const std::string& method, const POSTValues& args) const {
        auto content_type = args.find("Content-Type");
        if (content_type != args.end()
                && content_type->second == "application/json") {
            return JsonProcess(method, args);
        } else {
            return HtmlProcess(method, args);
        }
    }

    std::string JsonProcess(const std::string& method, const POSTValues& args) const {
        auto resource = resources_.find(method);
        if (resource == resources_.end()) {
            return (Html() << Div().AddClass("alert alert-red") <<
                "method not supported" << Close()).Get();
        }
        std::string json = resource->second.Process(args);
        return json + "\n";
    }

    std::string HtmlProcess(const std::string& method, const POSTValues& args) const {
        Html html;

        auto resource = resources_.find(method);
        if (resource == resources_.end()) {
            return (html << Div().AddClass("alert alert-red") <<
                "method not supported" << Close()).Get();
        }

        html << resource->second.Process(args);

        for (const auto& r : resources_) {
            html << r.second.MakeForm();
        }

        return theme_(html.Get()) + "\n";
    }
};

int main(int argc, char** argv) {
    //google::InitGoogleLogging(argv[0]);
    InitHttpInterface();  // Init the http server

    WebJobsPool jp;
    auto t1 = jp.StartJob(std::unique_ptr<MonitoringJob>(new MonitoringJob()));
    auto monitoring_job = jp.GetId(t1);

    RegisterUrl("/compute", RestPageMaker(MakePage)
            .AddResource("GET", RestResource(
                FormDescriptor<int, int> {
                    "GET", "/compute",
                    "Compute Stuff",  // name
                    "Compute a + b",  // longer description
                    { { "a", "number", "Value A" },
                      { "b", "number", "Value B" } }
                },
                [](int a, int b) {
                    return a + b;
                },
                [](int a) { return std::to_string(a); },
                [](int a) { return "{\"result\":" + std::to_string(a) + "}"; }
                )));

    RegisterUrl("/permute", RestPageMaker(MakePage)
            .AddResource("GET", RestResource(
                FormDescriptor<std::string> {
                    "GET", "/permute",
                    "Permute Stuff",  // name
                    "Permute a string",  // longer description
                    { { "str", "text", "the string" } }
                },
                [&jp](const std::string& str) {
                    return jp.StartJob(std::make_unique<PermutationJob>(str));
                },
                [](int id) { return Html() << "job_id: " << std::to_string(id); },
                [](int id) { return "{\"job_id\":" + std::to_string(id) + "}"; })));

    RegisterUrl("/", [&jp, &monitoring_job](const std::string&, const POSTValues&) {
            return MakePage(*monitoring_job->job_data().page());
        });

    RegisterUrl("/jobs", [&jp](const std::string&, const POSTValues& args) {
            auto id = args.find("id");
            if (id == args.end()) {
                Html html;
                html <<
                    Table().AddClass("table") <<
                        Tr() <<
                            Th() << "Job" << Close() <<
                            Th() << "Started" << Close() <<
                            Th() << "Finished" << Close() <<
                            Th() << "Details" << Close() <<
                        Close();

                jp.foreach_job([&html](WebJobsPool::job_type& x) {
                    html <<
                        Tr() <<
                            Td() <<
                                std::to_string(x.first) << ": " <<
                                x.second->job_data().name() <<
                            Close() <<
                            Td() << "xxxx" << Close() <<
                            Td() <<
                                    (x.second->IsFinished() ? "true" : "false") <<
                            Close() <<
                            Td() <<
                                A().Attr("href", "/jobs?id="
                                    + std::to_string(x.first)) << "See" <<
                                Close() <<
                            Close() <<
                        Close();
                });

                html << Close();
                return MakePage(html.Get());
            } else {
                Html html;
                auto job = jp.GetId(std::atoi(id->second.c_str()));

                if (job == nullptr) {
                    return MakePage((Html() << "not found").Get());
                }

                return MakePage(*job->job_data().page());
            }
        });

    ServiceLoopForever();  // infinite loop ending only on SIGINT / SIGTERM / SIGKILL

    StopHttpInterface();  // clear resources
    return 0;
}

