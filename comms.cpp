/*

 tinymudserver - an example MUD server

 Author:  Nick Gammon 
          http://www.gammon.com.au/ 

(C) Copyright Nick Gammon 2004. Permission to copy, use, modify, sell and
distribute this software is granted provided this copyright notice appears
in all copies. This software is provided "as is" without express or implied
warranty, and with no claim as to its suitability for any purpose.
 
*/

#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

// standard library includes ...

#include <stdexcept>
#include <iostream>
#include <algorithm>

using namespace std; 

#include "utils.h"
#include "constants.h"
#include "player.h"
#include "globals.h"

void PeriodicUpdates ();

// comms descriptors
static fd_set in_set;  
static fd_set out_set;
static fd_set exc_set;

/* Here when a signal is raised */

void bailout (int sig)
{
  cout << "**** Terminated by player on signal " << sig << " ****" << endl << endl;
  bStopNow = true;
} /* end of bailout */

/* set up comms - get ready to listen for connection */

int InitComms ()
  {
  struct sockaddr_in sa;
    
  try
    {
  
    // Create the control socket
    if ( (iControl = socket (AF_INET, SOCK_STREAM, 0)) == -1)
      throw runtime_error ("creating control socket");
    
    // make sure socket doesn't block
    if (fcntl( iControl, F_SETFL, FNDELAY ) == -1)
      throw runtime_error ("fcntl on control socket");
  
    struct linger ld = linger ();  // zero it
  
    // Don't allow closed sockets to linger
    if (setsockopt( iControl, SOL_SOCKET, SO_LINGER,
                    (char *) &ld, sizeof ld ) == -1)
      throw runtime_error ("setsockopt (SO_LINGER)");

    int x = 1;

    // Allow address reuse 
    if (setsockopt( iControl, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &x, sizeof x ) == -1)
      throw runtime_error ("setsockopt (SO_REUSEADDR)");
    
    sa.sin_family       = AF_INET;
    sa.sin_port         = htons (PORT);
    sa.sin_addr.s_addr  = INADDR_ANY;   /* change to listen on a specific adapter */
  
    // bind the socket to our connection port
    if ( bind (iControl, (struct sockaddr *) &sa, sizeof sa) == -1)
      throw runtime_error ("bind");
    
    // listen for connections
  
    if (listen (iControl, SOMAXCONN) == -1)   // SOMAXCONN is the backlog count
      throw runtime_error ("listen");
  
    tLastMessage = time (NULL);
    }  // end of try block
    
  // problem?
  catch (runtime_error & e)
    {
    cerr << "Cannot initialise comms ..." << endl;
    perror (e.what ());
    return 1;    
    }

  // standard termination signals
  signal (SIGINT,  bailout);
  signal (SIGTERM, bailout);
  signal (SIGHUP,  bailout);

  return 0;
  }   /* end of InitComms */


/* close listening port */

void CloseComms ()
  {

  cerr << "Closing all comms connections." << endl;

  // close listening socket
  if (iControl != NO_SOCKET)
    close (iControl);

  // delete all players - this will close connections
  for_each (playerlist.begin (), playerlist.end (), DeleteObject ());

  // delete all rooms
  for_each (roommap.begin (), roommap.end (), DeleteMapObject ());
 
  } /* end of CloseComms */

  // prepare for comms
struct setUpDescriptors
{
  int iMaxdesc;
  
  setUpDescriptors (const int i) : iMaxdesc (i) {}
    
  // check this player
  void operator() (const tPlayer * p) 
    {
     /* don't bother if connection is closed */
      if (p->Connected ())
        {
        iMaxdesc = max (iMaxdesc, p->GetSocket ());
        // don't take input if they are closing down
        if (!p->closing)
          {
          FD_SET( p->GetSocket (), &in_set  );
          FD_SET( p->GetSocket (), &exc_set );
          }

        /* we are only interested in writing to sockets we have something for */
        if (p->PendingOutput ())
          FD_SET( p->GetSocket (), &out_set );
        } /* end of active player */
    } // end of operator()  
    
  int GetMax () const { return iMaxdesc; }
  
};  // end of setUpDescriptors

// handle comms
struct processDescriptors
{
  
  // handle this player
  void operator() (tPlayer * p) 
    {
      /* handle exceptions */
      if (p->Connected () && FD_ISSET (p->GetSocket (), &exc_set))
        p->ProcessException ();

      /* look for ones we can read from, provided they aren't closed */
      if (p->Connected () && FD_ISSET (p->GetSocket (), &in_set))
        p->ProcessRead ();

      /* look for ones we can write to, provided they aren't closed */
      if (p->Connected () && FD_ISSET (p->GetSocket (), &out_set))
        p->ProcessWrite ();
     } // end of operator()  
      
};  // end of processDescriptors

