#pragma once

#include <algorithm>
#include <string>
#include <functional>
#include <map>
#include <boost/circular_buffer.hpp>

typedef std::map<std::string, std::string> POSTValues ;
typedef std::function<std::string(const std::string&, const POSTValues&)> UrlHandler;

template <class T>
struct ToString_ { static std::string Do(T v) { return std::to_string(v); } };

template <>
struct ToString_<std::string> { static std::string Do(const std::string& str) { return str; } };

template <>
struct ToString_<char*> { static std::string Do(char* str) { return str; } };

template <class T> std::string ToString(T v) {
    return ToString_<T>::Do(v);
}

template <class T>
std::string Intersperse(const T& vs, const std::string& sep) {
    return std::accumulate(vs.begin() + 1, vs.end(), ToString(vs[0]),
            [&sep](const std::string& str, const typename T::value_type& v) {
                return str + sep + ToString(v);
            }
        );
}

template <class T>
std::string ToCSV(const T& vs) { return Intersperse(vs, ", "); }

bool InitHttpInterface();  // call in the beginning of main()
void StopHttpInterface();  // may be ueless depending on the use case
void ServiceLoopForever();  // a convenience function for a proper event loop

void RegisterUrl(const std::string& str, const UrlHandler& f);  // call f is url is accessed

// set an overridable value that you can see on the status page. Useful for non numeric info
void SetStatusVar(const std::string& name, const std::string& value, size_t id = 0);

bool ServiceRuns();

