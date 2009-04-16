//
//  MurphyLevel.h
//  iMurphy
//
//  Created by Dave Peck on 4/15/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface MurphyLevel : NSObject {
	NSString *name;
	uint16_t infotrons;
	uint16_t width;
	uint16_t height;
	uint8_t *iconS;
	uint8_t *iconT;
}

@property (readonly, copy) NSString *name;
@property (readonly) uint16_t infotrons;
@property (readonly) uint16_t width;
@property (readonly) uint16_t height;

+(id)murphyLevelWithData:(NSData*)data;
+(id)murphyLevelWithFile:(NSString*)path;
+(id)murphyLevelWithNamedResource:(NSString*)name;

-(uint8_t) getIconSForX:(uint16_t)x Y:(uint16_t)y;
-(uint8_t) getIconTForX:(uint16_t)x Y:(uint16_t)y;

@end
