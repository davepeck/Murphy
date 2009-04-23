//
//  FlickDynamics.h
//  iMurphy
//
//  Implements "flick/scroll" behavior independent of any particular view/UI context.
//  You could use it to add flick/scroll behavior, for example, to your OpenGL game.
//
//  Created by Dave Peck on 4/23/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef struct TouchInfo {
	double x;
	double y;
	NSTimeInterval time; // all relative to the 1970 GMT epoch
} TouchInfo;

@interface FlickDynamics : NSObject {	
	TouchInfo *history;
	NSUInteger historyCount;
	NSUInteger historyHead;

	double currentScrollLeft;
	double currentScrollTop;
	
	double animationRate;
	
	double viewportWidth;
	double viewportHeight;	
	
	double scrollBoundsLeft;
	double scrollBoundsTop;
	double scrollBoundsRight;
	double scrollBoundsBottom;
	
	double motionX;
	double motionY;
	
	double motionDamp;
	double motionMultiplier;
	double motionMinimum;
	double flickThresholdX;
	double flickThresholdY;	
}

+(id)flickDynamicsWithViewportWidth:(double)viewportWidth viewportHeight:(double)viewportHeight scrollBoundsLeft:(double)scrollBoundsLeft scrollBoundsTop:(double)scrollBoundsTop scrollBoundsRight:(double)scrollBoundsRight scrollBoundsBottom:(double)scrollBoundsBottom animationRate:(NSTimeInterval)animationRate;
+(id)flickDynamicsWithViewportWidth:(double)viewportWidth viewportHeight:(double)viewportHeight scrollBoundsLeft:(double)scrollBoundsLeft scrollBoundsTop:(double)scrollBoundsTop scrollBoundsRight:(double)scrollBoundsRight scrollBoundsBottom:(double)scrollBoundsBottom;

@property (readwrite) double currentScrollLeft;
@property (readwrite) double currentScrollTop;

-(void)startTouchAtX:(double)x y:(double)y;
-(void)moveTouchAtX:(double)x y:(double)y;
-(void)endTouchAtX:(double)x y:(double)y;
-(void)animate; /* call this with whatever periodicity you specified on initialization */
-(void)stopMotion;

@end
