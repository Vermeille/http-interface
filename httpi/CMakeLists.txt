add_library(httpi
    html/html.h
    html/chart.cpp
    html/chart.h
    html/form-gen.h
    html/form-gen.cpp
    html/json.h
    displayer.cpp
    displayer.h
    job.h
    monitoring.h
    monitoring.cpp
    rest-helpers.h
    webjob.h)

add_executable(test
    main.cpp)

target_link_libraries(test LINK_PUBLIC httpi
    microhttpd glog gflags)
target_link_libraries(httpi LINK_PUBLIC microhttpd glog gflags)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14")

install(TARGETS httpi
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY . DESTINATION include/httpi
    FILES_MATCHING PATTERN "*.h")
