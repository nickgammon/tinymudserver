#ifndef TINYMUDSERVER_CONSTANTS_H
#define TINYMUDSERVER_CONSTANTS_H

// configuration constants

static const string VERSION = "2.2.0";        // server version
static const int PORT = 4000;                 // incoming connections port
static const string PROMPT = "> ";            // normal player prompt
static const int INITIAL_ROOM = 1000;         // what room they start in
static const int MAX_PASSWORD_ATTEMPTS = 3;   // times they can try a password
static const int MESSAGE_INTERVAL = 60;       // seconds between tick messages
// This is the time the "select" waits before timing out.
static const long COMMS_WAIT_SEC = 0;         // time to wait in seconds
static const long COMMS_WAIT_USEC = 500000;   // time to wait in microseconds
static const int NO_SOCKET = -1;              // indicator for no socket connected
// files
static const string PLAYER_DIR    = "./players/";    // location of player files
static const string PLAYER_EXT    = ".player";       // suffix for player files
static const char * MESSAGES_FILE = "./system/messages.txt";  // messages
static const char * CONTROL_FILE  = "./system/control.txt";   // control file
static const char * ROOMS_FILE    = "./rooms/rooms.txt";      // rooms file
// player names must consist of characters from this list
static const string valid_player_name = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";


#endif // TINYMUDSERVER_CONSTANTS_H

