#include <sys/types.h>
#include <microhttpd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <memory>
#include <unistd.h>
#include <boost/circular_buffer.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h>
#include <ctime>
#include <fstream>

#include "displayer.h"

#define PORT 8888
#define NOT_FOUND_ERROR "<html><head><title>Not found</title></head><body>Go away.</body></html>"

static struct MHD_Daemon *g_daemon;
static std::thread* g_monitoring_thread;
static std::map<std::string, boost::circular_buffer<size_t>> g_data;
static std::map<std::string, std::string> g_status;
static std::map<std::string, UrlHandler> g_callbacks;
static std::string g_name;
static std::mutex g_data_access;

std::string Chart::Get() {
    return "<div class=\"col-md-6\">"
            "<h3>" + name_ + "</h3>"
            "<div class=\"ct-chart ct-golden-section\" id=\"" + name_ + "\"></div></div>"
        "<script>"
            "new Chartist.Line('#" + name_ + "', {"
                "labels: [" + ToCSV(g_data[label_]) + "].reverse(),"
                "series: [" +
                MAP_INTERSPERSE(values_, ", ", v, "[" + ToCSV(g_data[v]) + "].reverse()") +
                "]"
            "});"
        "</script>";
}

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

  std::cerr << key << "\n";
  if (size > 0)
  {
      std::string& value = args[key];

      value.append(data, size);
      std::cerr << key << ": " << value << std::endl;
  }

  return MHD_YES;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url,
        const char *method, const char* /* version */, const char *upload_data, size_t *upload_data_size,
        void **con_cls)
{
    ConnInfo* info = static_cast<ConnInfo*>(*con_cls);
    std::cerr << method << " " << url << " upload size: " << *upload_data_size << std::endl;
    if (*con_cls == nullptr) {
        info = new ConnInfo;
        *con_cls = info;
        if (strcmp(method, "POST") == 0) {
            info->post = MHD_create_post_processor(connection, 512, iterate_post, &info->args);
            return MHD_YES;
        }
    }

    if (*upload_data_size != 0) {
        MHD_post_process(info->post, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    auto res = g_callbacks.find(url);
    if (res != g_callbacks.end()) {
        MHD_get_connection_values(connection, MHD_POSTDATA_KIND,
                [](void* cls, MHD_ValueKind, const char* k, const char* v) {
                    POSTValues& post = *static_cast<POSTValues*>(cls);
                    std::cerr << k << ": " << v << std::endl;
                    post[k] = v;
                    return MHD_YES;
                }, &info->args);
        info->page =
            "<!DOCTYPE html>"
            "<html>"
               "<head>"
                    R"(<meta charset="utf-8")"
                    R"(<meta http-equiv="X-UA-Compatible" content="IE=edge">)"
                    R"(<meta name="viewport" content="width=device-width, initial-scale=1">)"
                    R"(<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css">)"
                    R"(<link rel="stylesheet" href="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.css">)"
                    R"(<script src="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.js"></script>)"
                "</head>"
                "<body lang=\"en\"><div class=\"container\">" +
                res->second(method, info->args) +
                "</div></body>"
            "</html>";

        struct MHD_Response *response = MHD_create_response_from_buffer(info->page.size(),
                (void*) info->page.c_str(), MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

        MHD_destroy_response(response);
        return ret;
    }
    return not_found_page(cls, connection);
}

std::string Status(const std::string& /* method */, const std::map<std::string, std::string>&) {
    std::string page;
    page +=
            "<ul>";
    g_data_access.lock();
    page += MAP_CONCAT(g_data, v, "<li>" + v.first + ": " + ToCSV(v.second) + "</li>")
        + "<div class=\"row\">"
        + Chart("monitoring_ram")
            .Label("Time")
            .Value("Available RAM")
            .Get()
        + Chart("monitoring_cpu")
            .Label("Time")
            .Value("CPU")
            .Get()
        + "</div><h1>Status</h1><ul>"
        + MAP_CONCAT(g_status, v, "<li>" + v.first + ": " + v.second + "</li>");
    g_data_access.unlock();
    page += "</ul>";

    return page;
}

std::string LandingPage(const std::string& /* method */, const std::map<std::string, std::string>&) {
    std::string page;
    page += "<ul>";
    g_data_access.lock();
    page += MAP_CONCAT(g_callbacks, v, "<li><a href=\"" + v.first + "\">" + v.first + "</a></li>");
    g_data_access.unlock();
    page += "</ul>";
    return page;
}

static bool g_continue = true;

void handle_sigint(int sig) {
    if (sig == SIGINT) {
        g_continue = false;
    }
}

struct Stats {
    size_t utime = 0;
    size_t stime = 0;
    size_t vsize = 0;
};

Stats GetMonitoringStats() {
    std::ifstream stat("/proc/self/stat");
    Stats s;

    int i = 1;
    while (stat) {
        if (i == 14)
            stat >> s.utime;
        else if (i == 15)
            stat >> s.stime;
        else if (i == 23)
            stat >> s.vsize;
        else if (i == 2 || i == 3) {
            std::string ignore;
            stat >> ignore;
        } else {
            size_t ignore;
            stat >> ignore;
        }

        ++i;
    }
    return s;
}

bool InitHttpInterface() {
    g_data_access.lock();
    g_data["Available RAM"].rset_capacity(20);
    g_data["CPU"].rset_capacity(20);
    g_data["Time"].rset_capacity(20);

    for (int i = 0; i < 20; ++i) {
        g_data["Available RAM"].push_front(0);
        g_data["CPU"].push_front(0);
        g_data["Time"].push_back(i * 20);
    }
    g_data_access.unlock();

    g_monitoring_thread = new std::thread ([](){
        size_t last_time = 0;
        while (true) {
            auto begin = std::chrono::system_clock::now();

            Stats s = GetMonitoringStats();

            g_data_access.lock();
            g_data["Available RAM"].push_front(s.vsize);
            g_data["CPU"].push_front((100 / 20) * ((double)s.utime +  s.stime - last_time)
                    / sysconf(_SC_CLK_TCK));
            g_data_access.unlock();

            last_time = s.utime + s.stime;

            std::this_thread::sleep_for(std::chrono::seconds(20)
                - (std::chrono::system_clock::now() - begin));
        }
    });

    signal(SIGINT, handle_sigint);

    g_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
            &answer_to_connection, NULL, MHD_OPTION_END,
            MHD_OPTION_NOTIFY_COMPLETED, request_completed,
            NULL, MHD_OPTION_END);

    if (NULL == g_daemon)
        return false;

    g_callbacks["/"] = &LandingPage;
    g_callbacks["/status"] = &Status;

    return true;
}

void RegisterUrl(const std::string& str, const UrlHandler& f) {
    g_data_access.lock();
    g_callbacks.insert(std::make_pair(str, f));
    g_data_access.unlock();
}

void StopHttpInterface() {
    delete g_monitoring_thread;
    MHD_stop_daemon(g_daemon);
}

void ServiceLoopForever() {
    while (g_continue) {
        pause();
    }
}

void SetStatusVar(const std::string& name, const std::string& value) {
    g_data_access.lock();
    g_status[name] = value;
    g_data_access.unlock();
}

void LogData(const std::string& name, size_t value) {
    g_data_access.lock();
    g_data[name].push_front(value);
    g_data_access.unlock();
}
