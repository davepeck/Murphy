//
//  NSBuffer.h
//  iMurphy
//
//  Created by Dave Peck on 4/21/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface NSBuffer : NSObject {
	id *buffer;
	NSUInteger capacity;
	NSUInteger length;
	NSUInteger head; /* a circular array */
}

+(id)bufferWithCapacity:(NSUInteger)capacity;

@property (readonly) NSUInteger capacity;
@property (readonly) NSUInteger length;

-(void)addObject:(id)object;
-(id)objectAtIndex:(NSUInteger)index;

@end
