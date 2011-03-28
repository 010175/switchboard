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


class applicationLister{	
	
public:
	
	applicationLister();
	~applicationLister();
	
	OSStatus findExecutable(CFURLRef enclosingDirectoryURL);
	
	OSStatus updateApplicationsList();
	
	CFStringRef getApplicationName(int _index);
	
	CFStringRef getApplicationPath(int _index);
	
	int getApplicationIndex(CFStringRef _searchName);

	CFStringRef getApplicationEnclosingDirectoryPath(int _index);
			
	int getApplicationCount();
		
private:
	
	CFURLRef applicationsDirectoryURL;
	
	CFMutableArrayRef applicationBundleArray; // array of applications bundle in the Applications directory
	
};

#endif