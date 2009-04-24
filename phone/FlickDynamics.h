//
//  FlickDynamics.h
//  (c) 2009 Dave Peck <davepeck [at] davepeck [dot] org> 
//  http://davepeck.org/
//
//  This code is released under the BSD license. If you use my code in your product,
//  please put my name somewhere in the credits and let me know about it!
//
//  This code mimics the scroll/flick dynamics of the iPhone UIScrollView.
//  What's cool about this code is that it is entirely independent of any iPhone
//  UI, so you can use it to provide scroll/flick behavior on your custom views.
//
//  The key thing (which you'll learn fast if you try and build this yourself) is that
//  you can't just rely on the last two points to compute your motion vector. Instead
//  you need to "look back in time" to figure out where the touch was, say, 0.07 seconds
//  ago. That will give you a much better sense of your vector and speed.
//
//  In order to answer the question "where was the touch 0.07 seconds ago" we keep a 
//  history of the last N touches. When the user's touch is released, we look back through
//  the history and use linear interpolation to determine where the touch _would have been_
//  had we recorded a touch at exactly 0.07 seconds ago. We use that point as the basis
//  for our motion vector. To ensure that we never scroll "too fast," we clamp down on
//  any large motion that we compute, being sure to maintain the direction while reducing
//  the magnitude of motion.
//
//  This code is coordinate system agnostic. I've chosen constants that made sense for 
//  a coordinate system where the viewport is 1.0 by 1.0 in size. However, when you
//  initialize this code, it will scale the constants as appropriate for your viewport size.
//  (For example, it works fine if your viewport is 320 x 480 in size.)
//
//  This code expects that you already have an animation loop running. By default, the
//  expectation is that you will call animate: sixty times a second. If you want to
//  run at a different rate, be sure to initialize this class with your expected animation
//  rate. Again, the built-in constants will be scaled to match.
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
