//
// database.cpp
// Created by Diogo Martins on 24/04/15.

#include "database.h"

PGconn* conn = NULL;
 
void initDB()
{
	conn = PQconnectdb("host='vdbm.fe.up.pt' user='sinf15g43' password='umacenafacilqualquer' dbname='sinf15g43'");
 
	if (!conn)
	{
		cout << "Error! Can't connect to DB!" << endl;
		exit(-1);
	}
 
	if (PQstatus(conn) != CONNECTION_OK)
	{
		cout << "Error! Can't connect to DB!" << endl;
		exit(-1);
	}
	
	cout << "Connected to DB successfully!" << endl;
}
 
PGresult* executeSQL(string sql)
{
	PGresult* res = PQexec(conn, sql.c_str());
 
	if (!(PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK))
	{
		cout << "Error executing command!" << endl;
		return NULL;
	}
 
	return res;
}
 
void closeDB()
{
	PQfinish(conn);
	
	cout << "DB connection closed." << endl;
}