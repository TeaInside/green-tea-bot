#!/bin/sh

if test -z "$TEST_MYSQL_HOST"; then
  export TEST_MYSQL_HOST="127.0.0.1"
fi

if test -z "$TEST_MYSQL_USER"; then
  export TEST_MYSQL_USER="test"
fi

if test -z "$TEST_MYSQL_PASSWORD"; then
  export TEST_MYSQL_PASSWORD="test123"
fi

if test -z "$TEST_MYSQL_DBNAME"; then
  export TEST_MYSQL_DBNAME="test_db"
fi

if test -z "$TEST_MYSQL_PORT"; then
  export TEST_MYSQL_PORT="3306"
fi

mysql -e "CREATE USER IF NOT EXISTS '${TEST_MYSQL_USER}'@'%' IDENTIFIED BY '${TEST_MYSQL_PASSWORD}';";
mysql -e "CREATE DATABASE IF NOT EXISTS ${TEST_MYSQL_DBNAME};";
mysql -e "GRANT ALL PRIVILEGES ON ${TEST_MYSQL_DBNAME}.* TO '${TEST_MYSQL_USER}'@'%';";
mysql -e "FLUSH PRIVILEGES;";
cd tests && make -j$(nproc) runtests;
