//
// command.h
// Created by Diogo Martins on 08/05/15.

#ifndef _command_h_
#define _command_h_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include "socket.h"
#include "clients.h"

using namespace std;

string split_command(const string &s);
vector<string> split_args(const string &s);
bool check_command(int socketfd, const string &s);

#endif