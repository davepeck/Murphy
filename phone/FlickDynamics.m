//
//  FlickDynamics.m
//  iMurphy
//
//  Created by Dave Peck <davepeck [at] davepeck [dot] org> on 4/23/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "FlickDynamics.h"

/* these assume a 1.0 x 1.0 viewport at 60FPS */

// these constants were determined by experimentation
const double DEFAULT_MOTION_DAMP = 0.95;
const double DEFAULT_MOTION_MINIMUM = 0.0001;
const double DEFAULT_FLICK_THRESHOLD = 0.01;
const double DEFAULT_ANIMATION_RATE = 1.0f / 60.0f;
const double DEFAULT_MOTION_MULTIPLIER = 0.25f;

const double MOTION_MAX = 0.065f;
const NSTimeInterval FLICK_TIME_BACK = 0.07;
const NSUInteger DEFAULT_CAPACITY = 20;


@interface FlickDynamics (FlickDynamicsPrivate)

-(id)initWithViewportWidth:(double)myViewportWidth viewportHeight:(double)myViewportHeight scrollBoundsLeft:(double)myScrollBoundsLeft scrollBoundsTop:(double)myScrollBoundsTop scrollBoundsRight:(double)myScrollBoundsRight scrollBoundsBottom:(double)myScrollBoundsBottom animationRate:(NSTimeInterval)myAnimationRate;
-(void)dealloc;

-(void)clearHistory;
-(void)addToHistory:(TouchInfo)info;
-(TouchInfo)getHistoryAtIndex:(NSUInteger)index;
-(TouchInfo)getRecentHistory;

-(void)ensureValidScrollPosition;

-(double)linearMap:(double)value valueMin:(double)valueMin valueMax:(double)valueMax targetMin:(double)targetMin targetMax:(double)targetMax;
-(double)linearInterpolate:(double)from to:(double)to percent:(double)percent;

@end

@implementation FlickDynamics (FlickDynamicsPrivate)

-(id)initWithViewportWidth:(double)myViewportWidth viewportHeight:(double)myViewportHeight scrollBoundsLeft:(double)myScrollBoundsLeft scrollBoundsTop:(double)myScrollBoundsTop scrollBoundsRight:(double)myScrollBoundsRight scrollBoundsBottom:(double)myScrollBoundsBottom animationRate:(NSTimeInterval)myAnimationRate
{
	self = [super init];
	
	if (self != nil)
	{
		// "history" is a buffer of the last N touches. For performance, it is 
		// managed as a circular queue; older items are just dropped from it.
		history = (TouchInfo*) malloc(sizeof(TouchInfo) * DEFAULT_CAPACITY);
		historyCount = 0;
		historyHead = 0;
		
		currentScrollLeft = 0.0;
		currentScrollTop = 0.0;
		
		animationRate = myAnimationRate;
		
		viewportWidth = myViewportWidth;
		viewportHeight = myViewportHeight;
		
		scrollBoundsLeft = myScrollBoundsLeft;
		scrollBoundsTop = myScrollBoundsTop;
		scrollBoundsRight = myScrollBoundsRight;
		scrollBoundsBottom = myScrollBoundsBottom;		
		
		// our default constants assume a 1.0 x 1.0 viewport at 60FPS.
		// here is where we scale them. Only some of our constants are FPS dependent.
		double animationRateAdjustment = myAnimationRate / DEFAULT_ANIMATION_RATE;
		double xAdjustment = myViewportWidth / 1.0;
		double yAdjustment = myViewportHeight / 1.0;
		double viewportAdjustment = (xAdjustment + yAdjustment) / 2.0;
		
		motionDamp = pow(DEFAULT_MOTION_DAMP, animationRateAdjustment);
		motionMultiplier = DEFAULT_MOTION_MULTIPLIER * viewportAdjustment;
		motionMinimum = DEFAULT_MOTION_MINIMUM * viewportAdjustment;		
		flickThresholdX = DEFAULT_FLICK_THRESHOLD * xAdjustment;
		flickThresholdY = DEFAULT_FLICK_THRESHOLD * yAdjustment;
		
		motionX = 0.0;
		motionY = 0.0;
	}
	
	return self;
}

-(void)dealloc
{
	if (history != nil)
	{
		free(history);
		history = nil;
	}
	
	[super dealloc];
}

-(void)clearHistory
{
	historyCount = 0;
	historyHead = 0;
}

