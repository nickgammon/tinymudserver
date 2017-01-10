#ifndef TINYMUDSERVER_PLAYER_H
#define TINYMUDSERVER_PLAYER_H

#include <set>
#include <list>

#include <unistd.h>   // for close
#include "strings.h"  // for ciLess
#include "constants.h"  // for NO_SOCKET

// connection states - add more to have more complex connection dialogs 
typedef enum
{
  eAwaitingName,        // we want their player name
  eAwaitingPassword,    // we want their old password
  
  eAwaitingNewName,     // they have typed 'new' and are being asked for a new name
  eAwaitingNewPassword, // we want a new password
  eConfirmPassword,     // confirm the new password
  
  ePlaying              // this is the normal 'connected' mode
} tConnectionStates;

/*---------------------------------------------- */
/*  player class - holds details about each connected player */
/*---------------------------------------------- */

class tPlayer
{
private:
  int s;              // socket they connected on
  int port;           // port they connected on
 
  string outbuf;      // pending output
  string inbuf;       // pending input
  string address;     // address player is from

public:
  tConnectionStates connstate;      /* connection state */
  string prompt;      // the current prompt
  string playername;  // player name
  string password;    // their password
  int badPasswordCount;   // password guessing attempts
  int room;         // what room they are in
  bool closing;     // true if they are about to leave us
  std::set<string, ciLess> flags;  // player flags

  tPlayer (const int sock, const int p, const string a) 
    : s (sock), port (p), address (a), closing (false)  
      { Init (); } // ctor
  
  ~tPlayer () // dtor
    {
    ProcessWrite ();    // send outstanding text
    if (s != NO_SOCKET) /* close connection if active */
      close (s);
    if (connstate == ePlaying)
      Save ();          // auto-save on close
    };
  
  void Init ()
    {
    connstate = eAwaitingName;
    room = INITIAL_ROOM;
    flags.clear ();
    prompt = "Enter your name, or 'new' to create a new character ...  "; 
    }
    
  // what's our socket?
  int GetSocket () const { return s; }
  // true if connected at all
  bool Connected () const { return s != NO_SOCKET; }
  // true if this player actively playing
  bool IsPlaying () const { return Connected () && connstate == ePlaying && !closing; }
  // true if we have something to send them
  bool PendingOutput () const { return !outbuf.empty (); }

  // output to player (any type)
  template<typename T>
  tPlayer & operator<< (const T & i)  
    {     
    outbuf += MAKE_STRING (i); 
    return *this; 
    }
  
  void ClosePlayer () { closing = true; }    // close this player's connection
  
  void ProcessRead ();    // get player input
  void ProcessWrite ();   // output outstanding text
  void ProcessException (); // exception on socket
  void Load ();           // load player from disk
  void Save ();           // save player to disk

  tPlayer * GetPlayer (istream & sArgs, 
                      const string & noNameMessage = "Do that to who?", 
                      const bool & notme = false);
  
  bool HaveFlag   (const string & name);  // is flag set?
  void NeedFlag   (const string & name);  // flag must be set
  void NeedNoFlag (const string & name);  // flag must not be set
  
  void DoCommand (const string & command);  // simulate player input (eg. look)
  string GetAddress () const { return address; }  // return player IP address
  
};
  
// player list type
typedef std::list <tPlayer*> tPlayerList;
typedef tPlayerList::iterator tPlayerListIterator;


template <typename T>
class player_output_iterator : public std::iterator <std::output_iterator_tag, void, void, void, void>
{
 protected:
   tPlayer &    player_;  // who we are outputting to
   const char * delim_;   // delimiter between each item
   
 public:
  // constructor
  player_output_iterator (tPlayer & p, const char* d = "")
    : player_ (p), delim_ (d) {}
  
  // copy constructor
  player_output_iterator (const player_output_iterator<T>& rhs)
   : player_ (rhs.player_), delim_ (rhs.delim_) {}
  
  // assignment
  player_output_iterator<T>& operator= (const T& rhs)
    { 
    player_ << rhs << delim_;  
    return *this;
    }
  
  // dereference - no operation, returns reference to itself
  player_output_iterator<T>& operator* ()     { return *this; }
  // increment   - no operation, returns reference to itself
  player_output_iterator<T>& operator++ ()    { return *this; }
  // increment   -  no operation, returns reference to itself
  player_output_iterator<T>& operator++ (int) { return *this; }
};  // end of player_output_iterator


// an action handler (commands, connection states)
typedef void (*tHandler) (tPlayer * p, istream & args) ;

// functor to help finding a player by name
struct findPlayerName
{
  const string name;
  // ctor
  findPlayerName (const string & n) : name ( n ) {} 
  // check for player with correct name, and actually playing
  bool operator() (const tPlayer * p) const
    {
    return p->IsPlaying () && ciStringEqual (p->playername, name);
    } // end of operator()  
};  // end of findPlayerName

// find a player by name
tPlayer * FindPlayer (const string & name);
void ProcessCommand (tPlayer * p, istream & sArgs);
void ProcessPlayerInput (tPlayer * p, const string & s);
void SendToAll (const string & message, const tPlayer * ExceptThis = NULL, const int InRoom = 0);

#endif // TINYMUDSERVER_PLAYER_H
