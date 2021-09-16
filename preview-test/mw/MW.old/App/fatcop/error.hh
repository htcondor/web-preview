#ifndef ERROR_HH
#define ERROR_HH

#include <iostream.h>

void Error( bool condition, const char *errMsg , bool quit)
{
  if(condition){
    cerr << "Error message: " << errMsg << endl;
    if(quit){
      exit(1);
    }
  }
  
  return;
  
}

#endif
