//
//  iMurphyAppDelegate.m
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//

#import "iMurphyAppDelegate.h"
#import "EAGLView.h"

@implementation iMurphyAppDelegate

@synthesize window;
@synthesize glView;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
	glView.animationInterval = 1.0 / 1.0;
	[glView startAnimation];
}


- (void)applicationWillResignActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / 1.0;
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
	glView.animationInterval = 1.0 / 1.0;
}


- (void)dealloc {
	[window release];
	[glView release];
	[super dealloc];
}

@end
