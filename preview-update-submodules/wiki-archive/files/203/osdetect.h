#ifndef _CONDOR_OSDETECT_H
#define _CONDOR_OSDETECT_H

#include <string>
using namespace std;

class OSDetect {
private:
	string attrName;
	string attrValue;
public:
	OSDetect();
	void detect();
	const string & getAttrName() const;
	const string & getAttrValue() const;
};

#endif // _CONDOR_OSDETECT_H
