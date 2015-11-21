
#include "displayer.h"
#include "monitoring.h"
#include "job.h"
#include "html/chart.h"
#include "html/form-gen.h"

// a demo file for a toy app

using namespace httpi::html;

// descriptors for two jobs
static const FormDescriptor compute_stuff_form_desc = {
    "Compute Stuff",  // name
    "Compute a + b",  // longer description
    { { "a", "number", "Value A" }, { "b", "number", "Value B" } }
};

static const std::string compute_stuff_form = compute_stuff_form_desc
        .MakeForm("/compute", "POST").Get();

std::string ComputeStuff(const std::vector<std::string>& vs) {
    return std::to_string(std::atoi(vs[0].c_str()) + std::atoi(vs[1].c_str()));
}

static const FormDescriptor permutation_form_desc = {
    "Permutations",
    "Permute a string",
    { { "str", "text", "String to permute" } }
};

static const std::string permutation_form = permutation_form_desc
        .MakeForm("/permute", "POST").Get();

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

int main(int argc, char** argv) {
    //google::InitGoogleLogging(argv[0]);
    InitHttpInterface();  // Init the http server

    WebJobsPool jp;
    auto t1 = jp.StartJob(std::unique_ptr<MonitoringJob>(new MonitoringJob()));
    auto monitoring_job = jp.GetId(t1);

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

    RegisterUrl("/compute", [](const std::string& method, const POSTValues& args) {
            if (method == "GET") {
                return MakePage(compute_stuff_form);
            } else if (method == "POST") {
                auto vargs = compute_stuff_form_desc.ValidateParams(args);
                if (std::get<0>(vargs)) {
                    return MakePage(std::get<1>(vargs).Get());
                } else {
                    return MakePage(ComputeStuff(std::get<2>(vargs)));
                }
            } else {
                return MakePage("no such method");
            }
        });

    RegisterUrl("/permute", [&jp](const std::string& method, const POSTValues& args) {
            if (method == "GET")
                return MakePage(permutation_form);
            else if (method == "POST") {
                auto vargs = permutation_form_desc.ValidateParams(args);
                if (std::get<0>(vargs)) {
                    return MakePage(std::get<1>(vargs).Get());
                } else {
                    jp.StartJob(std::unique_ptr<PermutationJob>(new PermutationJob(std::get<2>(vargs)[0])));
                    return MakePage("Job started");
                }
            }
        });

    ServiceLoopForever();  // infinite loop ending only on SIGINT / SIGTERM / SIGKILL

    StopHttpInterface();  // clear resources
    return 0;
}

