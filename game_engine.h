#ifndef _game_engine_h_
#define _game_engine_h_

#include <iostream>
#include <string>
#include <vector>
#include <ctime>        
#include <cstdlib>      
#include <algorithm>   
#include <fstream>
#include <functional>
#include <sstream>
#include <unistd.h>

#include "clients.h"
#include "socket.h"
#include "database.h"

using namespace std;

void *game_engine(void *socketfd);

#endif