/* new player has connected */

void ProcessNewConnection ()
  {
  static struct sockaddr_in sa;
  socklen_t sa_len = sizeof sa;   

  int s;    /* incoming socket */

  /* loop until all outstanding connections are accepted */
  while (true)
    {
    s = accept ( iControl, (struct sockaddr *) &sa, &sa_len);

    /* a bad socket probably means no more connections are outstanding */
    if (s == NO_SOCKET)
      {

      /* blocking is OK - we have accepted all outstanding connections */
      if ( errno == EWOULDBLOCK )
        return;

      perror ("accept");
      return;
      }
        
    /* here on successful accept - make sure socket doesn't block */
    
    if (fcntl (s, F_SETFL, FNDELAY) == -1)
      {
      perror ("fcntl on player socket");
      return;
      }

    string address = inet_ntoa ( sa.sin_addr);
    int port = ntohs (sa.sin_port);
            
    // immediately close connections from blocked IP addresses
    if (blockedIP.find (address) != blockedIP.end ())
      {
      cerr << "Rejected connection from " << address << endl;
      close (s);
      continue;      
      }
      
    tPlayer * p = new tPlayer (s, port, address);
    playerlist.push_back (p);
    
    cout << "New player accepted on socket " << s << 
            ", from address " << address << 
            ", port " << port << endl;
      
    *p << "\nWelcome to the Tiny MUD Server version " << VERSION << "\n"; 
    *p << messagemap ["welcome"];   // message from message file
    *p << p->prompt;    // initial prompt (Enter your name ...)
    
    } /* end of processing *all* new connections */

  } /* end of ProcessNewConnection */
  
void RemoveInactivePlayers ()
{
  for (tPlayerListIterator i = playerlist.begin (); i != playerlist.end (); )
    {
    if (!(*i)->Connected () ||        // no longer connected
         (*i)->closing)               // or about to leave us
      {
      delete *i;
      playerlist.erase (i++);
      }
    else
      ++i;
    } /* end of looping through players */
} // end of RemoveInactivePlayers

/* process player input - check connection state, and act accordingly */

void ProcessPlayerInput (tPlayer * p, const string & s)
{
   try
    {
    istringstream is (s);
              
    // look up what to do in state map  
    map<tConnectionStates, tHandler>::iterator si = statemap.find (p->connstate);
  
    if (si != statemap.end ())
      si->second (p, is);  // execute command (eg. ProcessCommand) 
    } // end of try block

  // all errors during input processing will be caught here
  catch (runtime_error & e)
    {
    *p << e.what () << "\n";    
    }
  
  *p << p->prompt;   // re-prompt them

} /* end of ProcessPlayerInput */

// main processing loop
void MainLoop ()
{
  // loop processing input, output, events
  do
    {

    // We will go through this loop roughly every COMMS_WAIT_SEC/COMMS_WAIT_USEC
    // seconds (at present 0.5 seconds).
    PeriodicUpdates ();   // do things that don't rely on player input
      
    // delete players who have closed their comms - have to do it outside other loops to avoid 
    // access violations (iterating loops that have had items removed)
    RemoveInactivePlayers ();
      
    // get ready for "select" function ... 
    FD_ZERO (&in_set);
    FD_ZERO (&out_set);
    FD_ZERO (&exc_set);

    // add our control socket (for new connections)
    FD_SET (iControl, &in_set);

    // set bits in in_set, out_set etc. for each connected player
    int iMaxdesc = for_each (playerlist.begin (), playerlist.end (), 
                             setUpDescriptors (iControl)).GetMax ();
    
    // set up timeout interval
    struct timeval timeout;
    timeout.tv_sec = COMMS_WAIT_SEC;    // seconds
    timeout.tv_usec = COMMS_WAIT_USEC;  // + 1000th. of second

    // check for activity, timeout after 'timeout' seconds
    if (select (iMaxdesc + 1, &in_set, &out_set, &exc_set, &timeout) > 0)
      {
      // New connection on control port?
      if (FD_ISSET (iControl, &in_set))
        ProcessNewConnection ();
      
      // handle all player input/output
      for_each (playerlist.begin (), playerlist.end (), 
                processDescriptors ());      
      } // end of something happened
  
    }  while (!bStopNow);   // end of looping processing input

}   // end of MainLoop

