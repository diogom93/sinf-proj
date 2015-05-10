//
// socket.h
// Created by Diogo Martins on 08/05/15.

#ifndef _socket_h_
#define _socket_h_

#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <set>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "clients.h"
#include "database.h"

using namespace std;

bool read_socket(int socketfd, string &line);
bool write_to_socket(int socketfd, string line);
void* player(void* args);
void broadcast (int origin, string text);

#endif