-(void)addToHistory:(TouchInfo)info
{
	NSUInteger rawIndex;
	
	if (historyCount < DEFAULT_CAPACITY)
	{
		rawIndex = historyCount;
		historyCount += 1;
	}
	else
	{
		rawIndex = historyHead;
		historyHead += 1;
		if (historyHead == DEFAULT_CAPACITY)
		{
			historyHead = 0;
		}
	}
	
	history[rawIndex].x = info.x;
	history[rawIndex].y = info.y;
	history[rawIndex].time = info.time;
}

-(TouchInfo)getHistoryAtIndex:(NSUInteger)index
{
	NSUInteger rawIndex = historyHead + index;
	
	if (rawIndex >= DEFAULT_CAPACITY)
	{
		rawIndex -= DEFAULT_CAPACITY;
	}
	
	return history[rawIndex];
}

-(TouchInfo)getRecentHistory
{
	return [self getHistoryAtIndex:(historyCount-1)];
}

-(void)ensureValidScrollPosition
{
	if (currentScrollLeft + viewportWidth > scrollBoundsRight)
	{
		currentScrollLeft = scrollBoundsRight - viewportWidth;
	}
	
	if (currentScrollLeft < scrollBoundsLeft)
	{
		currentScrollLeft = scrollBoundsLeft;
	}	
	
	if (scrollBoundsBottom < scrollBoundsTop)
	{
		// inverted (gl-style) viewport
		if (currentScrollTop - viewportHeight < scrollBoundsBottom)
		{
			currentScrollTop = scrollBoundsBottom + viewportHeight;
		}		
		
		if (currentScrollTop > scrollBoundsTop)
		{
			currentScrollTop = scrollBoundsTop;
		}
	}
	else
	{
		// regular (Y increases downward) viewport
		if (currentScrollTop + viewportHeight > scrollBoundsBottom)
		{
			currentScrollTop = scrollBoundsBottom - viewportHeight;
		}
		
		if (currentScrollTop < scrollBoundsTop)
		{
			currentScrollTop = scrollBoundsTop;
		}
	}		
}

-(double)linearMap:(double)value valueMin:(double)valueMin valueMax:(double)valueMax targetMin:(double)targetMin targetMax:(double)targetMax
{
    double zeroValue = value - valueMin;
    double valueRange = valueMax - valueMin;
    double targetRange = targetMax - targetMin;
    double zeroTargetValue = zeroValue * (targetRange / valueRange);
    double targetValue = zeroTargetValue + targetMin;
	return targetValue;
}

-(double)linearInterpolate:(double)from to:(double)to percent:(double)percent
{
	return (from * (1.0f - percent)) + (to * percent);
}

@end

@implementation FlickDynamics

+(id)flickDynamicsWithViewportWidth:(double)myViewportWidth viewportHeight:(double)myViewportHeight scrollBoundsLeft:(double)myScrollBoundsLeft scrollBoundsTop:(double)myScrollBoundsTop scrollBoundsRight:(double)myScrollBoundsRight scrollBoundsBottom:(double)myScrollBoundsBottom animationRate:(NSTimeInterval)myAnimationRate
{
	return [[[FlickDynamics alloc] initWithViewportWidth:myViewportWidth viewportHeight:myViewportHeight scrollBoundsLeft:myScrollBoundsLeft scrollBoundsTop:myScrollBoundsTop scrollBoundsRight:myScrollBoundsRight scrollBoundsBottom:myScrollBoundsBottom animationRate:myAnimationRate] autorelease];
}

+(id)flickDynamicsWithViewportWidth:(double)myViewportWidth viewportHeight:(double)myViewportHeight scrollBoundsLeft:(double)myScrollBoundsLeft scrollBoundsTop:(double)myScrollBoundsTop scrollBoundsRight:(double)myScrollBoundsRight scrollBoundsBottom:(double)myScrollBoundsBottom
{
	return [FlickDynamics flickDynamicsWithViewportWidth:myViewportWidth viewportHeight:myViewportHeight scrollBoundsLeft:myScrollBoundsLeft scrollBoundsTop:myScrollBoundsTop scrollBoundsRight:myScrollBoundsRight scrollBoundsBottom:myScrollBoundsBottom animationRate:DEFAULT_ANIMATION_RATE];
}

@synthesize currentScrollLeft;
@synthesize currentScrollTop;

-(void)startTouchAtX:(double)x y:(double)y
{
	[self stopMotion];
	[self clearHistory];
	
	TouchInfo info;
	info.x = x;
	info.y = y;
	info.time = [[NSDate date] timeIntervalSince1970];
	
	[self addToHistory:info];
}

