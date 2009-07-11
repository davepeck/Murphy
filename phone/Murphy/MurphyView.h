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
#import "TMTileGrid.h"
#import "FlickDynamics.h"

@interface MurphyView : UIView<TMEngineDelegate>
{		
	TMEngine *tileEngine;	
	
	TMTileGrid *tileGrid; /* shared with engine -- for now */
	
	FlickDynamics *flickDynamics;
	NSUInteger currentLevel;
	NSArray *levelNames;	
}

- (void)setAnimationRate:(NSTimeInterval)animationRate;
- (void)startAnimation;

@end
