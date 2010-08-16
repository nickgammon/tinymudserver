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

#include <stdexcept>

using namespace std; 

#include "utils.h"
#include "room.h"
#include "globals.h"

tRoom * FindRoom (const int & vnum)
{
  tRoomMapIterator roomiter = roommap.find (vnum);
  
  if (roomiter == roommap.end ())
    throw runtime_error (MAKE_STRING ("Room number " << vnum << " does not exist."));

  return roomiter->second;
}
