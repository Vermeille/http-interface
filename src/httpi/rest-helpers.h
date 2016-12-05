#pragma once

#include <string>

#include "displayer.h"
#include "html/html.h"

namespace httpi {

namespace {

template <class F, class... Args, size_t... I>
auto CurryCallImpl(F&& f,
                   std::tuple<Args...>&& args,
                   std::index_sequence<I...>) {
    return f(std::get<I>(std::forward<Args>(args))...);
}

template <class F, class... Args, size_t... I>
auto CurryCallImpl(F&& f,
                   const std::tuple<Args...>& args,
                   std::index_sequence<I...>) {
    return f(std::get<I>(args)...);
}

template <class F, class... Args>
auto CurryCall(F&& f, const std::tuple<Args...>& args) {
    return CurryCallImpl(f, args, std::index_sequence_for<Args...>());
}

template <class F, class... Args>
auto CurryCall(F&& f, std::tuple<Args...>&& args) {
    return CurryCallImpl(f,
                         std::forward<std::tuple<Args...>>(args),
                         std::index_sequence_for<Args...>());
}

template <class>
static constexpr int is_tuple_v = false;

template <class... Args>
static constexpr int is_tuple_v<std::tuple<Args...>> = true;

template <class F, class Args>
auto CurryCall(F&& f, std::enable_if_t<!is_tuple_v<Args>, Args>&& args) {
    return f(std::forward<Args>(args));
}
}  // anonymous

class RestResource {
    class ResourceAccessor {
       public:
        virtual std::string HtmlProcess(const POSTValues&) const = 0;
        virtual std::string JsonProcess(const POSTValues&) const = 0;
        virtual std::string MakeForm() const = 0;
    };

    template <class Serializer,
              class Exec,
              class HtmlRenderer,
              class JsonRenderer>
    class ResourceImpl : public ResourceAccessor {
        Serializer serializer_;
        Exec exec_;
        HtmlRenderer html_;
        JsonRenderer json_;

       public:
        // FIXME: I don't belong here
        httpi::html::Html ErrorsToHtml(
            const std::vector<std::string>& errs) const {
            httpi::html::Html html;
            for (const auto& e : errs) {
                html << httpi::html::Div().AddClass("alert") << e
                     << httpi::html::Close();
            }
            return html;
        }

        ResourceImpl(Serializer&& s,
                     Exec&& e,
                     HtmlRenderer&& html,
                     JsonRenderer&& json)
            : serializer_(s), exec_(e), html_(html), json_(json) {}

        virtual std::string HtmlProcess(const POSTValues& post_args) const {
            httpi::html::Html html;
            auto args = serializer_.Validate(post_args);
            if (!args.second.empty()) {
                html << ErrorsToHtml(args.second);
            } else {
                html << html_(CurryCall(exec_, args.first));
            }
            return html.Get();
        }

        virtual std::string JsonProcess(const POSTValues& post_args) const {
            auto args = serializer_.Validate(post_args);
            if (!args.second.empty()) {
                return ErrorsToHtml(args.second).Get();
            } else {
                return json_(CurryCall(exec_, args.first));
            }
        }

        virtual std::string MakeForm() const {
            return serializer_.MakeForm().Get();
        }
    };

    std::shared_ptr<ResourceAccessor> rs_accessor_;

   public:
    std::string JsonProcess(const POSTValues& args) const {
        return rs_accessor_->JsonProcess(args);
    }

    std::string HtmlProcess(const POSTValues& args) const {
        return rs_accessor_->HtmlProcess(args);
    }

    std::string MakeForm() const { return rs_accessor_->MakeForm(); }

    template <class Serializer,
              class Exec,
              class HtmlRenderer,
              class JsonRenderer>
    RestResource(Serializer&& s,
                 Exec&& e,
                 HtmlRenderer&& html,
                 JsonRenderer&& json)
        : rs_accessor_(
              std::make_unique<
                  ResourceImpl<Serializer, Exec, HtmlRenderer, JsonRenderer>>(
                  std::forward<Serializer>(s),
                  std::forward<Exec>(e),
                  std::forward<HtmlRenderer>(html),
                  std::forward<JsonRenderer>(json))) {}
};

class RestPageMaker {
    std::map<std::string, RestResource> resources_;
    std::function<std::string(const std::string&)> theme_;

   public:
    RestPageMaker(const decltype(theme_)& theme) : theme_(theme) {}

    RestPageMaker(const RestPageMaker& theme) = default;

    RestPageMaker& AddResource(const std::string& method,
                               const RestResource& res) {
        resources_.emplace(method, res);
        return *this;
    }

    std::string operator()(const std::string& method,
                           const POSTValues& args) const {
        auto content_type = args.find("Accept");
        if (content_type != args.end() &&
            content_type->second == "application/json") {
            return JsonProcess(method, args);
        } else {
            return HtmlProcess(method, args);
        }
    }

    std::string JsonProcess(const std::string& method,
                            const POSTValues& args) const {
        using namespace httpi::html;
        auto resource = resources_.find(method);
        if (resource == resources_.end()) {
            return (Html() << Div().AddClass("alert alert-red")
                           << "method not supported"
                           << Close())
                .Get();
        }
        std::string json = resource->second.JsonProcess(args);
        return json + "\n";
    }

    std::string HtmlProcess(const std::string& method,
                            const POSTValues& args) const {
        using namespace httpi::html;
        Html html;

        auto resource = resources_.find(method);
        if (resource == resources_.end()) {
            return (html << Div().AddClass("alert alert-red")
                         << "method not supported"
                         << Close())
                .Get();
        }

        html << resource->second.HtmlProcess(args);

        for (const auto& r : resources_) {
            html << r.second.MakeForm();
        }

        return theme_(html.Get()) + "\n";
    }
};

}  // httpi
