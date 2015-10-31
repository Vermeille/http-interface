add_library(httpi
    chart.cpp
    chart.h
    displayer.cpp
    displayer.h
    html.h
    job.cpp
    job.h
    monitoring.cpp)

add_executable(test
    main.cpp)

target_link_libraries(test LINK_PUBLIC httpi
    microhttpd glog gflags)
target_link_libraries(httpi LINK_PUBLIC microhttpd glog gflags)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

install(TARGETS httpi
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

file(GLOB headers *.h)
install(FILES ${headers} DESTINATION include/httpi)
