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
1. run .sh files in resources
2. inject table in resources/sq/table.sql in mysql in docker
3. After checking db is ready, run build.sh & run.sh
4. When run.sh, you can specify network-interface and port like `./run.sh ens4 9000`

-- You can find the api in resources/api.json
