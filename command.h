//
// command.h
// Created by Diogo Martins on 08/05/15.

#ifndef _command_h_
#define _command_h_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

string split_command(const string &s);
vector<string> split_args(const string &s);

#endif