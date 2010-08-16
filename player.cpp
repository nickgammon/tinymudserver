/*

 tinymudserver - an example MUD server

 Author:  Nick Gammon 
          http://www.gammon.com.au/ 

(C) Copyright Nick Gammon 2004. Permission to copy, use, modify, sell and
distribute this software is granted provided this copyright notice appears
in all copies. This software is provided "as is" without express or implied
warranty, and with no claim as to its suitability for any purpose.
 
*/

// standard library includes ...

#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iterator>
#include <errno.h>

using namespace std; 

#include "utils.h"
#include "constants.h"
#include "strings.h"
#include "player.h"
#include "room.h"
#include "globals.h"

/* find a player by name */

tPlayer * FindPlayer (const string & name)
{
  tPlayerListIterator i =
    find_if (playerlist.begin (), playerlist.end (), findPlayerName (name));

  if (i == playerlist.end ())
    return NULL;
  else
    return *i;
  
} /* end of FindPlayer */

// member function to find another playing, including myself
tPlayer * tPlayer::GetPlayer (istream & args, const string & noNameMessage, const bool & notme)
{
  string name;
  args >> name;  
  if (name.empty ())
    throw runtime_error (noNameMessage);
  tPlayer * p = this;
  if (ciStringEqual (name, "me") || ciStringEqual (name, "self"))
    p = this;
  else
    p = FindPlayer (name);
  if (p == NULL)
    throw runtime_error (MAKE_STRING ("Player " << tocapitals (name) << " is not connected."));
  if (notme && p == this)
    throw runtime_error ("You cannot do that to yourself.");
  return p;  
} // end of GetPlayer

void tPlayer::ProcessException ()
{
  /* signals can cause exceptions, don't get too excited. :) */
  cerr << "Exception on socket " << s << endl;
} /* end of tPlayer::ProcessException */

void tPlayer::Load ()
{
  ifstream f ((PLAYER_DIR + playername + PLAYER_EXT).c_str (), ios::in);
  if (!f)
    throw runtime_error ("That player does not exist, type 'new' to create a new one.");
  
  // read player details
  f >> password;
  f >> room;
  f.ignore (numeric_limits<int>::max(), '\n'); // skip rest of this line  
  LoadSet (f, flags);   // player flags (eg. can_shutdown) 
  
} /* end of tPlayer::Load */

void tPlayer::Save ()
{
  ofstream f ((PLAYER_DIR + playername + PLAYER_EXT).c_str (), ios::out);
  if (!f)
  {
  cerr << "Could not write to file for player " << playername << endl;
  return;
  }
  
  // write player details
  f << password << endl;
  f << room << endl;
  copy (flags.begin (), flags.end (), ostream_iterator<string> (f, " "));
  f << endl;
  
} /* end of tPlayer::Save */

void tPlayer::DoCommand (const string & command)
{
  istringstream is (command);
  ProcessCommand (this, is);
} /* end of tPlayer::Load */

// is flag set?
bool tPlayer::HaveFlag (const string & name)
{
  return flags.find (name) != flags.end ();
} // end of NeedFlag

// flag must be set
void tPlayer::NeedFlag (const string & name)
{
  if (!HaveFlag (name))
    throw runtime_error ("You are not permitted to do that.");
} // end of NeedFlag

// flag must not be set
void tPlayer::NeedNoFlag (const string & name)
{
  if (HaveFlag (name))
    throw runtime_error ("You are not permitted to do that.");
} // end of NeedNoFlag

/* Here when there is outstanding data to be read for this player */

void tPlayer::ProcessRead ()
{
  
  if (closing)
    return;   // once closed, don't handle any pending input
  
  // I make it static to save allocating a buffer each time.
  // Hopefully this function won't be called recursively.
  static vector<char> buf (1000);  // reserve 1000 bytes for reading into
  
  int nRead = read (s, &buf [0], buf.size ());
  
  if (nRead == -1)
    {
    if (errno != EWOULDBLOCK)
      perror ("read from player");
    return;
    }

  if (nRead <= 0)
    {
    close (s);
    cerr << "Connection " << s << " closed" << endl;
    s = NO_SOCKET;
    DoCommand ("quit");  // tell others the s/he has left
    return;
    }

  inbuf += string (&buf [0], nRead);    /* add to input buffer */

  /* try to extract lines from the input buffer */
  for ( ; ; )
    {
    string::size_type i = inbuf.find ('\n');
    if (i == string::npos)
      break;  /* no more at present */

    string sLine = inbuf.substr (0, i);  /* extract first line */
    inbuf = inbuf.substr (i + 1, string::npos); /* get rest of string */

    ProcessPlayerInput (this, Trim (sLine));  /* now, do something with it */        
    }
    
} /* end of tPlayer::ProcessRead */

/* Here when we can send stuff to the player. We are allowing for large
 volumes of output that might not be sent all at once, so whatever cannot
 go this time gets put into the list of outstanding strings for this player. */

void tPlayer::ProcessWrite ()
{
  /* we will loop attempting to write all in buffer, until write blocks */
  while (s != NO_SOCKET && !outbuf.empty ())
    {

    // send a maximum of 512 at a time
    int iLength = min<int> (outbuf.size (), 512);

    // send to player
    int nWrite = write (s, outbuf.c_str (), iLength );

    // check for bad write
    if (nWrite < 0)
      {
      if (errno != EWOULDBLOCK )
        perror ("send to player");  /* some other error? */
      return;
      }

    // remove what we successfully sent from the buffer
    outbuf.erase (0, nWrite);
      
    // if partial write, exit
    if (nWrite < iLength)
       break;

    } /* end of having write loop */

}   /* end of tPlayer::ProcessWrite  */

// functor for sending messages to all players
struct sendToPlayer
{
  const string message;
  const tPlayer * except;
  const int room;
  
  // ctor
  sendToPlayer (const string & m, const tPlayer * e = NULL, const int r = 0) 
      : message (m), except (e), room (r) {}
  // send to this player
  void operator() (tPlayer * p) 
    {
    if (p->IsPlaying () && p != except && (room == 0 || p->room == room))
      *p << message;
    } // end of operator()  
};  // end of sendToPlayer

// send message to all connected players
// possibly excepting one (eg. the player who said something)
// possibly only in one room (eg. for saying in a room)
void SendToAll (const string & message, const tPlayer * ExceptThis, const int InRoom)
{
  for_each (playerlist.begin (), playerlist.end (), 
            sendToPlayer (message, ExceptThis, InRoom)); 
} /* end of SendToAll */

