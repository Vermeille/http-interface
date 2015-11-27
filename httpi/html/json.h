#pragma once

#include <string>
#include <utility>

std::string ToJsonString(int x) { return std::to_string(x); }
std::string ToJsonString(double x) { return std::to_string(x); }
std::string ToJsonString(const std::string& x) { return "\"" + x + "\""; }

class JsonBuilder {
    std::string json_;

  public:
    JsonBuilder& Append(const std::string& key, int x) {
        if (!json_.empty()) {
            json_ += ", ";
        }

        json_ += "\"" + key + "\": " + std::to_string(x);
        return *this;
    }

    JsonBuilder& Append(const std::string& key, double x) {
        if (!json_.empty()) {
            json_ += ", ";
        }

        json_ += "\"" + key + "\": " + std::to_string(x);
        return *this;
    }

    JsonBuilder& Append(const std::string& key, const std::string& x) {
        if (!json_.empty()) {
            json_ += ", ";
        }

        json_ += "\"" + key + "\": \"" + x + "\"";
        return *this;
    }

    JsonBuilder& Append(const std::string& key, const JsonBuilder& x) {
        if (!json_.empty()) {
            json_ += ", ";
        }

        json_ += "\"" + key + "\": " + x.Build() + "";
        return *this;
    }

    template <class Container>
    JsonBuilder& Append(const std::string& key, const Container& container) {
        if (!json_.empty()) {
            json_ += ", ";
        }

        json_ += "\"" + key + "\": ";

        if (container.size() == 0) {
            json_ += "[]";
        } else if (container.size() == 1) {
            json_ += "[" + ToJsonString(container[0]) + "]";
        } else {
            json_ += "[" + ToJsonString(container[0]);

            for (size_t i = 1; i < container.size(); ++i) {
                json_ += ", " + container[i];
            }

            json_ += "]";
        }

        return *this;
    }

    std::string Build() const { return "{" + json_ + "}"; }
};

std::string ToJsonString(const JsonBuilder& x) { return x.Build(); }
