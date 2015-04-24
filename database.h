//
// database.h
// Created by Diogo Martins on 24/04/15.

#ifndef _database_h_
#define _database_h_

#include <iostream>
#include <string>

#include <libpq-fe.h>

using namespace std;

void initDB();
PGresult* executeSQL(string sql);
void closeDB();

#endif