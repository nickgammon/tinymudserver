#ifndef TINYMUDSERVER_STRINGS_H
#define TINYMUDSERVER_STRINGS_H

#include <ctype.h>   // for toupper, tolower

// strings.h - string functions

static const string SPACES = " \t\r\n";           // what gets removed when we trim

// case-independent (ci) string compare
// returns true if strings are EQUAL
struct ciEqualTo : binary_function <string, string, bool>
  {
  struct compare_equal 
    : public binary_function <unsigned char, unsigned char,bool>
    {
    bool operator() (const unsigned char& c1, const unsigned char& c2) const
      { return tolower (c1) == tolower (c2); }
    };  // end of compare_equal

  bool operator() (const string & s1, const string & s2) const
    {
    pair <string::const_iterator, string::const_iterator> result =
      mismatch (s1.begin (), s1.end (), s2.begin (), compare_equal ()); 

    // match if both at end
    return result.first == s1.end () && result.second == s2.end ();
    }
  }; // end of ciEqualTo

// case-independent (ci) string less_than
// returns true if s1 < s2
struct ciLess : binary_function <string, string, bool>
  {
  // case-independent (ci) compare_less binary function
  struct compare_less 
    : public binary_function <unsigned char, unsigned char,bool>
    {
    bool operator() (const unsigned char& c1, const unsigned char& c2) const
      { return tolower (c1) < tolower (c2); }
    }; // end of compare_less

  bool operator() (const string & s1, const string & s2) const
    {
    return lexicographical_compare
          (s1.begin (), s1.end (), s2.begin (), s2.end (), compare_less ()); 
    }
  }; // end of ciLess

// prototypes
  
// find and replace in a string
string FindAndReplace (const string& source, const string target, const string replacement);
  
// get rid of leading and trailing spaces from a string
string Trim (const string & s, const string & t = SPACES);

// convert to lowercase
string tolower (const string & s);

// capitalise a string
string tocapitals (const string & s);

// case-independent compare equal  
bool ciStringEqual (const string & s1, const string & s2);
  
// split a string into first word, rest-of-line
pair<string, string> GetWord (const string & s);
  
#endif // TINYMUDSERVER_STRINGS_H

