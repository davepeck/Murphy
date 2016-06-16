//
//  EnumRanges.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

class RawRepresentableGenerator <T: RawRepresentable where T.RawValue : Strideable> : IteratorProtocol {
    var current: T.RawValue
    let last: T.RawValue
    
    init(first: T, last: T) {
        self.current = first.rawValue
        self.last = last.rawValue
    }
    
    func next() -> T? {
        var v:T?
        if (current <= last) {
            v = T(rawValue:current)
            current = current.advanced(by: 1)
        }
        return v
    }
    
}


extension Strideable {
    
}


extension RawRepresentable where Self.RawValue : Strideable {
    func to(_ last: Self) -> AnySequence<Self> {
        return AnySequence(AnyIterator(RawRepresentableGenerator(first:self, last: last)))
    }
}


// This is probably a step too far.
func ...<T : RawRepresentable where T.RawValue : Strideable>(start: T, end: T) -> AnySequence<T>
{
    return start.to(end)
}
