add_library(httpi
    chart.cpp
    chart.h
    displayer.cpp
    displayer.h
    html.h
    job.cpp
    job.h)

add_executable(test
    main.cpp)

target_link_libraries(test LINK_PUBLIC httpi
    microhttpd glog gflags)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
