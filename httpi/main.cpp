#include <glog/logging.h>

#include "displayer.h"
#include "job.h"

// a demo file for a toy app

// descriptors for two jobs
static const JobDesc compute_stuff = {
    { { "a", "number", "Value A" }, { "b", "number", "Value B" } },  // args
    "Compute Stuff",  // name
    "/compute",  // url
    "Compute a + b",  // longer description
    true /* synchronous */,
    true /* reentrant */,
    [](const std::vector<std::string>& vs, JobResult& job) { // the actual function
        job.SetPage(Html() << std::to_string(std::atoi(vs[0].c_str()) + std::atoi(vs[1].c_str())));
    }
};

int factorial(int n) {
    return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

static const JobDesc ackermann = {
    { { "str", "text", "String to permute" } },
    "Permutations",
    "/permute",
    "Permute a string",
    false /* synchronous */,
    true /* reentrant */,
    [](const std::vector<std::string>& vs, JobResult& job) {
        Chart progression("progression");
        progression.Label("iter").Value("iter").Value("max");

        std::string str = vs[0];
        std::sort(str.begin(), str.end());

        int i = 0;
        size_t total = factorial(str.size());

        Html html;
        do {
            html << Li() << str << Close();
            if (i % 10 == 0) {
                progression.Log("iter", i)
                    .Log("max", total)
                    .MostRecent(30);
                job.SetPage(
                        Html() << progression.Get() <<
                        Ul() << html << Close());
            }
            ++i;
        } while(std::next_permutation(str.begin(), str.end()));
    },
};

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    InitHttpInterface();  // Init the http server

    // register the two functions
    RegisterJob(compute_stuff);
    RegisterJob(ackermann);

    SetStatusVar("Has Computed", "false");  // simple status variable, see it on the status page
    ServiceLoopForever();  // infinite loop ending only on SIGINT / SIGTERM / SIGKILL

    StopHttpInterface();  // clear resources
    return 0;
}

