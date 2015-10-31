#pragma once

#include <algorithm>
#include <string>
#include <functional>
#include <map>
#include <boost/circular_buffer.hpp>

typedef std::map<std::string, std::string> POSTValues ;

#include "html.h"

typedef std::function<Html(const std::string&, const POSTValues&)> UrlHandler;
typedef std::map<std::string, boost::circular_buffer<size_t>> DataLog;

const DataLog& GetDataLog(size_t id = 0);

#define MAP_INTERSPERSE(C, S, I, B) \
    MapIntersperse(C, S, [](const typename decltype (C)::value_type& I) { return B; })

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

bool InitHttpInterface();  // call in the beginning of main()
void StopHttpInterface();  // may be ueless depending on the use case
void ServiceLoopForever();  // a convenience function for a proper event loop

void RegisterUrl(const std::string& str, const UrlHandler& f);  // call f is url is accessed

// set an overridable value that you can see on the status page. Useful for non numeric info
void SetStatusVar(const std::string& name, const std::string& value, size_t id = 0);

bool ServiceRuns();
