#include <glog/logging.h>
#include <folly/Conv.h>

#include "displayer.h"
#include "job.h"

static const JobDesc compute_stuff = {
    { { "a", "number", "Value A" }, { "b", "number", "Value B" } },
    "Compute Stuff",
    "/compute",
    "Compute a + b",
    true /* synchronous */,
    true /* reentrant */,
    [](const std::vector<std::string>& vs) {
        return Html() << folly::to<std::string>(folly::to<int>(vs[0]) + folly::to<int>(vs[1]));
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
    InitHttpInterface();

    RegisterJob(compute_stuff);
    RegisterJob(ackermann);
    SetStatusVar("Has Computed", "false");
    ServiceLoopForever();
    StopHttpInterface();
    return 0;
}
