/***********************************************************************
 ** __MyCompanyName__
 ** guillaume
 ** Copyright (c) 2009. All rights reserved.
 **********************************************************************/

#include "applicationLister.h"
#include <dirent.h>

// for NSLog
#if __cplusplus
extern "C" {
#endif
    
	void NSLog(CFStringRef format, ...);
	void NSLogv(CFStringRef format, va_list args);
    
#if __cplusplus
}
#endif

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
	
	return bundleExecutablePathCFString;
	
}

//----------------------------------------------------------
int applicationLister::getApplicationIndex(CFStringRef _searchName){
	
	CFStringRef applicationName;
	for (int i = 0; i< getApplicationCount(); i++){
		applicationName = getApplicationName(i);
		
		if (CFStringCompare(applicationName, _searchName, kCFCompareCaseInsensitive)==kCFCompareEqualTo)
		{
			return i;
		}
	}
	return -1;
}

//----------------------------------------------------------
CFStringRef applicationLister::getApplicationEnclosingDirectoryPath(int _index){
	
	CFBundleRef aBundle = (CFBundleRef) CFArrayGetValueAtIndex(applicationBundleArray, _index);
	
	CFURLRef bundleUrl = CFBundleCopyBundleURL(aBundle); // get bundle url
	
	CFURLRef directoryUrl = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault,bundleUrl);
	CFStringRef bundleExecutablePathCFString = CFURLCopyFileSystemPath(CFURLCopyAbsoluteURL(directoryUrl),kCFURLPOSIXPathStyle); // copy CFURL to CFString
	
	CFShow(bundleExecutablePathCFString);
	
	CFRelease(bundleUrl);
	CFRelease(directoryUrl);
	
	return bundleExecutablePathCFString;
	
}

//----------------------------------------------------------
int applicationLister::getApplicationCount(){
	
	size_t applicationBundleArraySize = CFArrayGetCount(applicationBundleArray);
	return applicationBundleArraySize;
}

//----------------------------------------------------------
OSStatus applicationLister::findExecutable(CFURLRef enclosingDirectoryURL){
	
	OSStatus err = noErr;
	CFArrayRef tmpApplicationBundleArray;
	
	if (enclosingDirectoryURL == NULL) { err = true; }
	
	if (err==noErr){
		tmpApplicationBundleArray = CFBundleCreateBundlesFromDirectory(
                                                                       kCFAllocatorDefault,
                                                                       enclosingDirectoryURL, 
                                                                       CFSTR("app")
                                                                       );
		if (tmpApplicationBundleArray==NULL){ err = true; }
	}
	
	if (err==noErr){
		CFArrayAppendArray(
                           applicationBundleArray,
                           tmpApplicationBundleArray,
                           CFRangeMake(0, CFArrayGetCount(tmpApplicationBundleArray))
                           );
	}
	
	return err;
}


//----------------------------------------------------------
OSStatus applicationLister::updateApplicationsList(){
	
	OSStatus err = noErr;
	
	CFURLRef meURL;
	CFURLRef parentURL;
	
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	struct stat stateResult;
	char pathBuffer[PATH_MAX];
	
	CFArrayRemoveAllValues(applicationBundleArray);
	
	// get my own path
	meURL = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
	
	// get a CFURL of the parent directory
	if (err == noErr){ 
		parentURL = CFURLCreateCopyDeletingLastPathComponent (kCFAllocatorDefault, meURL);
		if (parentURL == NULL) { err = true; }
	}
	
	if (err == noErr){ 
		applicationsDirectoryURL = CFURLCreateCopyAppendingPathComponent(
                                                                         kCFAllocatorDefault,
                                                                         parentURL,
                                                                         CFSTR("Applications"),
                                                                         true
                                                                         );
		if (applicationsDirectoryURL == NULL) { err = true; }
	}
	
	// get c_string version of URL
	err = !CFURLGetFileSystemRepresentation ( 
											 applicationsDirectoryURL,
											 true,
											 (UInt8 *)pathBuffer,
											 PATH_MAX
											 );
	
	// check if directory exist
	if (err == noErr){
		err = stat(pathBuffer,&stateResult);
	} else NSLog(CFSTR("ApplicationLister : Error getting file system representation !"));
	
	// find first level applications
	if (err == noErr){
		err = findExecutable(applicationsDirectoryURL);
	} else NSLog(CFSTR("ApplicationLister : Applications directory does not exist !"));
	
	
	
	// find applications in sub directory
	if (err == noErr){
		dir = opendir(pathBuffer);
		
		while ((entry = readdir(dir)) != NULL){
			
			if ((entry->d_name[0] != '\0')&&(entry->d_name[0] != '.')) //ignore invisible files, ./ and ../
			{
				string fullPath =string(pathBuffer)+"/"+string(entry->d_name); // build absolute path to entry 
				//printf("%s\n",fullPath.c_str());
				
				if (stat(fullPath.c_str(), &stateResult) != -1){
					if ((stateResult.st_mode & S_IFDIR ) != 0) { // is entry a directory ?
                        CFStringRef subDirName = CFStringCreateWithCString( 
                                                                           kCFAllocatorDefault, 
                                                                           entry->d_name, 
                                                                           kCFStringEncodingUTF8 
                                                                           );
                        
						CFURLRef subDirURL =CFURLCreateCopyAppendingPathComponent(
                                                                                  kCFAllocatorDefault,
                                                                                  applicationsDirectoryURL,
                                                                                  subDirName,
                                                                                  true
                                                                                  );
						CFRelease(subDirName);
						findExecutable(subDirURL);
					}
					
				}
			}
			
		}
		
		if ( dir != NULL )
		{
			closedir( dir );
			dir = NULL;
		}
	}else NSLog(CFSTR("ApplicationLister : Couldn't find executables !"));
	
	
	return err;
}