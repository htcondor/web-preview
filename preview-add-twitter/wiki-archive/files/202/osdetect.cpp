#include "osdetect.h"

OSDetect::OSDetect()
{
	attrName = "ActualPlatform";
	attrValue = "Unknown";
}

void OSDetect::detect()
{
	string os;
	string distro;
	string arch;
	char buffer[100];
	FILE * file = popen("uname -s","r");
	if (!file)
	{
		attrValue = "Unknown";
		return;
	}
	else
	{
		fgets(buffer,sizeof(buffer),file);
		if (buffer[strlen(buffer) - 1] == '\n')
		{
			buffer[strlen(buffer) - 1] = '\0';
		}
		os = buffer;
		pclose(file);
	}
	if (fopen("/etc/SuSE-release","r"))
	{
		distro = "SUSE";
	}
	else if (fopen("/etc/debian_version","r"))
	{
		distro = "Debian";
	}
	else if (fopen("/etc/lsb-release","r"))
	{
		distro = "Ubuntu";
	}
	else if (fopen("/etc/redhat-release","r"))
	{
		distro = "Red Hat";
	}
	else
	{
		attrValue = "Unknown";
		return;
	}
	file = popen("uname -m","r");
	if (file)
	{
		fgets(buffer,sizeof(buffer),file);
		if (buffer[strlen(buffer) - 1] == '\n')
		{
			buffer[strlen(buffer) - 1] = '\0';
		}
		arch = buffer;
		pclose(file);
	}
	else
	{
		attrValue = "Unknown";
	}
	attrValue = os + "," + distro + "," + arch;
}

const string & OSDetect::getAttrName() const
{
	return attrName;
}

const string & OSDetect::getAttrValue() const
{
	return attrValue;
}
