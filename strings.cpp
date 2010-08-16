/*

 tinymudserver - an example MUD server

 Author:  Nick Gammon 
          http://www.gammon.com.au/ 

(C) Copyright Nick Gammon 2004. Permission to copy, use, modify, sell and
distribute this software is granted provided this copyright notice appears
in all copies. This software is provided "as is" without express or implied
warranty, and with no claim as to its suitability for any purpose.
 
*/

#include <string>
using namespace std; 

#include "strings.h"

// string find-and-replace
string FindAndReplace
  (const string& source, const string target, const string replacement)
  {
  string str = source;
  string::size_type pos = 0,   // where we are now
                    found;     // where the found data is
  if (target.size () > 0)   // searching for nothing will cause a loop
    while ((found = str.find (target, pos)) != string::npos)
      {
      str.replace (found, target.size (), replacement);
      pos = found + replacement.size ();
      }
  return str;
  }   // end of FindAndReplace

// get rid of leading and trailing spaces from a string
string Trim (const string & s, const string & t)
{
  string d = s; 
  string::size_type i = d.find_last_not_of (t);
  if (i == string::npos)
    return "";
  else
   return d.erase (i + 1).erase (0, s.find_first_not_of (t)) ; 
}

// returns a lower case version of the string 
string tolower (const string & s)
  {
string d = s;
  transform (d.begin (), d.end (), d.begin (), (int(*)(int)) tolower);
  return d;
  }  // end of tolower

// transformation function for tocapitals that has a "state"
// so it can capitalise a sequence
class fCapitals : public unary_function<unsigned char,unsigned char> 
  {
  bool bUpper;

  public:

  // first letter in string will be in capitals
  fCapitals () : bUpper (true) {}; // constructor

  unsigned char operator() (const unsigned char & c)  
    { 
    unsigned char c1;
    // capitalise depending on previous letter
    if (bUpper)
      c1 = toupper (c);
    else
      c1 = tolower (c);

    // work out whether next letter should be capitals
    bUpper = isalnum (c) == 0 && c < 0x80;
    return c1; 
    }
  };  // end of class fCapitals

// returns a capitalized version of the string 
string tocapitals (const string & s)
  {
string d = s;
  transform (d.begin (), d.end (), d.begin (), fCapitals ());
  return d;
  }  // end of tocapitals
  
// compare strings for equality using the binary function above
// returns true is s1 == s2
bool ciStringEqual (const string & s1, const string & s2)
  {
  return ciEqualTo () (s1, s2);
  }  // end of ciStringEqual

  /* split a line into the first word, and rest-of-the-line */

pair<string, string> GetWord (const string & s)
{
  string rest = s;  // make copy so we can modify it
  
 // find delimiter  
  string::size_type i (rest.find (' '));

  // split into before and after delimiter
  string w (rest.substr (0, i));

  if (i == string::npos)
    rest.erase ();          // if no delimiter, remainder is empty
  else   
    rest.erase (0, i + 1);  // erase up to the delimiter

  // return first word in line, rest of line
  return make_pair (Trim (w), Trim (rest));
  
} /* end of GetWord */

  
