#ifndef _STRING_HH
#define _STRING_HH

#include <iostream.h>
#include <string.h>

const unsigned NO_SUBSTR_FOUND = 0xfff;
const unsigned DEFAULT_STRING_SIZE = 41;
const unsigned DEFAULT_STRING_SIZE_INCREM = 10;



enum stringErrors {NO_ERR, NULL_STRING, BAD_INDEX};



class cString
{
private:

  char badIndex;
  
protected:

  char* str; //pointer to characters
  unsigned maxSize; //total number of bytes allocated
  unsigned len; //current string size
  unsigned sizeIncr; //increment
  stringErrors strErr;
  
public:

  //constructors

  cString()
    {
      maxSize =  DEFAULT_STRING_SIZE;
      str = new char[maxSize];
      *str = '\0';
      len = 0;
      sizeIncr = DEFAULT_STRING_SIZE_INCREM ;
      strErr = NO_ERR;
      badIndex = ' ';
    }
  
  //creates a string using a c style string
  cString::cString(const char* s, const unsigned sizeIncrement=1)
    {
      badIndex = ' ';
      len = (s)?strlen(s):0;
      
      if(len>0){
	maxSize = len +1;
	str = new char[maxSize];
	strcpy(str,s);
      }
      else{
	maxSize =  DEFAULT_STRING_SIZE;
	str = new char[maxSize];
	str[0] = '\0';
      }
      
      sizeIncr = (sizeIncrement==0)?1:sizeIncrement;
      strErr = NO_ERR;
    }
  
  //create a string using an existing cString 
  cString::cString(const cString& s, const unsigned sizeIncrement)
    {
     badIndex = ' ';
     len = s.len;
     maxSize = len + 1;
     str = new char[maxSize];
     strcpy(str,s.str);
     sizeIncr = (sizeIncrement==0)?1:sizeIncrement;
     strErr = NO_ERR;
    }
  

  //destructor
  cString::~cString()
    {
      if(str){
	delete [] str;
      }
      maxSize = 0;
      len = 0;
      sizeIncr = 0;
    }
  
  cString& cString::resize(unsigned new_size)
    {
      unsigned current_len  = len;
      if(maxSize >= new_size) return *this;
      
      //if new_size is not a perfect multiple of sizeIncr
      if(((new_size/sizeIncr)*sizeIncr) < new_size)
	new_size = ((new_size/sizeIncr) + 1) * sizeIncr;
      
      //create new string
      char* strcopy = new char[new_size +1];
      strcpy(strcopy,str);
      delete [] str;
      maxSize = new_size;
      str = strcopy;
      len=current_len;
      return *this;
    }



  //assignment operators
  cString& cString::operator = (const cString& s)
    {
      len = s.len;
      if((len+1)>maxSize){
	resize(len+1);
      }
      strcpy(str,s.str);
      return *this;
    }
  
  cString& cString::operator = (const char* s)
    {
      len = strlen(s);
      if ((len+1) > maxSize){
	resize(len+1);
      }
      
      strcpy(str,s);
      return *this;
    }

  bool cString::checkBounds(unsigned i)
    {
      return(i<len)?true:false;
    }
  


  //The access operator
  char& cString::operator [](const unsigned i)
    {
      if(checkBounds(i))
	{
	  return *(str+i);
	}
      else{
	strErr = BAD_INDEX;
	return badIndex;
      }
    }
  

  //The relational operators
  int cString::operator ==(const cString& s)
    {
      return strcmp(str,s.str) == 0;
    }
  
  int cString::operator !=(const cString& s)
    {
      return strcmp(str,s.str) != 0;
    }
  
  int cString::operator ==(const char* s)
    {
      return strcmp(str,s) == 0;
    }
  


  //others
  char* cString::getString()
    {
      return str;
    }
  
  
  bool cString::contains(char* s)
    {
      return(strstr(str,s));
    }
  
  cString& cString::append(const char c, unsigned count=1)
    {
      if(0==count) return *this;
      
      char* s = new char[count+1];
      if((count+len+1)>maxSize)
	resize(count+len+1);
      
      memset(s,c,count);
      s[count] = '\0';
      
      strcat(str,s);
      len+=count;
      delete [] s;
      
      return *this;
    }
  


  //split the string to wrods separated by space
  void cString::split(cString* words, unsigned n)
    {
      unsigned int i = 0, j=0;
      
      //skip the initial spaces
      while(j<len && str[j]==' ') j++;

      while(j<len && i<n){
	
	while(str[j]!=' ' && j<len){
	  words[i].append(str[j]);
	  j++;
	}
      
	//move to the next nonspace character
	while(j<len && str[j]==' ') j++;
	i++;
      }
      
      return;
    }

  friend ostream& operator <<(ostream& os, cString& s)
    {
      os<<s.str;
      return os;
    }  

  int getLen()
    {
      return len;
    }
  
  
  
};




#endif
