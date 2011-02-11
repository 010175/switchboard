#ifndef APPLICATION_LISTER
#define APPLICATION_LISTER

/************************************************************
 
 applicatio_lister class
 
 File: 			applicatio_lister.h
 Description: 	application lister
 
 
 Last Modified: 2009.12.27
 Creator: Guillaume Stagnaro
 
 ************************************************************/

#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <vector>

#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

#define LAST_ADDED_APPLICATION -1

using namespace std;

struct application{
	string name;
	string path;
};


class applicationLister{	
	
public:
	
	applicationLister();
	~applicationLister();
	
	void findExecutable(string path);
	
	void updateApplicationsList();
	
	CFStringRef getApplicationName(int _index);
	
	CFStringRef getApplicationPath(int _index);
	
	CFStringRef getApplicationEnclosingDirectoryPath(int _index);
	
	CFStringRef getApplicationDescription(int _index);
	
	int getApplicationCount();
	
	void dumpApplicationList();
	
private:
	
	string applicationsDirectoryPath;
	
	string getExecutablePath();
	
	vector<application> applicationsVector; // vector containing the applications list
	
	//CFArrayRef applicationBundleArray; // array of applications bundle in the Applications directory
	CFMutableArrayRef applicationBundleArray; // array of applications bundle in the Applications directory
	
	
	
	
};

#endif