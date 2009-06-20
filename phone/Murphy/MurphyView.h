//
//  MurphyView.h
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//


#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "TMEngine.h"
#import "FlickDynamics.h"

@interface MurphyView : UIView
{
@private
	TMEngine *tileEngine;
	
	NSUInteger currentLevel;
	NSArray *levelNames;
	TMTileGrid *tileGrid;
	
	FlickDynamics *flickDynamics;
}

- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView;

@property NSTimeInterval animationInterval;

@end
