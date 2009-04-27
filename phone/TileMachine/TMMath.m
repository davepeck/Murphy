//
//  TMMath.m
//  iMurphy
//
//  Created by Dave Peck on 4/21/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TMMath.h"


double LINEAR_MAP(double value, double value_min, double value_max, double target_min, double target_max)
{
    double zero_value = value - value_min;
    double value_range = value_max - value_min;
    double target_range = target_max - target_min;
    double zero_target_value = zero_value * (target_range / value_range);
    double target_value = zero_target_value + target_min;
	return target_value;
}

