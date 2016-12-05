#include <signal.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "displayer.h"
#include "job.h"

struct ConnInfo {
    MHD_PostProcessor* post;
    std::string page;
    POSTValues args;

    ConnInfo() : post(nullptr) {}
};

static void request_completed(void* /* cls */,
                              struct MHD_Connection* /* connection */,
                              void** con_cls,
                              enum MHD_RequestTerminationCode /* toe */) {
    ConnInfo* info = static_cast<ConnInfo*>(*con_cls);
    MHD_destroy_post_processor(info->post);
    delete info;
}

static int iterate_post(void* coninfo_cls,
                        enum MHD_ValueKind,
                        const char* key,
                        const char* /* filename */,
                        const char* /* content_type */,
                        const char* /* transfer_encoding */,
                        const char* data,
                        uint64_t /* off */,
                        size_t size) {
    POSTValues& args = *static_cast<POSTValues*>(coninfo_cls);

    if (size > 0) {
        std::string& value = args[key];
        value.append(data, size);
    }

    return MHD_YES;
}

std::string HTTPServer::Execute(const std::string& url,
                                const std::string& method,
                                const POSTValues& pv) {
    std::unique_lock<std::mutex> lk(cb_mutex_);
    auto res = callbacks_.find(url);
    if (res != callbacks_.end()) {
        return res->second(method, pv);
    }
    return error404_;
}

static int answer_to_connection(void* cls,
                                struct MHD_Connection* connection,
                                const char* url,
                                const char* m,
                                const char* /* version */,
                                const char* upload_data,
                                size_t* upload_data_size,
                                void** con_cls) {
    HTTPServer* srv = static_cast<HTTPServer*>(cls);
    ConnInfo* info = static_cast<ConnInfo*>(*con_cls);
    std::string method = m;
    if (*con_cls == nullptr) {
        *con_cls = new ConnInfo;
        info = static_cast<ConnInfo*>(*con_cls);
        info->post = MHD_create_post_processor(
            connection, 512, iterate_post, &info->args);
        return MHD_YES;
    }

    if (*upload_data_size != 0) {
        MHD_post_process(info->post, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    MHD_get_connection_values(
        connection,
        static_cast<MHD_ValueKind>(MHD_POSTDATA_KIND | MHD_GET_ARGUMENT_KIND |
                                   MHD_RESPONSE_HEADER_KIND | MHD_HEADER_KIND),
        [](void* cls, MHD_ValueKind, const char* k, const char* v) {
            POSTValues& post = *static_cast<POSTValues*>(cls);
            post[k] = v;
            return MHD_YES;
        },
        &info->args);

    info->page = srv->Execute(url, method, info->args);
    struct MHD_Response* response = MHD_create_response_from_buffer(
        info->page.size(), (void*)info->page.c_str(), MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_destroy_response(response);
    return ret;
}

void HTTPServer::RegisterUrl(const std::string& str, UrlHandler f) {
    std::unique_lock<std::mutex> lk(cb_mutex_);
    callbacks_.insert(std::make_pair(str, std::move(f)));
}

HTTPServer::~HTTPServer() { MHD_stop_daemon(daemon_); }

void HTTPServer::ServiceLoopForever() {
    std::unique_lock<std::mutex> lk(stop_mutex_);
    stop_signal_.wait(lk, [&]() { return !running_; });
}

HTTPServer::HTTPServer(int port)
    : error404_(
          "<html><head><title>Not found</title></head><body>Go "
          "away.</body></html>") {
    daemon_ = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                               port,
                               nullptr,
                               nullptr,
                               &answer_to_connection,
                               this,
                               MHD_OPTION_NOTIFY_COMPLETED,
                               request_completed,
                               nullptr,
                               MHD_OPTION_END);

    running_ = (nullptr != daemon_);
}
