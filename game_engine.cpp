#include <iostream>
#include <clients.h>

using namespace std;

int game_engine(int *socketfd) {
	int timer_val = 0;
	int qres = 0, qid = 0;
	ostringstream iplayers, answer;
	
	PGresult* res = executeSQL("SELECT * FROM games WHERE uid = '" + usernames[socketfd] + "' AND state = 'ONGOING'");
	
	//line << "SELECT * FROM invites WHERE gid = " << PGgetvalue(res,0,0)
	timer_val = std::stoi( PQgetvalue(res,0,4) );
	
	int play_socks[PQtuples(res)];
	for(int rows=0, rows<PQntuples(res), rows++){
	res = executeSQL("SELECT * FROM invites WHERE uid = '" + usernames[socketfd] +"' AND state = 'ACCEPTED'");
	play_socks[rows] = sockets[PGgetvalue(res,rows,1)];
	
	}
	res = executeSQL("SELECT * FROM questions ORDER BY random() LIMIT 1");
	
	//shuffle answers here
	
	answer << PQgetvalue
	qid = PQgetvalue(res,0,0);

	
	

}