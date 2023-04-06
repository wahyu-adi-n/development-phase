FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8
ENV TZ Asia/Jakarta
# Boost library
RUN apt-get -y update && \
    apt-get -y upgrade && \
    apt-get -y dist-upgrade && \
    apt-get -y autoremove && \
    apt-get install -y build-essential gdb wget git libssl-dev && \
    mkdir ~/temp && cd ~/temp && \
    wget https://github.com/Kitware/CMake/releases/download/v3.21.4/cmake-3.21.4.tar.gz && \
    tar -zxvf cmake-3.21.4.tar.gz && \
    cd cmake-3.21.4 && \
    ./bootstrap && make -j4 && make install && \
    rm -rf ~/temp/* && \
    cd ~/temp &&  wget https://sourceforge.net/projects/boost/files/boost/1.73.0/boost_1_73_0.tar.gz && \
    tar -zxvf boost_1_73_0.tar.gz && cd boost_1_73_0 && ./bootstrap.sh && ./b2 cxxflags="-std=c++17" --reconfigure --with-fiber --with-date_time install && \
    cd ~/temp && git clone https://github.com/linux-test-project/lcov.git && cd lcov && make install && cd .. && \
    apt-get install -y libperlio-gzip-perl libjson-perl && \
    rm -rf ~/temp/* && \
    apt-get autoremove -y &&\
    apt-get clean -y &&\
    rm -rf /var/lib/apt/lists/*
# libpq
RUN apt-get -y update && \
    apt-get install -y libpq-dev libsqlite3-dev unzip && \
    cd ~/temp && \
    git clone https://github.com/jtv/libpqxx.git && cd libpqxx && \
    git checkout 7.7.4 && \
    mkdir build && cd build && \
    cmake .. -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/libpq && \
    make -j6 && make install && \
    cd ~/temp && \
    wget https://github.com/SOCI/soci/archive/refs/heads/master.zip && \
    unzip master.zip && \
    cd soci-master && \
    mkdir build && cd build && \
    cmake .. -DWITH_BOOST=ON -DWITH_POSTGRESQL=ON -DWITH_SQLITE3=ON -DCMAKE_CXX_STANDARD=14 -DSOCI_CXX11=ON && \
    make -j6 && make install && \
    # cp /usr/local/cmake/SOCI.cmake /usr/local/cmake/SOCIConfig.cmake && \
    ln -s /usr/local/lib64/libsoci_* /usr/local/lib && ldconfig && \
    rm -rf ~/temp/* && \
    apt-get autoremove -y &&\
    apt-get clean -y &&\
    rm -rf /var/lib/apt/lists/*
# Libasyik
RUN cd ~/temp  && \
    git clone https://github.com/okyfirmansyah/libasyik && \
    cd ~/temp/libasyik && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j4 && \
    make install

RUN apt-get update && \
    apt-get install -y python3-pip curl jq

RUN mkdir /app

RUN cd ~/temp && \
    wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.12.1%2Bcpu.zip && \
    unzip libtorch-cxx11-abi-shared-with-deps-1.12.1+cpu && \
    cp -r libtorch /app && \
    rm -rf ~/temp/*

COPY . /app

WORKDIR /app

RUN export NLOHMANN_JSON_VER=$(curl -sL https://api.github.com/repos/nlohmann/json/releases/latest | jq -r ".tag_name") && \
    wget -N -P /app/include "https://github.com/nlohmann/json/releases/download/${NLOHMANN_JSON_VER}/json.hpp"

RUN --mount=type=cache,target=/root/.cache \
    pip3 install --user -r requirements.txt

RUN --mount=type=cache,target=/root/.cache \
    apt-get update && \
    apt-get install -y libopencv-dev

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

ARG EXPOSE_PORT
ENV PORT=$EXPOSE_PORT

EXPOSE ${PORT}
RUN ls

RUN cd models && \
    ls

CMD ["./build/development-phase"]
