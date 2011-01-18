/***********************************************************************
 ** __MyCompanyName__
 ** guillaume
 ** Copyright (c) 2009. All rights reserved.
 **********************************************************************/

#include "application_lister.h"

//----------------------------------------------------------
applicationLister::applicationLister(){
	applicationBundleArray = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
	updateApplicationsList(); // build the application list
}

//----------------------------------------------------------
applicationLister::~applicationLister(){

	CFRelease(applicationBundleArray);
}

//----------------------------------------------------------
CFStringRef applicationLister::getApplicationName(int _index){
	
	if (_index==LAST_ADDED_APPLICATION){
	
		_index = 0;
	}
	
	CFBundleRef aBundle = (CFBundleRef) CFArrayGetValueAtIndex(applicationBundleArray, _index);
	
	//get bundle name
	CFTypeRef aBundleName = CFBundleGetValueForInfoDictionaryKey(aBundle, kCFBundleNameKey);
	
	// if no bundleNameKey exist get the executable name.
	if (aBundleName==NULL){ 
		aBundleName = CFBundleGetValueForInfoDictionaryKey(aBundle, kCFBundleExecutableKey);
	}
	
	//get bundle path
	CFStringRef bundleNameCFString = (CFStringRef)aBundleName;

	//CFRelease(aBundleName);
	//CFRelease(aBundle);
	
	return bundleNameCFString;
}

//----------------------------------------------------------
CFStringRef applicationLister::getApplicationPath(int _index){
	
	CFBundleRef aBundle = (CFBundleRef) CFArrayGetValueAtIndex(applicationBundleArray, _index);
	CFURLRef bundleExecutableUrl = CFBundleCopyExecutableURL(aBundle); //get the bundle executable url (executable name)
	CFStringRef bundleExecutablePathCFString = CFURLCopyFileSystemPath(CFURLCopyAbsoluteURL(bundleExecutableUrl),kCFURLPOSIXPathStyle); // copy CFURL to CFString
	
	CFRelease(bundleExecutableUrl);
	//CFRelease(aBundle); don't release bundle it's inside the array
	
	return bundleExecutablePathCFString;
	
}


//----------------------------------------------------------
int applicationLister::getApplicationCount(){
	
	size_t applicationBundleArraySize = CFArrayGetCount(applicationBundleArray);

	return applicationBundleArraySize;
}

//----------------------------------------------------------
void applicationLister::findExecutable(string path){
	
	/*
	 // get the path of our applications directory
	 applicationsDirectoryPath = getExecutablePath();
	 
	 size_t found;
	 found=applicationsDirectoryPath.find_last_of("/");
	 applicationsDirectoryPath = applicationsDirectoryPath.substr(0,found); // path to my folder
	 
	 applicationsDirectoryPath+="/Applications/"; // path to the Applications directory
	 
	 //cout << "Applications directory path is : " << applicationsDirectoryPath << endl;
	 */
	CFStringRef applicationsDirectoryPathCFString;
	applicationsDirectoryPathCFString = CFStringCreateWithCString(kCFAllocatorDefault,path.c_str(),kCFStringEncodingMacRoman);
	
	// Create a URL of applications directory.
	CFURLRef applicationsDirectoryUrl = CFURLCreateWithFileSystemPath(
																	  kCFAllocatorDefault,
																	  applicationsDirectoryPathCFString,		// file path name
																	  kCFURLPOSIXPathStyle,					// interpret as POSIX path
																	  true );   
	
	CFRelease(applicationsDirectoryPathCFString); // release CFStringRef
	
	if (!applicationsDirectoryUrl)
	{
		//CFShow(CFSTR("error creating applications directory url.\n"));
		return;
	}
	
	//
	
	//CFArrayRef applicationBundleArray; // array of applications bundle in the Applications directory
	
	CFArrayRef tmpApplicationBundleArray;
	
	tmpApplicationBundleArray = CFBundleCreateBundlesFromDirectory(kCFAllocatorDefault,applicationsDirectoryUrl, CFSTR("app"));
	
	if (tmpApplicationBundleArray){
		//CFShow(tmpApplicationBundleArray);
		CFArrayAppendArray(applicationBundleArray,tmpApplicationBundleArray, CFRangeMake(0, CFArrayGetCount(tmpApplicationBundleArray)));
	}
	
	CFRelease(applicationsDirectoryUrl); // release CFURLRef
	//CFRelease(tmpApplicationBundleArray); // release CFURLRef
	
}


//----------------------------------------------------------
void applicationLister::updateApplicationsList(){
	
	
	DIR *dir = NULL;
	struct dirent *entry;
	struct stat st;
	
	//if (applicationBundleArray) CFRelease(applicationBundleArray); // release if not null ?
	CFArrayRemoveAllValues(applicationBundleArray);
	
	// get the path of our applications directory
	applicationsDirectoryPath = getExecutablePath();
	
	size_t found;
	found=applicationsDirectoryPath.find_last_of("/");
	applicationsDirectoryPath = applicationsDirectoryPath.substr(0,found); // path to my folder
	
	applicationsDirectoryPath+="/Applications/"; // path to the Applications directory
	
	findExecutable(applicationsDirectoryPath);
	
	dir = opendir(applicationsDirectoryPath.c_str());
	
	string entry_name = "";
    string ext = "";
	string full_file_name;
	
	while ((entry = readdir(dir)) != NULL){
		//turn it into a C++ string
        entry_name = entry->d_name;
		full_file_name= applicationsDirectoryPath + entry_name;
		
		//lets get the length of the string here as we query it again
        int fileLen = entry_name.length();
		
		if(fileLen <= 0)continue; //if the name is not existant
		if(entry_name[0] == '.')continue; //ignore invisible files, ./ and ../
		
		if (stat(full_file_name.c_str(), &st) != -1){
			
			const bool is_directory = (st.st_mode & S_IFDIR ) != 0;
			
			if (is_directory) {
				findExecutable(full_file_name+"/");
				
				//printf("%s\n",entry->d_name );
				
			}
		}
		
	}
	
}

//----------------------------------------------------------
void applicationLister::dumpApplicationList(){

	//CFShow(applicationBundleArray);
}


//----------------------------------------------------------
string applicationLister::getExecutablePath(){
	
	char path[PATH_MAX];
	
	uint32_t size = sizeof(path);
	(_NSGetExecutablePath(path, &size) == 0);
	//if (_NSGetExecutablePath(path, &size) == 0)
		//printf("executables folder path is : %s/Applications/\n", path);
	//else
		//printf("buffer too small; need size : %u\n", size);
	
	return string(path);
	
}