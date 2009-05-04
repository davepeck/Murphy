//
//  MurphyAppDelegate.h
//  Murphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MurphyView;

@interface MurphyAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    MurphyView *glView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet MurphyView *glView;

@end

