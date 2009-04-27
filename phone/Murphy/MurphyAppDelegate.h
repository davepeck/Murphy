//
//  iMurphyAppDelegate.h
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class EAGLView;

@interface iMurphyAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    EAGLView *glView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;

@end

