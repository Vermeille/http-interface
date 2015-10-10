#pragma once

#include <algorithm>
#include <string>
#include <functional>
#include <map>
#include <boost/circular_buffer.hpp>

typedef std::map<std::string, std::string> POSTValues ;

#include "html.h"
#include "job.h"

typedef std::function<Html(const std::string&, const POSTValues&)> UrlHandler;
typedef std::map<std::string, boost::circular_buffer<size_t>> DataLog;

const DataLog& GetDataLog();

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

bool InitHttpInterface();
void StopHttpInterface();
void ServiceLoopForever();

void RegisterUrl(const std::string& str, const UrlHandler& f);
void RegisterJob(const JobDesc& jd);

void SetStatusVar(const std::string& name, const std::string& value);
void LogData(const std::string& name, size_t value);

