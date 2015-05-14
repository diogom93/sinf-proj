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
			map<int, string>::iterator it = usernames.find(sockets[c_args[0]]);
			
			if (it != usernames.end()) {
				write_to_socket(socketfd, "This user is already logged in.");
			} else {
				PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + c_args[0] + "' AND pass = '" + c_args[1] + "'");
				
				if (PQntuples(res) == 0) {
					write_to_socket(socketfd, "Wrong username and/or password!");
				} else {
					write_to_socket(socketfd, "Login successful!");
					sockets[c_args[0]] = socketfd;
					usernames[socketfd] = c_args[0];
				}
			}
		}
		
	} else if (split_command(s) == "\\register") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 2) {
			write_to_socket(socketfd, "Please specify username and password. For more information type \\help.");
		} else {
			PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + c_args[0] + "'");
			
			if (PQntuples(res) != 0 ) {
				write_to_socket(socketfd, "Sorry, that username is already taken.");
			} else {
				PGresult* res = executeSQL("INSERT INTO users VALUES ('" + c_args[0] + "', '" + c_args[1] + "', false, NULL)");
				
				if (res != NULL) {
					write_to_socket(socketfd, "Register successful! You are now logged in.");
						
					sockets[c_args[0]] = socketfd;
					usernames[socketfd] = c_args[0];
				} else {
					write_to_socket(socketfd, "Sorry, couldn't register you. Try limiting the username to 16 characters and avoiding any weird symbols.");
				}
			}
		}
		
	} else if (split_command(s) == "\\identify") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() == 0) {
			write_to_socket(socketfd, "The following users are online:");
			
			for (map<int, string>::iterator it = usernames.begin(); it != usernames.end(); it++) {
				write_to_socket(socketfd, it->second);
			}
		} else {
			PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + c_args[0] + "'");
			
			if (PQntuples(res) != 0) {
				map<int, string>::iterator it = usernames.find(sockets[c_args[0]]);
				
				if (it != usernames.end()) {
					write_to_socket(socketfd, "The user exists and is online at the moment.");
				} else {
					write_to_socket(socketfd, "The user exists and is offline at the moment.");
				}
			} else {
				write_to_socket(socketfd, "The user doesn't exist. Perhaps you want to \\register that username?");
			}
		}
		
	} else if (split_command(s) == "\\question") {
		c_args = split_args(s, '|');
		
		PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + usernames[socketfd] + "'");
			
		if (PQntuples(res) == 0) {
			write_to_socket(socketfd, "Sorry, only registered users can create questions. For more information type \\help.");
		} else {
			if (c_args.size() < 5) {
				write_to_socket(socketfd, "Oops, something's missing! For more information type \\help.");
			} else {
				PGresult* res = executeSQL("INSERT INTO questions VALUES (DEFAULT, '" + c_args[0] + "', '" + c_args[1] + "', '" + c_args[2] + "', '" + c_args[3] + "', '" + c_args[4] + "', '" + usernames[socketfd] + "')");
				
				if (res != NULL) {
					write_to_socket(socketfd, "Question added successfully!");
				} else {
					write_to_socket(socketfd, "Sorry, couldn't add the question. Maybe there were some weird symbols in the text?");
				}
			} 
		}
		
	} else if (split_command(s) == "\\create") {
		c_args = split_args(s, ' ');
		
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can create games. For more information type \\help.");
		} else {
			if (c_args.size() < 2) {
				write_to_socket(socketfd, "Please specify round time and number of rounds. For more information type \\help.");
			} else {
				int period = stoi(c_args[0]), n_questions = stoi(c_args[1]);
				
				if (period < 10 || period > 30) {
					write_to_socket(socketfd, "Round time must be between 10 and 30 seconds.");
				} else {
					if (n_questions < 1 || n_questions > 15) {
						write_to_socket(socketfd, "Number of rounds must be between 1 and 15.");
					} else {
						PGresult* res = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[socketfd] +"' AND state != 'OVER'");
						
						if (PQntuples(res) == 0) {
							executeSQL("INSERT INTO games VALUES (DEFAULT, 'IDLE', " + c_args[1] + ", " + c_args[0] + ", '" + usernames[socketfd] + "')");
							
							write_to_socket(socketfd, "Game created sucessfully!");
						} else {
							write_to_socket(socketfd, "Sorry, you already have a game pending.");
						}
					}
				}
			}
		}
		
	} else if (split_command(s) == "\\challenge") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username to challenge. For more information type \\help.");
		} else {
			map<string, int>::iterator it = sockets.find(c_args[0]);
			
			if (it == sockets.end()) {
				write_to_socket(socketfd, "You can only challenge online users.");
			} else {
				PGresult* res = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[socketfd] + "' AND state = 'IDLE'");
				
				if (PQntuples(res) == 0) {
					write_to_socket(socketfd, "You have no pending games.");
				} else {
					PGresult* res = executeSQL("SELECT gid FROM games WHERE uid = '" + usernames[socketfd] + "' AND state = 'IDLE'");
					
					ostringstream line;
					line << "INSERT INTO invites VALUES (" << PQgetvalue(res, 0, 0) << ", '" << c_args[0] << "', 'PENDING')";
					
					executeSQL(line.str());
					
					write_to_socket(socketfd, "Invitation sent!");
					write_to_socket(sockets[c_args[0]], "You have received an invitation from " + usernames[socketfd] + "!");
				}
			}
		}
		
	} else if (split_command(s) == "\\accept") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username of challenger. For more information type \\help.");
		} else {
			PGresult* game = executeSQL("SELECT gid FROM games WHERE uid = '" + c_args[0] +"' AND state = 'IDLE'");
			
			if (PQntuples(game) == 0) {
				write_to_socket(socketfd, "That user has no pending games.");
			} else {
				ostringstream line;
				line << "SELECT * FROM invites WHERE gid = " << PQgetvalue(game, 0, 0) << " AND uid = '" << usernames[socketfd] << "' AND state = 'PENDING'";
				PGresult* res = executeSQL(line.str());
				
				if (PQntuples(res) == 0) {
					write_to_socket(socketfd, "You have no challenges from such user.");
				} else {
					line.str("");
					line << "UPDATE invites SET state = 'ACCEPTED' WHERE gid = " << PQgetvalue(game, 0, 0) << " AND uid = '" << usernames[socketfd] << "'";
					
					executeSQL(line.str());
					
					write_to_socket(socketfd, "You accepted " + c_args[0] + " challenge!");
					write_to_socket(sockets[c_args[0]], usernames[socketfd] + " has accepted your challenge!");
				}
			}
		}
		
	} else if (split_command(s) == "\\decline") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username of challenger. For more information type \\help.");
		} else {
			PGresult* game = executeSQL("SELECT gid FROM games WHERE uid = '" + c_args[0] +"' AND state = 'IDLE'");
			
			if (PQntuples(game) == 0) {
				write_to_socket(socketfd, "That user has no pending games.");
			} else {
				ostringstream line;
				line << "SELECT * FROM invites WHERE gid = " << PQgetvalue(game, 0, 0) << " AND uid = '" << usernames[socketfd] << "' AND state = 'PENDING'";
				PGresult* res = executeSQL(line.str());
				
				if (PQntuples(res) == 0) {
					write_to_socket(socketfd, "You have no challenges from such user.");
				} else {
					line.str("");
					line << "UPDATE invites SET state = 'DECLINED' WHERE gid = " << PQgetvalue(game, 0, 0) << " AND uid = '" << usernames[socketfd] << "'";
					
					executeSQL(line.str());
					
					write_to_socket(socketfd, "You declined " + c_args[0] + " challenge!");
					write_to_socket(sockets[c_args[0]], usernames[socketfd] + " has declined your challenge!");
				}
			}
		}		
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
				PGresult* res = executeSQL("INSERT INTO messages VALUES (DEFAULT, '" + usernames[socketfd] + "', NULL, '" + c_args[0] + "', NULL)");
				
				if (res == NULL) {
					write_to_socket(socketfd, "Please avoid any weird symbols.");
				} else {
					broadcast(socketfd, c_args[0]);
					write_to_socket(socketfd, "Message sent.");
				}
				
			} else if (c_args.size() == 2) {
				PGresult* res = executeSQL("INSERT INTO messages VALUES (DEFAULT, '" + usernames[socketfd] + "', '" + c_args[0] + "', '" + c_args[1] + "', NULL)");
				
				if (res == NULL) {
					write_to_socket(socketfd, "Please avoid any weird symbols.");
				} else {
					write_to_socket(sockets[c_args[0]], usernames[socketfd] + " said: " + c_args[1]);
					write_to_socket(socketfd, "Message sent.");
				}
			} else {
				write_to_socket(socketfd, "Please specify message. For more information type \\help.");
			}
		}
		
	} else if (split_command(s) == "\\info") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify username. For more information type \\help.");
		} else {
			PGresult* res = executeSQL("SELECT * FROM users WHERE uid = '" + c_args[0] + "'");
			
			if (PQntuples(res) == 0) {
				write_to_socket(socketfd, "That user does not exist.");
			} else {
				ostringstream line;
				line << PQgetvalue(res, 0, 0) << " has a score of " << PQgetvalue(res, 0, 3) << " points.";
				write_to_socket(socketfd, line.str());
			}
		}
		
	} else if (split_command(s) == "\\ranking") {
		PGresult* res = executeSQL("SELECT * FROM users ORDER BY rank DESC");
		
		for (int row = 0; row < PQntuples(res); row++) {
			ostringstream line;
			line << PQgetvalue(res, row, 0) << " has a score of " << PQgetvalue(res, row, 3) << " points.";
			write_to_socket(socketfd, line.str());
		}
		
	} else if (split_command(s) == "\\list") {
		map<int, string>::iterator it = usernames.find(socketfd);
		
		if (it == usernames.end()) {
			write_to_socket(socketfd, "Sorry, only authenticated users can see the questions. For more information type \\help.");
		} else {
			PGresult* res = executeSQL("SELECT * FROM questions WHERE uid = '" + usernames[socketfd] + "'");
			
			if (PQntuples(res) == 0) {
				write_to_socket(socketfd, "You havent created any questions.");
			} else {
				for (int row = 0; row < PQntuples(res); row++) { 
					ostringstream line;
					line << PQgetvalue(res, row, 0) << " - " << PQgetvalue(res, row, 1) << '|' << PQgetvalue(res, row, 2) << '|' << PQgetvalue(res, row, 3) << '|' << PQgetvalue(res, row, 4) << '|' << PQgetvalue(res, row, 5);
					write_to_socket(socketfd, line.str());
				}
			}
		}
		
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
			} else {
				PGresult* res = executeSQL("UPDATE questions SET question = '" + c_args[1] + "', r_answer = '" + c_args[2] + "', w_answer1 = '" + c_args[3] + "', w_answer2 = '" + c_args[4] + "', w_answer3 = '" + c_args[5] + "' WHERE qid = " + c_args[0]);
				
				if (res != NULL) {
					write_to_socket(socketfd, "Question modified successfully!");
				} else {
					write_to_socket(socketfd, "Sorry, couldn't modify the question. Maybe there were some weird symbols in the text?");
				}
			}
		}
		
	} else if (split_command(s) == "\\delete" ) {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 1) {
			write_to_socket(socketfd, "Oops, something's missing! For more information type \\help.");
		} else {
			if (c_args[0] == "game") {
				PGresult* res = executeSQL("UPDATE games SET state = 'CANCELED' WHERE state = 'IDLE' AND uid = '" + usernames[socketfd] + "'");
				
				if (res == NULL) {
					write_to_socket(socketfd, "Oops, something went wrong! Maybe you need \\help?");
				} else {
					write_to_socket(socketfd, "Game deleted sucessfully!");
				}
			} else if (c_args[0] == "question"){
				if (c_args.size() < 2) {
					write_to_socket(socketfd, "Oops, something's missing! For more information type \\help.");
				} else {
					PGresult* res = executeSQL("DELETE FROM questions WHERE qid = " + c_args[1] + " AND uid = '" + usernames[socketfd] + "'");
					
					if (res == NULL) {
						write_to_socket(socketfd, "Oops, something went wrong! Maybe you need \\help?");
					} else {
						write_to_socket(socketfd, "Question deleted sucessfully!");
					}
				}
			}
		}
	}
	
	else if (split_command(s) == "\\exit") { 
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