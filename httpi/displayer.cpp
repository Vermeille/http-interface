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

#include "displayer.h"
#include "chart.h"
#include "job.h"

DEFINE_int32(port, 8888, "the port to serve the http on");
#define NOT_FOUND_ERROR "<html><head><title>Not found</title></head><body>Go away.</body></html>"

/* WARNING / FIXME: race conditions everywhere, mutex badly used */

static struct MHD_Daemon *g_daemon;
static std::thread* g_monitoring_thread;
static std::map<size_t, std::map<std::string, std::string>> g_status;
static std::map<std::string, UrlHandler> g_callbacks;
static std::mutex g_data_access;
static RunningJobs g_jobs;
static int g_monitoring_id;

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

Html LandingPage(const std::string& /* method */, const std::map<std::string, std::string>&);

std::string MakePage(const std::string& content) {
    return (Html() <<
        "<!DOCTYPE html>"
        "<html>"
           "<head>"
                R"(<meta charset="utf-8">)"
                R"(<meta http-equiv="X-UA-Compatible" content="IE=edge">)"
                R"(<meta name="viewport" content="width=device-width, initial-scale=1">)"
                R"(<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css">)"
                R"(<link rel="stylesheet" href="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.css">)"
                R"(<script src="//cdn.jsdelivr.net/chartist.js/latest/chartist.min.js"></script>)"
            "</head>"
            "<body lang=\"en\">"
                "<div class=\"container\">"
                    "<div class=\"col-md-9\">" <<
                        content <<
                    "</div>"
                    "<div class=\"col-md-3\">" <<
                        LandingPage("GET", {}) <<
                    "</div>"
                "</div>"
            "</body>"
        "</html>").Get();
}

std::string MakePage(const Html& content) {
    return MakePage(content.Get());
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

    MHD_get_connection_values(connection, (MHD_ValueKind)((int)MHD_POSTDATA_KIND | (int)MHD_GET_ARGUMENT_KIND),
            [](void* cls, MHD_ValueKind, const char* k, const char* v) {
                POSTValues& post = *static_cast<POSTValues*>(cls);
                std::cerr << k << ": " << v << std::endl;
                post[k] = v;
                return MHD_YES;
            }, &info->args);
    auto res = g_callbacks.find(url);
    if (res != g_callbacks.end()) {
        info->page = MakePage(res->second(method, info->args));
    }

    auto j_res = g_jobs.FindDescriptor(url);

    if (j_res) {
        if (!strcmp(method, "GET")) {
            info->page = MakePage(j_res->MakeForm());
        } else {
            info->page = MakePage(g_jobs.Exec(url, info->args));
        }
    }

    std::lock_guard<std::mutex> lock(g_data_access);
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

Html Status(const std::string& /* method */, const std::map<std::string, std::string>&) {
    const JobStatus* rj = g_jobs.FindJobWithId(g_monitoring_id);
    return Html() << *rj->result();
}

Html LandingPage(const std::string& /* method */, const std::map<std::string, std::string>&) {
    Html html;
    html << H3() << "Pages" << Close();
    html << Ul();
    g_data_access.lock();
    for (auto& v : g_callbacks)
        html << Li() << A().Attr("href", v.first) << v.first << Close() << Close();
    html <<
        Close() <<
        H3() << "Jobs" << Close()  <<
        g_jobs.RenderListOfDescriptors();
    g_data_access.unlock();
    return html;
}

Html JobStatuses(const std::string& /* method */, const POSTValues&) {
    return Html() << H2() << "Running jobs" << Close() << g_jobs.RenderTableOfRunningJobs();
}

Html JobDetails(const std::string& /* method */, const POSTValues& vs) {
    auto id = vs.find("id");

    if (id == vs.end()) {
        return Html() << "This job does not exist";
    }

    const JobStatus* rj = g_jobs.FindJobWithId(std::atoi(id->second.c_str()));

    if (!rj) {
        return Html() << "job not found";
    }

    Html html;
    html <<
        H1() << "Job Details" << Close() <<
        Div() <<
            P() << "Started on: " << rj->start_time() << Close() <<
            H2() << "Status Variables" << Close() <<
            Ul();

    for (auto& v : g_status[rj->id()]) {
        html << Li() << v.first + ": " + v.second << Close();
    }
    html << Close();

    html << *rj->result();

    html << Close();
    return html;
}

static bool g_continue = true;

bool ServiceRuns() { return g_continue; }

void handle_sigint(int sig) {
    if (sig == SIGINT) {
        g_continue = false;
    }
}

void RegisterUrl(const std::string& str, const UrlHandler& f) {
    g_data_access.lock();
    g_callbacks.insert(std::make_pair(str, f));
    g_data_access.unlock();
}

void RegisterJob(const JobDesc& jd) {
    g_data_access.lock();
    g_jobs.AddDescriptor(jd);
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

void SetStatusVar(const std::string& name, const std::string& value, size_t id) {
    g_data_access.lock();
    g_status[id][name] = value;
    g_data_access.unlock();
}

void MonitoringJob(const std::vector<std::string>& vs, JobStatus& job);

static const JobDesc monitoring_job = {
    { },  // args
    "",  // name
    "",  // url
    "",  // longer description
    true /* synchronous */,
    true /* reentrant */,
    &MonitoringJob
};

bool InitHttpInterface() {
    google::InstallFailureSignalHandler();

    g_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, FLAGS_port, NULL, NULL,
            &answer_to_connection, NULL, MHD_OPTION_END,
            MHD_OPTION_NOTIFY_COMPLETED, request_completed,
            NULL, MHD_OPTION_END);

    signal(SIGINT, handle_sigint);

    if (NULL == g_daemon)
        return false;

    g_monitoring_id = g_jobs.StartJob(&monitoring_job, {});

    g_callbacks["/"] = &LandingPage;
    g_callbacks["/status"] = &Status;
    g_callbacks["/jobs"] = &JobStatuses;
    g_callbacks["/job"] = &JobDetails;

    return true;
}

