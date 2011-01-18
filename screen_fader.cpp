/*
 *  screen_fader.cpp
 *  Switchboard
 *
 *  Created by guillaume on 24/03/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "screen_fader.h"


////////////////////////////////////////////////////////////
/// Perform fade operation where 'operation' is one of {FillScreen, CleanScreen}
/// and 'time' is the time during which you wish the operation to be performed.
/// Set 'sync' to true if you do not want the method to end before the end
/// of the fade operation. Pass the last used token or a new one if you are
/// using this method for the first time. This lets the method release some
/// resources when doing CleanScreen operation.
////////////////////////////////////////////////////////////


//void doFadeOperation(int operation, float time, bool sync) // from SMFL
void doFadeOperation(int operation, float time, bool sync)
{
	static CGDisplayFadeReservationToken prevToken = kCGDisplayFadeReservationInvalidToken;
	CGDisplayFadeReservationToken token = prevToken;
	
	CGError result = 0, capture = 0;
	
	if (operation == FillScreen) {
		// Get access for the fade operation
		result = CGAcquireDisplayFadeReservation((int)(3 + time), &token);
		
		if (!result) {
			// Capture display but do not fill the screen with black
			// so that we can see the fade operation
			capture = CGDisplayCaptureWithOptions(kCGDirectMainDisplay, kCGCaptureNoFill);
			
			if (!capture) {
				// Do the increasing fade operation
				CGDisplayFade(token, time,
							  kCGDisplayBlendNormal,
							  kCGDisplayBlendSolidColor,
							  0.0f, 0.0f, 0.0f, sync);
				
				// Now, release the non black-filling capture
				CGDisplayRelease(kCGDirectMainDisplay);
				
				// And capture with filling
				// so that we don't see the switching in the meantime
				CGDisplayCaptureWithOptions(kCGDirectMainDisplay, kCGCaptureNoOptions);
			}
			
			prevToken = token;
		}
	} else if (operation == CleanScreen) {
		// Get access for the fade operation
		if (token == kCGDisplayFadeReservationInvalidToken)
			result = CGAcquireDisplayFadeReservation((int)(3 + time), &token);
		
		if (!result) {
			if (!capture) {
				// Release the black-filling capture
				CGDisplayRelease(kCGDirectMainDisplay);
				
				// Capture the display but do not fill with black (still for the fade operation)
				CGDisplayCaptureWithOptions(kCGDirectMainDisplay, kCGCaptureNoFill);
				
				// Do the decreasing fading
				CGDisplayFade(token, time,
							  kCGDisplayBlendSolidColor,
							  kCGDisplayBlendNormal,
							  0.0f, 0.0f, 0.0f, sync);
				
				// Release the fade operation token
				CGReleaseDisplayFadeReservation(token);
				
				// Invalidate the given token
				prevToken = kCGDisplayFadeReservationInvalidToken;
			}
			
			// Release the captured display
			CGDisplayRelease(kCGDirectMainDisplay);
		}
	}
}
