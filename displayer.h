#pragma once

#include <algorithm>
#include <string>
#include <functional>
#include <map>
#include <stack>

#include <glog/logging.h>

class Tag {
    std::string tag_;
    std::map<std::string, std::string> attributes_;
    bool autoclose_;

    public:
        Tag(const std::string& tag) : tag_(tag), autoclose_(false) {}

        Tag& Autoclose() { autoclose_ = true; return *this; }
        bool IsAutoclose() const { return autoclose_; }

        Tag& Attr(const std::string& attr, const std::string& val) {
            auto res = attributes_.insert(std::make_pair(attr, val));
            if (!res.second)
                res.first->second += " " + val;
            return *this;
        }

        Tag& AddClass(const std::string& cl) { return Attr("class", cl); }

        Tag& Id(const std::string& id) { return Attr("id", id); }

        std::string GetTag() const { return tag_; }
        std::string OpeningTag() const {
            std::string tag = "<" + tag_;
            for (auto& attr : attributes_)
                tag += " " + attr.first + "=\"" + attr.second + "\"";
            tag += autoclose_ ? "/>" : ">";
            return tag;
        }
};

struct H1 : public Tag { H1() : Tag("h1") {} };
struct H2 : public Tag { H2() : Tag("h2") {} };
struct Ul : public Tag { Ul() : Tag("ul") {} };
struct Li : public Tag { Li() : Tag("li") {} };
struct P : public Tag { P() : Tag("p") {} };
struct A : public Tag { A() : Tag("a") {} };
struct Div : public Tag { Div() : Tag("div") {} };
struct Form : public Tag {
    Form() : Tag("form") {}
    Form(const std::string& method, const std::string& action) : Tag("form") {
        Attr("method", method).Attr("action", action);
    }
};
struct InputNumber : public Tag {
    InputNumber() : Tag("input") {
        Attr("type", "number").Autoclose();
    }
    Tag& Name(const std::string& name) { return Attr("name", name); }
};
struct Submit : public Tag { Submit() : Tag("input") { Attr("type", "submit").Autoclose(); } };

class Close {};

class Html {
        std::stack<std::string> tags_;
        std::string content_;
    public:

        Html& operator<<(const Tag& t) {
            content_ += t.OpeningTag();
            if (!t.IsAutoclose())
                tags_.push(t.GetTag());
            return *this;
        }

        Html& operator<<(const Close&) {
            content_ += "</" + tags_.top() + ">";
            tags_.pop();
            return *this;
        }

        Html& operator<<(const std::string& str) {
            content_ += str;
            return *this;
        }

        Html& operator<<(const Html& html) {
            content_ += html.Get();
            return *this;
        }

        const std::string& Get() const {
            LOG_IF(FATAL, !tags_.empty()) << "Tag " << tags_.top() << " not closed.";
            return content_;
        }
};

typedef std::map<std::string, std::string> POSTValues ;
typedef std::function<Html(const std::string&, const POSTValues&)> UrlHandler;

#define MAP_CONCAT(C, I, B) MapConcat(C, [] (const typename decltype (C)::value_type& I) { return B; })
#define MAP_INTERSPERSE(C, S, I, B) \
    MapIntersperse(C, S, [](const typename decltype (C)::value_type& I) { return B; })

template <class T>
std::string Intersperse(const T& vs, const std::string& sep) {
    return std::accumulate(vs.begin() + 1, vs.end(), std::to_string(vs[0]),
            [&sep](const std::string& str, const typename T::value_type& v) {
                return str + sep + std::to_string(v);
            }
        );
}

template <class T, class F>
std::string MapIntersperse(const T& vs, const std::string& sep, F&& f) {
    return std::accumulate(vs.begin() + 1, vs.end(), f(vs[0]),
            [&sep, &f](const std::string& str, const typename T::value_type& v) {
                return str + sep + f(v);
            }
        );
}

template <class T>
std::string ToCSV(const T& vs) { return Intersperse(vs, ", "); }

template <class T, class F>
std::string MapConcat(const T& vs, F&& f) {
    return std::accumulate(vs.begin(), vs.end(), std::string(""),
            [&f](const std::string& str, const typename T::value_type& v) {
                return str + f(v);
            }
        );
}

template <class T>
std::string Concat(const T& vs) {
    return std::accumulate(vs.begin(), vs.end(), std::string(""));
}

class Chart {
    std::string label_;
    std::vector<std::string> values_;
    const std::string& name_;

    public:
    Chart(const std::string& name) : name_(name) {}

    Chart& Label(const std::string& l) { label_ = l; return *this; }
    Chart& Value(const std::string& v) { values_.push_back(v); return *this; }
    std::string Get();
};

bool InitHttpInterface();
void StopHttpInterface();
void ServiceLoopForever();

void RegisterUrl(const std::string& str, const UrlHandler& f);

void SetStatusVar(const std::string& name, const std::string& value);
void LogData(const std::string& name, size_t value);
