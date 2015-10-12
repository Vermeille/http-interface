FROM ubuntu:14.04

ENV CC gcc
ENV CXX  g++

# What is the best practice regarding workdir?
WORKDIR /root

# TODO: this was intended as a very general dev platform, but the http interface doesn't need
# all of this. Purge the unnecessary
RUN apt-get update &&  apt-get install -y \
         bison \
         cmake \
         flex \
         gcc \
         g++ \
         git \
         automake \
         autoconf \
         autoconf-archive \
         libtool \
         libboost-all-dev \
         libevent-dev \
         libdouble-conversion-dev \
         libgoogle-glog-dev \
         libgflags-dev \
         liblz4-dev \
         liblzma-dev \
         libsnappy-dev \
         make \
         zlib1g-dev \
         binutils-dev \
         libiberty-dev \
         libjemalloc-dev \
         libssl-dev \
         libkrb5-dev \
         libsasl2-dev \
         libnuma-dev \
         pkg-config \
         libssl-dev

# Http Interface runs on GNU libmicrohttpd
RUN git clone https://github.com/Metaswitch/libmicrohttpd --depth=1 && \
    cd libmicrohttpd && \
    chmod +x ./configure && \
    ./configure --enable-doc=no && make && make install && \
    cd .. && \
    rm -rf libmicrohttpd

# folly is not necessary for http interface
RUN git clone https://github.com/facebook/folly && \
    cd folly/folly && \
    git checkout v0.57.0 && \
    autoreconf -if && ./configure && make -j6 && make install && \
    cd ../.. && \
    rm -rf folly

# Haven't dived much into wangle, but I'm sure we could use it for http interface and remove a lot
# of code, for a much better quality
RUN git clone https://github.com/facebook/wangle --depth=1 && \
    cd wangle/wangle && \
    cmake . && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf wangle

# thrift is not used yet
RUN git clone https://github.com/facebook/fbthrift --depth=1 && \
    cd fbthrift/thrift && \
    ./build/deps_ubuntu_14.04.sh && \
    autoreconf -if && ./configure && make && \
    cd ../.. && \
    rm -rf fbthrift

RUN apt-get install -y gdb valgrind

EXPOSE 8888

ADD . /root

RUN make

ENTRYPOINT ["valgrind", "./displayer"]
