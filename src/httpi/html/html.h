#pragma once

#include <map>
#include <stack>
#include <string>

namespace httpi {
namespace html {
// Abstract a HTML tag.
// TODO: quote escape tags, etc
class Tag {
    std::string tag_;
    std::map<std::string, std::string> attributes_;
    bool autoclose_;

   public:
    Tag(const std::string& tag) : tag_(tag), autoclose_(false) {}

    Tag& Autoclose() {
        autoclose_ = true;
        return *this;
    }
    bool IsAutoclose() const { return autoclose_; }

    Tag& Attr(const std::string& attr, const std::string& val) {
        auto res = attributes_.insert(std::make_pair(attr, val));
        if (!res.second) res.first->second += " " + val;
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

struct Table : public Tag {
    Table() : Tag("table") {}
};
struct Td : public Tag {
    Td() : Tag("td") {}
};
struct Tr : public Tag {
    Tr() : Tag("tr") {}
};
struct Th : public Tag {
    Th() : Tag("th") {}
};
struct H1 : public Tag {
    H1() : Tag("h1") {}
};
struct H2 : public Tag {
    H2() : Tag("h2") {}
};
struct H3 : public Tag {
    H3() : Tag("h3") {}
};
struct Ul : public Tag {
    Ul() : Tag("ul") {}
};
struct Li : public Tag {
    Li() : Tag("li") {}
};
struct P : public Tag {
    P() : Tag("p") {}
};
struct A : public Tag {
    A() : Tag("a") {}
};
struct Div : public Tag {
    Div() : Tag("div") {}
};
struct Span : public Tag {
    Span() : Tag("span") {}
};
struct Form : public Tag {
    Form() : Tag("form") {}
    Form(const std::string& method, const std::string& action) : Tag("form") {
        Attr("method", method)
            .Attr("action", action)
            .Attr("enctype", "multipart/form-data");
    }
};
struct Input : public Tag {
    Input() : Tag("input") { Autoclose(); }
    Tag& Name(const std::string& name) { return Attr("name", name); }
};

struct InputNumber : public Input {
    InputNumber() : Input() { Attr("type", "number").Autoclose(); }
};
struct Submit : public Tag {
    Submit() : Tag("input") { Attr("type", "submit").Autoclose(); }
};

// I don't have any better, sadly
class Close {};

// An Html document stream-like. It also makes sure you don't forget to close
// your tags.
// TODO: escaping, handling more data types
class Html {
    std::stack<std::string> tags_;
    std::string content_;

   public:
    Html& operator<<(const Tag& t) {
        content_ += t.OpeningTag();
        if (!t.IsAutoclose()) tags_.push(t.GetTag());
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
        // LOG_IF(FATAL, !tags_.empty()) << "Tag " << tags_.top() << " not
        // closed for `" << content_ << "`";
        return content_;
    }
};

}  // html
}  // httpi
