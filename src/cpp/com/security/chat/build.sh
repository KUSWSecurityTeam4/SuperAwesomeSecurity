#!/bin/bash
if [ ! -d ./build ]; then
    mkdir ./build
fi

g++ -z execstack -fno-stack-protector -z norelro -g -O0 -std=c++20 main.cpp -o ./build/run.out -lfmt -lssl -lcrypto -lmysqlcppconn8 -lboost_system -lcpprest -pthread
