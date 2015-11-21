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

Html FormDescriptor::MakeForm(const std::string& dst_url,
        const std::string& method) const {
    auto html = Html() <<
        H1() << name_ << Close() <<
        P() << desc_ << Close() <<
        Form(method, dst_url);

    for (auto& a : args_) {
        html << a.ArgToForm();
    }

    html << Submit() <<
        Close();
    return html;
}

FormDescriptor::FormDescriptor(
        const std::string& name,
        const std::string& desc,
        const std::vector<Arg>& args)
    : name_(name), desc_(desc), args_(args) {
}

std::tuple<bool, Html, std::vector<std::string>>
        FormDescriptor::ValidateParams(const std::map<std::string, std::string>& vs) const {
    std::vector<std::string> args_values;
    bool error = false;
    Html html;

    for (auto& a : args_) {
        auto arg_value = vs.find(a.name());
        if (arg_value == vs.end()) {
            html << Div().AddClass("alert alert-danger")
                << a.name() << " was not provided." <<
            Close();
            error = true;
        } else {
            args_values.push_back(arg_value->second);
        }
    }

    return std::make_tuple(error, html, args_values);
}

} // html
} // httpi
