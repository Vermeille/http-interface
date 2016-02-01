add_library(httpi
    httpi/html/html.h
    httpi/html/chart.cpp
    httpi/html/chart.h
    httpi/html/form-gen.h
    httpi/html/form-gen.cpp
    httpi/html/json.h
    httpi/displayer.cpp
    httpi/displayer.h
    httpi/job.h
    httpi/monitoring.h
    httpi/monitoring.cpp
    httpi/rest-helpers.h
    httpi/webjob.h
)

target_link_libraries(httpi LINK_PUBLIC microhttpd glog gflags pthread)
target_include_directories(httpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

