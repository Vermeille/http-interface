#include <glog/logging.h>
#include <folly/Conv.h>

#include "displayer.h"
#include "job.h"

static const JobDesc compute_stuff = {
    { { "a", "int", "Value A" }, { "b", "int", "Value B" } },
    "Compute Stuff",
    "/compute",
    "Compute a + b",
    true /* synchronous */,
    true /* reentrant */,
    [](const std::vector<std::string>& vs) {
        return folly::to<std::string>(folly::to<int>(vs[0]) + folly::to<int>(vs[1]));
    }
};

int main(int argc, char** argv) {
    InitHttpInterface();

    RegisterJob(compute_stuff);
    SetStatusVar("Has Computed", "false");
    ServiceLoopForever();
    StopHttpInterface();
    return 0;
}
