#include <utility>

#include <httpi/rest-helpers.h>
#include <httpi/displayer.h>
#include <httpi/monitoring.h>
#include <httpi/job.h>
#include <httpi/html/chart.h>
#include <httpi/html/form-gen.h>
#include <httpi/html/json.h>

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

std::string MakePage(const std::string& content) {
    using namespace httpi::html;
    return (Html() <<
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
                        Ul() <<
                            Li() <<
                                A().Attr("href", "/jobs") << "Jobs" << Close() <<
                            Close() <<
                            Li() <<
                                A().Attr("href", "/permute") <<
                                    "Permute" <<
                                Close() <<
                            Close() <<
                            Li() <<
                                A().Attr("href", "/compute") <<
                                    "Addition" <<
                                Close() <<
                            Close() <<
                        Close() <<
                    "</div>"
                "</div>"
            "</body>"
        "</html>").Get();
}

int main(int argc, char** argv) {
    //google::InitGoogleLogging(argv[0]);
    InitHttpInterface();  // Init the http server

    WebJobsPool jp;
    auto t1 = jp.StartJob(std::unique_ptr<MonitoringJob>(new MonitoringJob()));
    auto monitoring_job = jp.GetId(t1);

    RegisterUrl("/compute", httpi::RestPageMaker(MakePage)
            .AddResource("GET", httpi::RestResource(
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
                [](int a) {
                    return JsonBuilder()
                        .Append("result", a)
                        .Build();
                })));

    RegisterUrl("/permute", httpi::RestPageMaker(MakePage)
            .AddResource("GET", httpi::RestResource(
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
                [](int id) {
                    return JsonBuilder()
                        .Append("job_id", id)
                        .Build();
                })));

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

