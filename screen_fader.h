/*
 *  screen_fader.h
 *  Switchboard
 *
 *  Created by guillaume on 24/03/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreVideo/CoreVideo.h>
#include <CoreServices/CoreServices.h>

// Fade operations
enum {
	FillScreen,
	CleanScreen
};

void doFadeOperation(int operation, float time, bool sync);
