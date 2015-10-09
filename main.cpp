#include <glog/logging.h>

#include "displayer.h"
#include "job.h"

static const JobDesc compute_stuff = {
    { { "a", "int", "Value A" }, { "b", "int", "Value B" } },
    "Compute Stuff",
    "/compute",
    "Compute a + b",
    true,
    [](const std::vector<std::string>&) { return "jkjhk"; }
};

Html ComputeStuff(const std::string& method, const POSTValues& args) {
    Html html;
    if (method == "GET") {
        return compute_stuff.TaskToForm();
    } else if (method == "POST") {
        bool error = false;
        for (auto& a : compute_stuff.args) {
            if (args.find(a.name) == args.end()) {
                html << Div().AddClass("alert alert-danger")
                    << a.name << " was not provided." <<
                Close();
                error = true;
            }
        }

        if (error) {
            return html;
        }

        int a = std::atoi(args.find("a")->second.c_str());
        int b = std::atoi(args.find("b")->second.c_str());
        SetStatusVar("Has Computed", std::to_string(a + b));

        html <<
            H1() << compute_stuff.desc << Close() <<
            P() << "Result:" + std::to_string(a + b) << Close();
    }
    return html;
}

int main(int argc, char** argv) {
    InitHttpInterface();

    RegisterUrl("/compute", ComputeStuff);
    SetStatusVar("Has Computed", "false");
    ServiceLoopForever();
    StopHttpInterface();
    return 0;
}
