#pragma once

#include <microhttpd.h>
#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>

typedef std::map<std::string, std::string> POSTValues;
typedef std::function<std::string(const std::string&, const POSTValues&)>
    UrlHandler;

class HTTPServer {
   public:
    HTTPServer(int port);
    ~HTTPServer();
    void ServiceLoopForever();

    void RegisterUrl(const std::string& str, UrlHandler f);

    void StopService() {
        running_ = false;
        stop_signal_.notify_all();
    }

    std::string Execute(const std::string& url,
                        const std::string& method,
                        const POSTValues& pv);

   private:
    MHD_Daemon* daemon_;
    bool running_;
    mutable std::mutex cb_mutex_;
    std::mutex stop_mutex_;
    std::condition_variable stop_signal_;
    std::map<std::string, UrlHandler> callbacks_;
    std::string error404_;
};
