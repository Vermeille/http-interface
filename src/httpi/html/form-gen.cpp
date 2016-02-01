#include "form-gen.h"

namespace httpi {
namespace html {

///////////////// ARG //////////////

Arg::Arg(const std::string& name, const std::string& type, const std::string& desc)
    : name_(name),
    type_(type),
    desc_(desc) {
}

Html Arg::ArgToForm() const {
    return Html() <<
        Div().AddClass("form-group") <<
            Tag("label").Attr("for", name_) << desc_ << Close() <<
            Input().Attr("name", name_).Attr("type", type_).AddClass("form-control")
                .Id(name_).Attr("placeholder", name_) <<
        Close();
}

template <>
std::string Convert<std::string>(
        const std::string& str,
        std::vector<std::string>&) {
    return str;
}

template <>
int Convert<int>(const std::string& str, std::vector<std::string>& err) {
    try {
        return std::stoi(str.c_str());
    } catch (...) {
        err.push_back("cannot convert " + str + " to int");
        return 0;
    }
}

Html FormSerializer::MakeForm() const {
    auto html = Html() <<
        H1() << name_ << Close() <<
        P() << desc_ << Close() <<
        Form(method_ == "GET" ? "GET" : "POST", url_);

    for (auto& a : args_) {
        html << a.ArgToForm();
    }
    html << Input()
        .Name("_method")
        .Attr("type", "hidden")
        .Attr("value", method_);

    html << Submit() <<
        Close();
    return html;
}

FormSerializer::FormSerializer(
        const std::string& method,
        const std::string& url,
        const std::string& name,
        const std::string& desc,
        const std::vector<Arg>& args)
    : method_(method), url_(url), name_(name), desc_(desc), args_(args) {
}

} // html
} // httpi
