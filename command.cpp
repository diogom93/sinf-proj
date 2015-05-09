//
// command.cpp
// Created by Diogo Martins on 08/05/15.

#include "command.h"

string split_command(const string &s) {
	stringstream ss(s);
	string command;
	getline(ss, command, ' ');
	return command;
}

vector<string> split_args(const string &s) {
	vector<string> args;
	stringstream ss(s);
	string arg;
	getline(ss, arg, '|'); // This discards the command because we only want the arguments
	while (getline(ss, arg, '|')) {
		args.push_back(arg);
	}
	return args;
}

void check_command(int socketfd, const string &s) {
	if (s == "\\help") {
		char help_line[256];
		ifstream file_in("help.txt");
		while (!file_in.eof()) {
			file_in.getline(help_line, 255);
			write_to_socket(socketfd, help_line);
		}
	} else {
		string line = "Command not recognized. For help type \\help.";
		write_to_socket(socketfd, line);
	}
	
	return;
}