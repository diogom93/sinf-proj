#include "game_engine.h"

ofstream game_file("logEngine.txt", ios::app);

void *game_engine(void *socketfd) {
	int newsockfd = *(int*)socketfd;
	int rounds, timer, gid;
	int qres, qid, rid;
	
	map<int, int> scores;
	
	ostringstream line;
	ostringstream query;
	stringstream temp_buf;
	string correct_ans = "";
	
	PGresult* res = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[newsockfd] + "' AND state = 'ONGOING'");

	temp_buf << PQgetvalue(res, 0, 3);
	timer = atoi( temp_buf.str().c_str());
	temp_buf.str("");
	temp_buf << PQgetvalue(res, 0, 2);
	rounds = atoi( temp_buf.str().c_str());
	temp_buf.str("");
	temp_buf << PQgetvalue(res, 0, 0);
	gid = atoi(temp_buf.str().c_str());
	temp_buf.str("");

	game_file << "Timer: " << timer << " Rounds: " << rounds << endl; 
	
	line << "SELECT * FROM invites WHERE gid = " << gid << " AND state = 'ACCEPTED'";
	res = executeSQL(line.str());
	
	line.str("");
	line << "UPDATE invites SET state = 'DECLINED' WHERE gid = " << gid;
	executeSQL(line.str());
	
	vector<int> play_socks;
	
	play_socks.push_back(newsockfd);
	scores[newsockfd] = 0;
	for (int rows = 0; rows < PQntuples(res); rows++) {
		play_socks.push_back(sockets[PQgetvalue(res, rows, 1)]);
		game_file << "Player " << play_socks[rows] << endl;
		scores[play_socks[rows]] = 0;
	}
	
	stringstream db_buff;
	stringstream play_ans;	
	stringstream lock_in;

	for (int k = 0; k < play_socks.size(); k++) {
		line.str("");
		line << "UPDATE users SET state = " << gid << " WHERE uid = '" << usernames[play_socks[k]] << "'";
		executeSQL(line.str());
	}
		
	int i = 0;
	
	while (i < rounds) {
		vector<int>::iterator it;
		
		db_buff.str("");
		res = executeSQL("SELECT * FROM questions ORDER BY random() LIMIT 1");
		db_buff << PQgetvalue(res, 0, 1);
		
		for (int k = 0; k < play_socks.size(); k++) {
			write_to_socket(play_socks[k], db_buff.str()); 
		}
		
		game_file << db_buff.str() << endl;
		
		vector<string> shuffled_ans;
		db_buff.str("");
		db_buff << PQgetvalue(res, 0, 2);
		correct_ans = db_buff.str();
		shuffled_ans.push_back( db_buff.str());

		db_buff.str("");
		db_buff << PQgetvalue(res, 0, 3);
		shuffled_ans.push_back( db_buff.str());
		db_buff.str("");
		
		db_buff << PQgetvalue(res, 0, 4);
		shuffled_ans.push_back( db_buff.str());
		db_buff.str("");
		
		db_buff << PQgetvalue(res, 0, 5);
		shuffled_ans.push_back( db_buff.str());

		random_shuffle(shuffled_ans.begin(), shuffled_ans.end());

		temp_buf << PQgetvalue(res,0,0);
		qid = atoi( temp_buf.str().c_str());
		temp_buf.str("");

		rid = gid * 100 + (i + 1);
		line.str("");
		line << "INSERT INTO rounds VALUES (" << rid << ", " << gid << ", " << qid << ", '" << shuffled_ans[3] << "', '" << shuffled_ans[2] << "', '" << shuffled_ans[1] << "', '" << shuffled_ans[0] << "')";
		executeSQL(line.str());
		
		play_ans << "A - " << shuffled_ans[3] << "\n" << "B - " << shuffled_ans[2] << "\n" << "C - " << shuffled_ans[1] << "\n" << "D - " << shuffled_ans[0] << "\n";
		
		game_file << play_ans.str() << endl;
		
		for (int k = 0; k < play_socks.size(); k++) {
			write_to_socket(play_socks[k], play_ans.str()); 
		}

		play_ans.str("");
		usleep(timer * 1000000);
		
		for (int k = 0; k < play_socks.size(); k++) {
			line.str("");
			line << "SELECT answer FROM plays WHERE rid = " << rid << " AND uid = '" << usernames[play_socks[k]] << "'";
			PGresult* res = executeSQL(line.str());
			
			if (PQntuples(res) == 0) {
				query.str("");
				query << "INSERT INTO plays VALUES ('" << usernames[play_socks[k]] << "', " << rid << ", NULL, NULL)";
				executeSQL(query.str());
				write_to_socket(play_socks[k], "Wrong!");
			} else {
				string answer = PQgetvalue(res, 0, 0);
				query.str("");
				query << "SELECT r_answer FROM questions WHERE qid = " << qid;
				res = executeSQL(query.str());
				if (PQgetvalue(res, 0, 0) == answer) {
					write_to_socket(play_socks[k], "Correct!");
					scores[play_socks[k]] += 10;
				} else {
					write_to_socket(play_socks[k], "Wrong!");
				}
			}
		}

		i++;
	}
	
	line.str("");
	line << "UPDATE games SET state = 'OVER' WHERE gid = " << gid;
	executeSQL(line.str());
	executeSQL("UPDATE users SET state = NULL WHERE uid = '" + usernames[newsockfd] + "'"); 
	
	for (int k = 0; k < play_socks.size(); k++) {
		write_to_socket(play_socks[k], "Game ended!");
		for (int i = 0; i < play_socks.size(); i++) {
			line.str("");
			line << usernames[play_socks[i]] << " has scored " << scores[play_socks[i]] << " points.";
			write_to_socket(play_socks[k], line.str());
		}
		executeSQL("UPDATE users SET state = NULL WHERE uid = '" + usernames[play_socks[k]] + "'"); 
		line.str("");
		line << "UPDATE users SET rank = rank + " << scores[play_socks[k]] << ", ask_flag = 'FALSE', cut_flag = 'FALSE' WHERE uid = '" << usernames[play_socks[k]] << "'";
		executeSQL(line.str());
	}
		
	return 0;
}
