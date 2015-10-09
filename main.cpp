#include <glog/logging.h>

#include "displayer.h"

struct Arg {
    std::string name;
    std::string type;
    std::string desc;
};

struct TaskDesc {
    std::vector<Arg> args;
    std::string name;
    std::string url;
    std::string desc;
    std::function<std::string(const std::vector<std::string>&)> fun;
};

static const TaskDesc compute_stuff = {
    { { "a", "int", "Value A" }, { "b", "int", "Value B" } },
    "Compute Stuff",
    "/compute",
    "Compute a + b",
    [](const std::vector<std::string>&) { return "jkjhk"; }
};

Html ArgToForm(const Arg& arg) {
    return Html() <<
        Div().AddClass("form-group") <<
            Tag("label").Attr("for", arg.name) << arg.name << Close() <<
            InputNumber().Name(arg.name).AddClass("form-control")
                .Id(arg.name).Attr("placeholder", arg.desc) <<
        Close();
}

Html TaskToForm(const TaskDesc& td) {
    auto html = Html() <<
        H1() << td.name << Close() <<
        P() << td.desc << Close() <<
        Form("POST", "/compute");

        for (auto& a : td.args) {
            html << ArgToForm(a);
        }

        html << Submit() <<
            Close();
    return html;
}

Html ComputeStuff(const std::string& method, const POSTValues& args) {
    Html html;
    if (method == "GET") {
        return TaskToForm(compute_stuff);
    } else if (method == "POST") {
        auto find_a = args.find("a");
        auto find_b = args.find("b");
        if (find_a != args.end() && find_b != args.end()) {
            int a = std::atoi(find_a->second.c_str());
            int b = std::atoi(find_b->second.c_str());
            SetStatusVar("Has Computed", std::to_string(a + b));

            html <<
                H1() << "Compute Stuff" << Close() <<
                Ul();

            for (auto& arg : args)
                html << Li() << arg.first + ": " + arg.second << Close();

            html << Close() <<
            P() << "Result:" + std::to_string(a + b) << Close();
        } else {
            html <<
                H1() << "Compute Stuff" << Close();

                if (find_a == args.end())
                    html << P() << "No a" << Close();

                if (find_b == args.end())
                    html << P() << "No b" << Close();

                html << H2() << "Map Contains" << Close() <<
                Ul();

                for (auto& arg : args)
                    html << Li() << arg.first + ": " + arg.second << Close();

                html << Close();
        }
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
