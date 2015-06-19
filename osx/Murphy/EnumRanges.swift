//
//  EnumRanges.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

class RawRepresentableGenerator <T: RawRepresentable where T.RawValue : Comparable, T.RawValue : ForwardIndexType> : AnyGenerator<T> {
    typealias Element = T
    typealias ElementRaw = T.RawValue
    
    var current: ElementRaw
    let last: ElementRaw
    
    init(first: Element, last: Element) {
        self.current = first.rawValue
        self.last = last.rawValue
    }
    
    override func next() -> T? {
        var v:Element?
        if (current <= last) {
            v = T(rawValue:current)
            current = current.successor()
        }
        return v
    }
    
}


extension RawRepresentable where Self.RawValue : Comparable, Self.RawValue : ForwardIndexType {
    func to(last: Self) -> AnySequence<Self> {
        return AnySequence(RawRepresentableGenerator(first:self, last: last))
    }
}


extension AnySequence {
    func toArray() -> [T]
    {
        return [T](self)
    }
}


// Not all overloads of ... produce a Range in the standard library, but still, this is probably a step too far.
func ...<T : RawRepresentable where T.RawValue : Comparable, T.RawValue: ForwardIndexType>(start: T, end: T) -> AnySequence<T>
{
    return start.to(end)
}
