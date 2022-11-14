#!/bin/bash

export TZ=/usr/share/zoneinfo/Asia/Seoul

export DB_NAME=chat
export DB_HOST=34.64.190.215
export DB_USER=security
export DB_PASSWORD=1123

export CRT_PATH=controller/ssl/sslca.crt
export KEY_PATH=controller/ssl/sslca.key
export DH_PATH=controller/ssl/dh2048.pem

export LOG_FILE=secure_chat.log

### If executable exists, running it & interconnecting with network-interface($1)
if [ -f ./build/run.out ]; then
    gdb ./build/run.out
fi
