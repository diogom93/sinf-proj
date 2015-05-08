//
// socket.h
// Created by Diogo Martins on 08/05/15.

#ifndef _socket_h_
#define _socket_h_

#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

bool read_socket(int socketfd, string &line);
bool write_to_socket(int socketfd, string line);
void* player(void* args);

#endif