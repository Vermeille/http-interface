#pragma once

#include <algorithm>
#include <string>
#include <functional>
#include <map>
#include <boost/circular_buffer.hpp>

typedef std::map<std::string, std::string> POSTValues ;
typedef std::function<std::string(const std::string&, const POSTValues&)> UrlHandler;

bool InitHttpInterface();  // call in the beginning of main()
void StopHttpInterface();  // may be ueless depending on the use case
void ServiceLoopForever();  // a convenience function for a proper event loop

void RegisterUrl(const std::string& str, const UrlHandler& f);  // call f is url is accessed

bool IsServiceRunning();

