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
	string key = "key";
	
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
			map<int, string>::iterator ti = usernames.find(socketfd);
			string pass = XOR(c_args[1], key);
			
			if (it != usernames.end()) {
				write_to_socket(socketfd, "This user is already logged in.");
			} else if (ti != usernames.end()) {
				write_to_socket(socketfd, "You are already logged in!");
			} else {
				PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + c_args[0] + "' AND pass = '" + pass + "'");
				
				if (PQntuples(res) == 0) {
					write_to_socket(socketfd, "Wrong username and/or password!");
				} else {
					executeSQL("UPDATE users SET state = NULL WHERE uid = '" + c_args[0] + "'");
					write_to_socket(socketfd, "Login successful!");
					sockets[c_args[0]] = socketfd;
					usernames[socketfd] = c_args[0];
					
					PGresult* res = executeSQL("SELECT * FROM messages WHERE state = 'PENDING' AND rcv = '" + c_args[0] + "'");
					
					for (int row = 0; row < PQntuples(res); row++) {
						ostringstream line;
						line << PQgetvalue(res, row, 1) << " said: " << PQgetvalue(res, row, 3);
						write_to_socket(socketfd, line.str());
						line << "";
					}
					
					executeSQL("UPDATE messages SET state = 'SENT' WHERE state = 'PENDING' AND rcv = '" + c_args[0] + "'");
				}
			}
		}
		
	} else if (split_command(s) == "\\register") {
		c_args = split_args(s, ' ');
		
		if (c_args.size() < 2) {
			write_to_socket(socketfd, "Please specify username and password. For more information type \\help.");
		} else {
			PGresult* res = executeSQL("SELECT uid FROM users WHERE uid = '" + c_args[0] + "'");
			string pass = XOR(c_args[1], key);
			
			if (PQntuples(res) != 0 ) {
				write_to_socket(socketfd, "Sorry, that username is already taken.");
			} else {
				PGresult* res = executeSQL("INSERT INTO users VALUES ('" + c_args[0] + "', '" + pass + "', false, 0)");
				
				if (res != NULL) {
					map<int, string>::iterator ti = usernames.find(socketfd);
					if (ti != usernames.end()) {
						write_to_socket(socketfd, "Register successful!");
					} else {
						write_to_socket(socketfd, "Register successful! You are now logged in.");
							
						sockets[c_args[0]] = socketfd;
						usernames[socketfd] = c_args[0];
					}
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
				int period = atoi(c_args[0].c_str()), n_questions = atoi(c_args[1].c_str());
				
				if (period < 10 || period > 30) {
					write_to_socket(socketfd, "Round time must be between 10 and 30 seconds.");
				} else {
					if (n_questions < 1 || n_questions > 15) {
						write_to_socket(socketfd, "Number of rounds must be between 1 and 15.");
					} else { 
						PGresult* res = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[socketfd] + "' AND state = 'PENDING'");
						
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
		
		PGresult* gid = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[socketfd] + "' AND state = 'IDLE'");
		PGresult* st = executeSQL("SELECT state FROM users WHERE uid = '" + usernames[socketfd] + "'");
		string state = PQgetvalue(st, 0, 0);
		
		if (PQntuples(gid) == 0) {
			write_to_socket(socketfd, "You have no pending games.");
		} else if (state == "t") {
			write_to_socket(socketfd, "You can't start while in a game!");
		} else {
			ostringstream line;
			line << "SELECT * FROM invites WHERE gid = " << PQgetvalue(gid, 0, 0) << " AND state = 'ACCEPTED'";
				
			PGresult* res = executeSQL(line.str());
			if (PQntuples(res) == 0) {
				write_to_socket(socketfd, "No players have joined the game.");
			} else {
				ostringstream line;
				line << "UPDATE games SET state = 'ONGOING' WHERE gid = " << PQgetvalue(gid, 0, 0);
					
				res = executeSQL(line.str());
			 
				pthread_t thread;
				int newsockfd = socketfd;
		 
				if (pthread_create(&thread, NULL, game_engine, &newsockfd)) {
					write_to_socket(socketfd, "Could not start game.");
				} else {
					write_to_socket(socketfd, "Game starting...");
				}
			}
		}
			
	} else if (split_command(s) == "\\answer") {
		c_args = split_args(s, ' ');
		string gid = "";
		string play = "";
		string rid = "";
		ostringstream line;
		ostringstream query;
		
		PGresult* resuser = executeSQL("SELECT state FROM users WHERE uid = '" + usernames[socketfd] + "'");
		
		if (PQgetisnull(resuser, 0, 0)) {
			write_to_socket(socketfd, "You are not presently in game! For more information type \\help.");
		} else if (c_args.size() < 1) {
			write_to_socket(socketfd, "Please specify the alternative. For more information type \\help.");
		} else {
			PGresult* resgame = executeSQL("SELECT * FROM users WHERE uid = '" + usernames[socketfd] + "'");
			gid = PQgetvalue(resgame, 0, 3);
			
			PGresult* resround = executeSQL("SELECT * FROM rounds WHERE gid = " + gid + " ORDER BY rid DESC LIMIT 1");
			rid = PQgetvalue(resround, 0, 0);
			
			PGresult* res = executeSQL("SELECT * FROM plays WHERE uid = '" + usernames[socketfd] + "' and rid = " + rid);
			
			if (PQntuples(res) != 0) {
				write_to_socket(socketfd, "You have already answered!");
			} else {
				if ((c_args[0] == "a") || (c_args[0] == "A")){
					play = PQgetvalue(resround, 0, 3);
					query << "INSERT INTO plays VALUES ('" << usernames[socketfd] << "', " << rid << ", '" << play << "', DEFAULT, DEFAULT)";
					executeSQL(query.str());
					
				} else if( (c_args[0] == "b") || (c_args[0] == "B") ){
					play = PQgetvalue(resround, 0, 4);
					query << "INSERT INTO plays VALUES ('" << usernames[socketfd] << "', " << rid << ", '" << play << "', DEFAULT, DEFAULT)";
					line << executeSQL(query.str());
					
				} else if( (c_args[0] == "c") || (c_args[0] == "C") ){
					play = PQgetvalue(resround, 0, 5);
					query << "INSERT INTO plays VALUES ('" << usernames[socketfd] << "', " << rid << ", '" << play << "', DEFAULT, DEFAULT)";
					line << executeSQL(query.str());
					
				} else if( (c_args[0] == "d") || (c_args[0] == "D") ){
					play = PQgetvalue(resround, 0, 6);
					query << "INSERT INTO plays VALUES ('" << usernames[socketfd] << "', " << rid << ", '" << play << "', DEFAULT, DEFAULT)";
					line << executeSQL(query.str());
					
				} else {
					write_to_socket(socketfd, "Invalid answer!");
				}
			}
		}
		
	} else if (split_command(s) == "\\ask") {
		stringstream line;
		vector<string> ans_list;
		int ans_count[4] = {0};
		string gid, rid, qid;
		
		PGresult* resgame = executeSQL("SELECT state FROM users WHERE uid = '" + usernames[socketfd] + "'");
		
		if (PQgetisnull(resgame, 0, 0)) {
			write_to_socket(socketfd, "You are not presently in game! For more information type \\help.");
		} else {
			gid = PQgetvalue(resgame, 0, 0);
			
			PGresult* resround = executeSQL("SELECT * FROM rounds WHERE gid = " + gid + " ORDER BY rid DESC LIMIT 1");
			qid = PQgetvalue(resround, 0, 2);
			rid = PQgetvalue(resround, 0, 0);
			
			PGresult* res = executeSQL("SELECT * FROM plays WHERE uid = '" + usernames[socketfd] + "' and rid = " + rid);
			
			if (PQntuples(res) != 0) {
				write_to_socket(socketfd, "You have already answered!");
			} else if (PQgetvalue(res, 0, 3) == "1") {
				write_to_socket(socketfd, "You have already used \\ask! Cheater!");
			} else {
				line << PQgetvalue(resround, 0, 3);
				ans_list.push_back(line.str());
				line.str("");
				
				line << PQgetvalue(resround, 0, 4);
				ans_list.push_back(line.str());
				line.str("");
				
				line << PQgetvalue(resround, 0, 5);
				ans_list.push_back(line.str());
				line.str("");
				
				line << PQgetvalue(resround, 0, 6);
				ans_list.push_back(line.str());
				line.str("");

				line << "SELECT answer FROM plays WHERE rid IN (SELECT rid FROM rounds WHERE qid = " + qid + ")";
				PGresult* ans_hist = executeSQL(line.str());
				line.str("");
				
				for(int i = 0; i < PQntuples(ans_hist); i++) {
					line << PQgetvalue(resround, i, 0);
					int j = 0;
					for (vector<string>::iterator it = ans_list.begin(); it != ans_list.end(); ++it) {
						if(*it == line.str()) 
							ans_count[j]++;
						j++; 
					}
					line.str() = "";
				}

				line << "A blast from the past! The results of the previous games were recorded as:" << "\nA - " << ans_count[0] << "\nB - " << ans_count[1] << "\nC - " << ans_count[2] << "\nD - " << ans_count[3] << endl;
				write_to_socket(socketfd, line.str());
				executeSQL("UPDATE plays SET askflag = 'TRUE' WHERE uid = '" + usernames[socketfd] + "' AND rid = " + rid);
			}
		}
		
	} else if (split_command(s) == "\\cut") {
		string gid, qid, rid, correct_ans, db_cut = "";
		stringstream line;
		vector<string> ans_list;
		int cut_ans[2] = {0};
		int rng_ans = 0;
		
		srand (time(NULL));

		PGresult* resgame = executeSQL("SELECT * FROM invites WHERE uid = '" + usernames[socketfd] + "' AND state = 'PLAYING'");
		line << PQgetvalue(resgame, 0, 0);
		gid = atoi(line.str().c_str());
		line.str() = "";

		PGresult* resround = executeSQL("SELECT * FROM rounds WHERE gid = '" + gid + "' ORDER BY rid DESC LIMIT 1");		
		line << PQgetvalue(resround, 0, 2);
		qid = line.str();
		line.str() = "";
		line << PQgetvalue(resround, 0, 0);
		rid = line.str();
		line.str() = "";

		line << PQgetvalue(resround, 0, 3);
		ans_list.push_back(line.str());
		line.str() = "";
		line << PQgetvalue(resround, 0, 4);
		ans_list.push_back(line.str());
		line.str() = "";
		line << PQgetvalue(resround, 0, 5);
		ans_list.push_back(line.str());
		line.str() = "";
		line << PQgetvalue(resround, 0, 6);
		ans_list.push_back(line.str());
		line.str() = "";

		PGresult* resans = executeSQL("SELECT * FROM questions WHERE qid = '" + qid + "'");
		line << PQgetvalue(resans, 0, 2);
		correct_ans = line.str();
		line.str() = "";

		write_to_socket(socketfd, "User requested chop-chop, half the answers will drop!\n");

		int i = 0;
		while( i < 2){
			rng_ans = rand() % 4 + 1;
			if(ans_list[rng_ans] == correct_ans) continue;
			else if(cut_ans[0] != rng_ans) cut_ans[i] = rng_ans;
			i++;
		}

		sort(cut_ans, cut_ans+2);
		db_cut += (char)(cut_ans[0]+64);
		db_cut += (char)(cut_ans[1]+64);
		PGresult* resupdate = executeSQL("UPDATE plays SET cutans = '" + db_cut + "' WHERE uid = " + usernames[socketfd] + "AND rid = '" + rid + "'");
		line << "And the new answer selection makes a striking appearance!" << "\n" << db_cut.at(0) << " - " << ans_list[correct_ans[0]] << db_cut.at(1) << " - " << ans_list[correct_ans[1]] << endl;
		write_to_socket(socketfd, line.str());
		
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
				
				map<string, int>::iterator it = sockets.find(c_args[0]);
				
				if (it != sockets.end()) {
					write_to_socket(sockets[c_args[0]], usernames[socketfd] + " said: " + c_args[1]);
					PGresult* res = executeSQL("INSERT INTO messages VALUES (DEFAULT, '" + usernames[socketfd] + "', '" + c_args[0] + "', '" + c_args[1] + "', 'SENT')");
					if (res == NULL) {
						write_to_socket(socketfd, "Please avoid any weird symbols.");
					} else {
						write_to_socket(socketfd, "Message sent!");
					}
				} else {
					PGresult* res = executeSQL("INSERT INTO messages VALUES (DEFAULT, '" + usernames[socketfd] + "', '" + c_args[0] + "', '" + c_args[1] + "', 'PENDING')");
					if (res == NULL) {
						write_to_socket(socketfd, "Please avoid any weird symbols.");
					} else {
						write_to_socket(socketfd, "Message sent!");
					}
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
				line << PQgetvalue(res, 0, 0) << " has a score of " << PQgetvalue(res, 0, 2) << " points.";
				write_to_socket(socketfd, line.str());
			}
		}
		
	} else if (split_command(s) == "\\ranking") {
		PGresult* res = executeSQL("SELECT * FROM users ORDER BY rank DESC");
		
		for (int row = 0; row < PQntuples(res); row++) {
			ostringstream line;
			line << row + 1 << " - " << PQgetvalue(res, row, 0) << " has a score of " << PQgetvalue(res, row, 2) << " points.";
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
			} // Adicionar lÃ³gica
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
		
	} else if (split_command(s) == "\\exit") { 
		string username = usernames[socketfd] ;
		
		if (username != "") {
			write_to_socket(socketfd, "May the force be with you " + username + "!");
		} else {
			write_to_socket(socketfd, "May the force be with you!");
		}
		
		sockets.erase(username);
		usernames.erase(socketfd);
		
		return false;
		
	} else {
		write_to_socket(socketfd, "Command not recognized. For help type \\help.");
	}
	
	return true;
}
