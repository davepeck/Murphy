//
//  NSBuffer.m
//  iMurphy
//
//  Created by Dave Peck on 4/21/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "NSBuffer.h"

@interface NSBuffer (Private)

-(id)initWithCapacity:(NSUInteger)myCapacity;
-(void)dealloc;

@end

@implementation NSBuffer (Private)

-(id)initWithCapacity:(NSUInteger)myCapacity
{
	self = [super init];
	
	if (self != nil)
	{
		capacity = myCapacity;
		buffer = malloc(sizeof(id) * capacity);
		bzero(buffer, sizeof(id) * capacity);
		length = 0;
		head = 0;
	}
	
	return self;
}

-(void)dealloc
{
	for (NSUInteger i = 0; i < length; i++)
	{
		[buffer[i] release];
	}
	
	free(buffer);	
	[super dealloc];
}

@end

@implementation NSBuffer

+(id)bufferWithCapacity:(NSUInteger)myCapacity
{
	return [[[NSBuffer alloc] initWithCapacity:myCapacity] autorelease];
}

@synthesize capacity;
@synthesize length;

-(void)addObject:(id)object
{
	if (length < capacity)
	{
		buffer[length] = [object retain];
		length += 1;
	}
	else
	{
		if (buffer[head] != nil)
		{
			[buffer[head] release];
			buffer[head] = nil;
		}
			
		buffer[head] = [object retain];		
		head += 1;		
		
		if (head == capacity)
		{
			head = 0;
		}				
	}
}

- (id)objectAtIndex:(NSUInteger)index
{
	NSAssert(index < length, @"Index is out of bounds.");
	
	if (length < capacity)
	{
		return buffer[index];
	}
	else
	{
		// head represents the "real" zero index into the array.
		NSUInteger finalIndex = head + index;
		if (finalIndex >= capacity)
		{
			finalIndex -= capacity;
		}
		NSAssert(finalIndex < capacity, @"This should never happen!");
		
		return buffer[finalIndex];
	}
}

@end
