# Super Awesome Security

## Dependency
- Docker
- Mysql & postfix(You can use this by build & run script in /resources/docker/)
- openssl 3(https://github.com/openssl/openssl/blob/master/INSTALL.md)
- spdlog(apt install libspdlog-dev, https://github.com/gabime/spdlog)
- fmt(https://github.com/fmtlib/fmt)
- cpprestsdk(https://github.com/Microsoft/cpprestsdk/wiki/How-to-build-for-Linux)
- g++ 10.3.0(https://launchpad.net/~savoury1/+archive/ubuntu/gcc-10)
- boost(https://www.boost.org/users/history/version_1_80_0.html)
- mysql connector/c++ 8.0(https://dev.mysql.com/doc/connector-cpp/8.0/en/connector-cpp-installation-binary.html)

## Build
- (Notice) You should add current user in `docker` group(Run docker without `sudo`)
- Run `src/cpp/com/security/chat/build.sh`
- Run `src/resource/docker/build.sh & run.sh`
- Check mysql is ready. And then, inject schema in db by `mysql -h [public-ip] -u security -p chat < resources/sql/table.sql`
- Check DB_HOST in `run.sh`(it should be same as your public IP address)

## Run
- Run `src/cpp/com/security/chat/run.sh [non-loopback-ip-interface] [port]` like `run.sh ens4 9000`

## Test
- Check the api in `src/resource/api.json`
- You can test them via some tools like postman


## Runtime Environment
You can access environment by `ssh ctf@34.64.190.215`
All requirements are already installed.

user : ctf
password: security

If you want to run executable, Move `/home/security/KoreaUniversity_SoftwareSecurity_GIT_CTF/Test`.
Run `run.sh` & The log is `secure_chat.log`

You can show database information in `run.sh`
db name : chat
db user : security
db password : 1123

You can also the schema in resources/sql/table.sql files

If you want to test codes,
- When run.sh, you can specify network-interface and port like `./run.sh ens4 9000`

-- You can find the api in resources/api.json

## Test API Server open, `http://34.64.190.215:9000`
  - Initial company name & password is 'company', '1123'.
  - You can login by the api
  ```
  POST /auth/login?type=company
  body : 
    {
      "name" : "company",
      "password" : "1123"
     }
     
  Response :
      {
    "code": 200,
    "data": {
        "array": [
            {
                "company": {
                    "id": 1,
                    "name": "company"
                }
            },
            {
                "session": {
                    "id": 11442710240445645052,
                    "token": "GU12vwRsZ4CNuziSM2GqMWWd2QGmkm4VXNL9HpQVEWxo8T0Hatnj4AGDirIE7VSQtkZUC8gTDe3E6L7eQCpzROV5TWskm6nUwbxO50MEZsn8VOhKOjwdwxSImlQZiZF7jj8OXXmCtp768cGPbG3gDy1DxVFtGs16r6Muqf7kPd4hOML5j1QX02GJ9KkuALNTICydB8kLNUgYAgfJlFjsKj8gXuVeO1sDRzXbIfw4wGr3F80ebWJKUTZXsg1dKMPV"
                }
            }
        ]
    },
    "message": "AuthController[LOGIN](/auth/login?type=company) : ok"
    }
  ```
  Calling all apis except login MUST require session id & token. You can get them after login
  
  
