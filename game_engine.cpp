#include "game_engine.h"

ofstream game_file("logEngine.txt", ios::app);

void *game_engine(void *socketfd) {
	int newsockfd = *(int*)socketfd;
	int rounds, timer, gid;
	int qres, qid, rid;
	
	map<int, int> scores;
	
	ostringstream line;
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
	
	line << "SELECT * FROM invites WHERE gid = '" << gid << "' AND state = 'ACCEPTED'";
	res = executeSQL(line.str());
	
	vector<int> play_socks(PQntuples(res));
	
	map<string, int>::iterator it;
	
	for (it = sockets.begin(); it != sockets.end(); it++) {
		game_file << it->first << " " << it->second << endl;
	}
	
	for (int rows = 0; rows < PQntuples(res); rows++) {
		play_socks[rows] = sockets[PQgetvalue(res, rows, 1)];
		game_file << "Player " << play_socks[rows] << sockets[PQgetvalue(res, rows, 1)] << endl;
		scores[play_socks[rows]] = 0;
	}
	
	stringstream db_buff;
	stringstream play_ans;	
	stringstream lock_in;
		
	int i = 0;
	
	while (i < rounds) {
		db_buff.str("");
		res = executeSQL("SELECT * FROM questions ORDER BY random() LIMIT 1");
		db_buff << PQgetvalue(res, 0, 1);
		
		write_to_socket(newsockfd, db_buff.str());
		
		for (int k = 0; k < play_socks.size(); k++) {
			write_to_socket(play_socks[k], db_buff.str()); 
		}
		
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
		
		write_to_socket(newsockfd, play_ans.str());
		
		for (int k = 0; k < play_socks.size(); k++) {
			write_to_socket(play_socks[k], play_ans.str()); 
		}

		play_ans.str("");
		usleep(timer * 1000000);

		i++;
	}
	
	line.str("");
	line << "UPDATE games SET state = 'OVER' WHERE gid = " << gid;
	executeSQL(line.str());
	
	return 0;
}
