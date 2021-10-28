
#include <iostream>

#include "MySQL.hpp"


static void do_test(void)
{
	MYSQL_ROW row;
	int num_fields;
	mysql::MySQLRes *res;
	mysql::MySQL *db = nullptr;
	const char *host = "127.0.0.1";
	const char *user = "memcpy";
	const char *passwd = "858869123qweASDzxc";
	const char *dbname = NULL;
	uint16_t port = 9999;

	try {
		db = new mysql::MySQL(host, user, passwd, dbname);
		db->setPort(port);
		db->connect();
		db->query("SELECT 1, 2 UNION SELECT 2 UNION SELECT 3;");
		res = db->storeResultRaw();
	} catch (const std::runtime_error& e) {
		std::cout << e.what() << std::endl;
		goto out;
	}

	num_fields = res->numFields();
	while ((row = res->fetchRow())) {
		int i;

		for (i = 0; i < num_fields; i++) {
			printf("%s ", row[i] ? row[i] : "NULL");
		}
		printf("\n");
	}

	delete res;
out:
	delete db;
}


int main(void)
{
	do_test();
}
