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
    [](const std::vector<std::string>& vs) { // the actual function
        return Html() << std::to_string(std::atoi(vs[0].c_str()) + std::atoi(vs[1].c_str()));
    }
};

static const JobDesc ackermann = {
    { { "str", "text", "String to permute" } },
    "Permutations",
    "/permute",
    "Permute a string",
    false /* synchronous */,
    true /* reentrant */,
    [](const std::vector<std::string>& vs) {
        std::string str = vs[0];
        std::sort(str.begin(), str.end());

        Html html;
        html << Ul();
        do {
            html << Li() << str << Close();
        } while(std::next_permutation(str.begin(), str.end()));
        html << Close();

        return html;
    }
};

int main(int argc, char** argv) {
    InitHttpInterface();  // Init the http server

    // register the two functions
    RegisterJob(compute_stuff);
    RegisterJob(ackermann);

    SetStatusVar("Has Computed", "false");  // simple status variable, see it on the status page
    ServiceLoopForever();  // infinite loop ending only on SIGINT / SIGTERM / SIGKILL

    StopHttpInterface();  // clear resources
    return 0;
}
