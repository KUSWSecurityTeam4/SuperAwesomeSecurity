#!/bin/bash

docker run --name mysql --hostname db.security.com -p 3306:3306 -p 33060:33060 -idt mysql
docker run --name postfix --hostname mail.security.com -p 25:25 -idt postfix
