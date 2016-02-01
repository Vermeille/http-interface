#pragma once

#include <vector>
#include <string>
#include <map>
#include <utility>

#include "html.h"

namespace httpi {
namespace html {

template <class T>
T Convert(const std::string& str, std::vector<std::string>&);

template <>
std::string Convert<std::string>(
        const std::string& str,
        std::vector<std::string>&);

template <>
int Convert<int>(const std::string& str, std::vector<std::string>& err);

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

class FormSerializer {
    std::string method_;
    std::string url_;
    std::string name_;
    std::string desc_;
    std::vector<Arg> args_;

  public:
    FormSerializer(
            const std::string& method,
            const std::string& url,
            const std::string& name,
            const std::string& desc,
            const std::vector<Arg>& args);

    const std::vector<Arg> args() const { return args_; }

    Html MakeForm() const;
};

template <class... Args>
class FormDescriptor {
    FormSerializer form_;

  public:
    typedef std::tuple<Args...> args_type;
    typedef std::vector<std::string> errorlog_type;
    typedef std::map<std::string, std::string> input_type;

    FormDescriptor(
            const std::string& method,
            const std::string& url,
            const std::string& name,
            const std::string& desc,
            const std::vector<Arg>& args)
        : form_(method, url, name, desc, args) {
    }

    Html MakeForm() const {
        return form_.MakeForm();
    }

    std::pair<args_type, errorlog_type> Validate(const input_type& vs) const {
        std::vector<std::string> args_values;
        errorlog_type errors;

        for (auto& a : form_.args()) {
            auto arg_value = vs.find(a.name());
            if (arg_value == vs.end()) {
                errors.push_back(a.name() + " missing in argument list");
            } else {
                args_values.push_back(arg_value->second);
            }
        }

        if (!errors.empty()) {
            return std::make_pair(args_type(), errors);
        } else {
            auto params
                =  MakeParams(
                        args_values,
                        errors,
                        std::index_sequence_for<Args...>());
                return std::make_pair(params, errors);
        }
    }

  private:
    template <size_t... I>
    std::tuple<Args...> MakeParams(
            const std::vector<std::string>& vs,
            errorlog_type& error,
            std::index_sequence<I...>) const {
        return std::make_tuple<Args...>(Convert<Args>(vs[I], error)...);
    }
};

template <>
class FormDescriptor<> {
  public:
    typedef std::tuple<> args_type;
    typedef std::vector<std::string> errorlog_type;
    typedef std::map<std::string, std::string> input_type;

    Html MakeForm() const {
        return Html();
    }

    std::pair<args_type, errorlog_type> Validate(const input_type&) const {
        return std::make_pair(std::tuple<>(), errorlog_type());
    }
};

} // html
} // httpi
