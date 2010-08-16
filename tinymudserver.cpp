/*

 tinymudserver - an example MUD server

 Author:  Nick Gammon 
          http://www.gammon.com.au/ 

 Date:    22nd July 2004

(C) Copyright Nick Gammon 2004. Permission to copy, use, modify, sell and
distribute this software is granted provided this copyright notice appears
in all copies. This software is provided "as is" without express or implied
warranty, and with no claim as to its suitability for any purpose.
 
*/

// standard library includes ...

#include <iostream>

using namespace std; 

#include "constants.h"
#include "globals.h"

void LoadThings ();
int InitComms ();
void MainLoop ();
void CloseComms ();
   

// called approximately every 0.5 seconds - handle things like fights here
void PeriodicUpdates ()
  {
    //      The example below just sends a message every MESSAGE_INTERVAL seconds.
    // send new command if it is time
  if (time (NULL) > (tLastMessage + MESSAGE_INTERVAL))
    {
    SendToAll ("You hear creepy noises ...\n");
    tLastMessage = time (NULL);
    }
    
  } // end of PeriodicUpdates
  

// main program
int main ()
{
  cout << "Tiny MUD server version " << VERSION << endl;

  LoadThings ();    // load stuff
  
  if (InitComms ()) // listen for new connections
    return 1;

  cout << "Accepting connections from port " <<  PORT << endl;
  
  MainLoop ();    // handle player input/output

  // game over - tell them all
  SendToAll ("\n\n** Game shut down. **\n\n");
  
  CloseComms ();  // stop listening

  cout << "Game shut down." << endl;  
  return 0;
}   // end of main
