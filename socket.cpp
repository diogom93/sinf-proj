//
// socket.cpp
// Created by Diogo Martins on 08/05/15.

#include "socket.h"

bool read_socket(int socketfd, string &line) {
	int n; 
	char buffer[1025]; 
	
	line = "";

	while (line.find('\n') == string::npos) {
		int n = read(socketfd, buffer, 1024);
		if (n == 0) return false;
		buffer[n] = 0; 
		line += buffer;
	}
	
	line.erase(line.end() - 1);
	line.erase(line.end() - 1);
	return true;  
}

bool write_to_socket(int socketfd, string line) {
	string tosend = line + "\n";
	write(socketfd, tosend.c_str(), tosend.length());
	
	return true;
}

void broadcast (int origin, string text) {
	map<string, int>::iterator it;
	for (it = sockets.begin(); it != sockets.end(); it++)
	  if (it->second != origin) {
		write_to_socket(it->second, usernames[origin] + " said: " + text);
	}
}