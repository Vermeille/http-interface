#include "displayer.h"

size_t Ackermann(int m, int n) {
    if (m == 0) {
        return n +1;
    } else if (n == 0) {
        return Ackermann(m - 1, 1);
    } else {
        return Ackermann(m - 1, Ackermann(m, n - 1));
    }
}

std::string ComputeStuff(const std::string& method, const POSTValues& args) {
    if (method == "GET") {
        return
            "<h1>Compute Stuff</h1>" +
            Form("POST", "/compute")
                .Number("a")
                .Number("b")
                .Submit("Ackermann")
            .Get();
    } else if (method == "POST") {
        auto find_a = args.find("a");
        auto find_b = args.find("b");
        if (find_a != args.end() && find_b != args.end()) {
            int a = std::atoi(find_a->second.c_str());
            int b = std::atoi(find_b->second.c_str());
            SetStatusVar("Has Computed", std::to_string(a + b));
            return
                "<h1>Compute Stuff</h1>"
                "<ul>" + MapConcat(args, [] (const POSTValues::value_type& v) {
                        return "<li>" + v.first + ": " + v.second + "</li>";
                        }) +
                "</ul>"
            "<p>Result:" + std::to_string(Ackermann(a, b)) + "</p>";
        } else {
            return
                "<h1>Compute Stuff</h1>" +
                std::string(find_a == args.end() ? "<p>No a</p>" : "") +
                (find_b == args.end() ? "<p>No b</p>" : "") +
                "<h2>Map Contains:</h2>"
                "<ul>" + MapConcat(args, [] (const POSTValues::value_type& v) {
                        return "<li>" + v.first + ": " + v.second + "</li>";
                        }) + "</ul>";
        }
    }
    return "INVALID";
}

int main(int argc, char** argv) {
    InitHttpInterface();

    RegisterUrl("/compute", ComputeStuff);
    SetStatusVar("Has Computed", "false");
    ServiceLoopForever();
    StopHttpInterface();
    return 0;
}
