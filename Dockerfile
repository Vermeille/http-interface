FROM ubuntu:14.04

ENV CC gcc
ENV CXX  g++

# What is the best practice regarding workdir?
WORKDIR /root

# TODO: this was intended as a very general dev platform, but the http interface doesn't need
# all of this. Purge the unnecessary
RUN apt-get update &&  apt-get install -y \
         automake \
         autoconf \
         autoconf-archive \
         cmake \
         gcc \
         g++ \
         git \
         libboost-all-dev \
         libgoogle-glog-dev \
         libgflags-dev \
         make \
         pkg-config

# Http Interface runs on GNU libmicrohttpd
RUN git clone https://github.com/Metaswitch/libmicrohttpd --depth=1 && \
    cd libmicrohttpd && \
    chmod +x ./configure && \
    ./configure --enable-doc=no && make && make install && \
    cd .. && \
    rm -rf libmicrohttpd

RUN apt-get install -y gdb valgrind

EXPOSE 8888

ADD . /root

RUN mkdir build && cd build && cmake .. && make

ENTRYPOINT ["valgrind", "./build/http-interface/test"]
