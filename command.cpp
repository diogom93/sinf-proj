//
// command.cpp
// Created by Diogo Martins on 08/05/15.

#include "command.h"

map<string, int> sockets;
map<int, string> usernames;

string split_command(const string &s) {
	stringstream ss(s);
	string command;
	getline(ss, command, ' ');
	return command;
}

vector<string> split_args(const string &s, char delim) {
	vector<string> args;
	stringstream ss(s);
	string arg;
	getline(ss, arg, ' '); // This discards the command because we only want the arguments
	while (getline(ss, arg, delim)) {
		args.push_back(arg);
	}
	return args;
}

bool check_command(int socketfd, const string &s) {
	vector<string> c_args;
	
	if (split_command(s) == "\\help") {
		char help_line[256];
		ifstream file_in("help.txt");
		while (!file_in.eof()) {
			file_in.getline(help_line, 255);
			write_to_socket(socketfd, help_line);
		}
		
	} else if (split_command(s) == "\\login") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 2) {
			write_to_socket(socketfd, "Please specify username and password. For more information type \\help.");
		} else {
			// Adicionar lógica
			
			sockets[c_args[0]] = socketfd;
			usernames[socketfd] = c_args[0];
		}
		
	} else if (split_command(s) == "\\register") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 2) {
			write_to_socket(socketfd, "Please specify username and password. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\identify") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\question") {
		c_args = split_args(s, '|');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can create questions. For more information type \\help.");
		} else {
			if (c_args.size() < 5) {
				write_to_socket(socketfd, "Oops, something's missing! For more information type \\help.");
			} // Adicionar lógica
		}
		
	} else if (split_command(s) == "\\create") {
		c_args = split_args(s, ' ');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can create games. For more information type \\help.");
		} else {
			if (c_args.size() < 2) {
				write_to_socket(socketfd, "Please specify round time and number of rounds. For more information type \\help.");
			} // Adicionar lógica
		}
		
	} else if (split_command(s) == "\\challenge") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username to challenge. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\accept") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username of challenger. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\decline") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username of challenger. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\start") {
		// Adicionar lógica
		
	} else if (split_command(s) == "\\answer") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify the alternative. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\ask") {
		// Adicionar lógica
		
	} else if (split_command(s) == "\\cut") {
		// Adicionar lógica
		
	} else if (split_command(s) == "\\say") {
		c_args = split_args(s, '|');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can send messages. For more information type \\help.");
		} else {
			if (c_args.size() == 1) {
				broadcast(socketfd, c_args[0]);
			} else if (c_args.size() == 2){
				write_to_socket(sockets[c_args[0]], usernames[socketfd] + " said: " + c_args[1]);
			} else {
				write_to_socket(socketfd, "Please specify message. For more information type \\help.");
			}
		}
		
	} else if (split_command(s) == "\\info") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username. For more information type \\help.");
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\list") {
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can see the questions. For more information type \\help.");
		} else {
			
		} // Adicionar lógica
		
	} else if (split_command(s) == "\\stats") {
		c_args = split_args(s, ' ');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can see question statistics. For more information type \\help.");
		} else {
			if (c_args.size() < 1) {
				write_to_socket(socketfd, "Please specify question number. For more information type \\help.");
			} // Adicionar lógica
		}
		
	} else if (split_command(s) == "\\modify") {
		c_args = split_args(s, '|');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can modify questions. For more information type \\help.");
		} else {
			if (c_args.size() < 6) {
				write_to_socket(socketfd, "Oops, something's missing! For more information type \\help.");
			} // Adicionar lógica
		}
		
	} else if (split_command(s) == "\\exit") { 
		string username = usernames[socketfd] ;
		
		if (username != "") {
			write_to_socket(socketfd, "Goodbye " + username + "!");
		} else {
			write_to_socket(socketfd, "Goodbye!");
		}
		
		sockets.erase(username);
		usernames.erase(socketfd);
		
		return false;
		
	} else {
		write_to_socket(socketfd, "Command not recognized. For help type \\help.");
	}
	
	return true;
}