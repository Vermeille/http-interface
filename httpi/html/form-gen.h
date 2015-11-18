#pragma once

#include <vector>
#include <string>
#include <map>

#include "html.h"

namespace httpi {
namespace html {

class Arg {
    std::string name_;  // formal name
    std::string type_;  // from now, it takes the possibles values of an HTML's input tag
    std::string desc_;  // short text describing the argument
  public:

    Arg(const std::string& name, const std::string& type, const std::string& desc);

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }

    Html ArgToForm() const;
};

class FormDescriptor {
    std::string name_;
    std::string desc_;
    std::vector<Arg> args_;

  public:
    FormDescriptor(const std::string& name, const std::string& desc, const std::vector<Arg>& args);
    Html MakeForm(const std::string& dst_url) const;
    std::tuple<bool, Html, std::vector<std::string>>
        ValidateParams(const std::map<std::string, std::string>& vs) const;
};

} // html
} // httpi