-(void)moveTouchAtX:(double)x y:(double)y
{
	TouchInfo old = [self getRecentHistory];
	
	TouchInfo new;
	new.x = x;
	new.y = y;
	new.time = [[NSDate date] timeIntervalSince1970];
	[self addToHistory:new];
	
	currentScrollLeft += (old.x - new.x);
	currentScrollTop += (old.y - new.y);	
	[self ensureValidScrollPosition];
}

-(void)endTouchAtX:(double)x y:(double)y
{	
	TouchInfo old = [self getRecentHistory];
	TouchInfo last;
	last.x = x;
	last.y = y;
	last.time = [[NSDate date] timeIntervalSince1970];
	[self addToHistory:last];

	// do the standard scrolling motion in response
	currentScrollLeft += (old.x - last.x);
	currentScrollTop += (old.y - last.y);	
	[self ensureValidScrollPosition];
	
	// find the first point in our touch history that is younger than FLICK_TIME_BACK seconds.
	// this point, and the point of release, will allow us to find our vector for motion.
	NSTimeInterval crossoverTime = last.time - FLICK_TIME_BACK;
	NSUInteger recentIndex = 0;
	for (NSUInteger testIndex = 0; testIndex < historyCount; testIndex++)
	{
		TouchInfo testInfo = [self getHistoryAtIndex:testIndex];
		if (testInfo.time > crossoverTime)
		{
			recentIndex = testIndex;
			break;
		}		
	}
	
	if (recentIndex == 0)
	{
		// this is a very fast gesture. we will want to interpolate this point
		// and the next _as if_ they projected out to where the touch would have
		// been at time NOW - FLICK_TIME_BACK
		recentIndex += 1;
	}
	
	// We have the two points closest to FLICK_TIME_BACK seconds
	// Use linear interpolation to decide where the point _would_ have been at FLICK_TIME_BACK seconds
	TouchInfo recentInfo = [self getHistoryAtIndex:recentIndex];
	TouchInfo previousInfo = [self getHistoryAtIndex:(recentIndex - 1)];	
	double crossoverTimePercent = [self linearMap:crossoverTime valueMin:previousInfo.time valueMax:recentInfo.time targetMin:0.0f targetMax:1.0f];
	double flickX = [self linearInterpolate:previousInfo.x to:recentInfo.x percent:crossoverTimePercent];
	double flickY = [self linearInterpolate:previousInfo.y to:recentInfo.y percent:crossoverTimePercent];
	
	// Dampen the motion along each axis if it is too small to matter
	if (fabs(last.x - flickX) < flickThresholdX)
	{
		flickX = last.x;
	}
	
	if (fabs(last.y - flickY) < flickThresholdY)
	{
		flickY = last.y;
	}
	
	// this is not a flick gesture if there is no motion after interpolation and dampening
	if ((last.x == flickX) && (last.y == flickY))
	{
		return;
	}
	
	// determine our raw motion
	double rawMotionX = (flickX - last.x) * motionMultiplier;
	double rawMotionY = (flickY - last.y) * motionMultiplier;
	
	// Clamp down on motion to prevent extreme speeds.
	// To keep the direction of motion correct, make sure to
	// preserve the "aspect ratio."
	double absX = fabs(rawMotionX);
	double absY = fabs(rawMotionY);
	if (absX >= MOTION_MAX && absX >= absY)
	{
		double scaleFactor = MOTION_MAX / absX;
		rawMotionX *= scaleFactor;
		rawMotionY *= scaleFactor;
	}
	else if (absY >= MOTION_MAX)
	{
		double scaleFactor = MOTION_MAX / absY;
		rawMotionX *= scaleFactor;
		rawMotionY *= scaleFactor;
	}
	
	// done! assign our motion!
	motionX = rawMotionX;
	motionY = rawMotionY;
}

-(void)animate
{
	if (motionX == 0.0 && motionY == 0.0)
	{
		return;
	}
	
	currentScrollLeft += motionX;
	currentScrollTop += motionY;
	
	motionX *= motionDamp;
	motionY *= motionDamp;
	
	if (fabs(motionX) < motionMinimum)
	{
		motionX = 0.0;
	}
	
	if (fabs(motionY) < motionMinimum)
	{
		motionY = 0.0;
	}
	
	[self ensureValidScrollPosition];
}

-(void)stopMotion
{
	motionX = 0.0;
	motionY = 0.0;
}

@end
