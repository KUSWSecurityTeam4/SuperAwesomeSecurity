# Super Awesome Security

## Runtime Environment
You can access environment by `ssh security@34.64.190.215`
All required already installed.

user : security
password: 1123

You can show database information in `run.sh`
db name : chat
db user : security
db password : 1123

You can also the schema in resources/sql/table.sql files

If you want to test codes,
1. run .sh files in resources(build.sh -> run.sh)
2. inject table in resources/sq/table.sql in mysql in docker
3. After checking db is ready, run build.sh & run.sh
4. When run.sh, you can specify network-interface and port like `./run.sh ens4 9000`

-- You can find the api in resources/api.json

## Test API Server open, `http://34.64.190.215:9000`
  - Super user id & password is 'company', '1123'.
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
  
  
