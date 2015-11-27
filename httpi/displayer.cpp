#include <sys/types.h>
#include <microhttpd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <memory>
#include <unistd.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h>
#include <ctime>
#include <ctime>

#include <gflags/gflags.h>

#include "displayer.h"
#include "job.h"

DEFINE_int32(port, 8888, "the port to serve the http on");
#define NOT_FOUND_ERROR "<html><head><title>Not found</title></head><body>Go away.</body></html>"

/* WARNING / FIXME: race conditions everywhere, mutex badly used */

static struct MHD_Daemon *g_daemon;
static std::map<std::string, UrlHandler> g_callbacks;

struct ConnInfo {
    MHD_PostProcessor* post;
    std::string page;
    POSTValues args;
};

static void
request_completed(void* /* cls */, struct MHD_Connection* /* connection */,
                   void **con_cls, enum MHD_RequestTerminationCode /* toe */) {
    ConnInfo* info = static_cast<ConnInfo*>(*con_cls);
    MHD_destroy_post_processor(info->post);
    delete info;
}

static int not_found_page(const void* /* cls */, struct MHD_Connection *connection) {
    int ret;
    struct MHD_Response *response;

    /* unsupported HTTP method */
    response = MHD_create_response_from_buffer(strlen(NOT_FOUND_ERROR),
            (void *) NOT_FOUND_ERROR, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, nullptr);
    MHD_destroy_response(response);
    return ret;
}

static int
iterate_post(void* coninfo_cls, enum MHD_ValueKind, const char* key,
              const char* /* filename */, const char* /* content_type */,
              const char* /* transfer_encoding */, const char* data, uint64_t /* off */,
              size_t size) {
  POSTValues& args = *static_cast<POSTValues*>(coninfo_cls);;

  if (size > 0) {
      std::string& value = args[key];
      value.append(data, size);
  }

  return MHD_YES;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url,
        const char *m, const char* /* version */, const char *upload_data, size_t *upload_data_size,
        void **con_cls)
{
    std::string method = m;
    ConnInfo* info = static_cast<ConnInfo*>(*con_cls);
    std::cerr << method << " " << url << " upload size: " << *upload_data_size << std::endl;
    if (*con_cls == nullptr) {
        info = new ConnInfo;
        *con_cls = info;
        if (method != "GET") {
            info->post = MHD_create_post_processor(connection, 512, iterate_post, &info->args);
            return MHD_YES;
        }
    }

    if (*upload_data_size != 0) {
        MHD_post_process(info->post, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    MHD_get_connection_values(connection,
            (MHD_ValueKind)
                ((int)MHD_POSTDATA_KIND
                 | (int)MHD_GET_ARGUMENT_KIND
                 | (int)MHD_RESPONSE_HEADER_KIND
                 | (int)MHD_HEADER_KIND),
            [](void* cls, MHD_ValueKind, const char* k, const char* v) {
                POSTValues& post = *static_cast<POSTValues*>(cls);
                post[k] = v;
                return MHD_YES;
            }, &info->args);
    auto res = g_callbacks.find(url);
    auto new_method = info->args.find("_method");
    if (new_method != info->args.end()) {
        method = new_method->second;
    }

    if (res != g_callbacks.end()) {
        info->page = res->second(method, info->args);
    }

    if (info->page.empty()) {
        return not_found_page(cls, connection);
    } else {
        struct MHD_Response *response = MHD_create_response_from_buffer(info->page.size(),
                (void*) info->page.c_str(), MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

        MHD_destroy_response(response);
        return ret;
    }
}

static bool g_continue = true;

bool IsServiceRunning() { return g_continue; }

void handle_sigint(int sig) {
    if (sig == SIGINT) {
        g_continue = false;
    }
}

void RegisterUrl(const std::string& str, const UrlHandler& f) {
    g_callbacks.insert(std::make_pair(str, f));
}

void StopHttpInterface() {
    MHD_stop_daemon(g_daemon);
}

void ServiceLoopForever() {
    while (g_continue) {
        pause();
    }
}

bool InitHttpInterface() {
    g_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, FLAGS_port, NULL, NULL,
            &answer_to_connection, NULL, MHD_OPTION_END,
            MHD_OPTION_NOTIFY_COMPLETED, request_completed,
            NULL, MHD_OPTION_END);

    signal(SIGINT, handle_sigint);

    if (NULL == g_daemon)
        return false;

    return true;
}

