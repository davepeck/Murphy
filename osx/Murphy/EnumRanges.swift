//
//  EnumRanges.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

// Just toying with Swift types; this is nuclear-level nonsense for anything "real"
// Finally, with Swift b6, I can use Swift's RawRepresentable directly!

class RawRepresentableRangeGeneratorType<T: RawRepresentable where T.Raw : Comparable, T.Raw : ForwardIndexType>: GeneratorType {
    typealias Element = T
    typealias ElementRaw = T.Raw
    
    var current: ElementRaw
    let last: ElementRaw
    
    init(first: ElementRaw, last: ElementRaw) {
        self.current = first
        self.last = last
    }
    
    func next() -> T? {
        var v:Element?
        if (current <= last) {
            v = T.fromRaw(current)
            current++;
        }
        return v
    }
}


func enumerate<T: RawRepresentable where T.Raw : Comparable, T.Raw : ForwardIndexType>(first: T, last: T) -> SequenceOf<T> {
    return SequenceOf<T>({ RawRepresentableRangeGeneratorType(first: first.toRaw(), last: last.toRaw()) })
}


class MappedRangeGeneratorType<T, U> : GeneratorType {
    typealias Element = U
    
    var generator: GeneratorOf<T>
    let transform: (T) -> U
    
    init(generator: GeneratorOf<T>, transform: (T) -> U) {
        self.generator = generator
        self.transform = transform
    }
    
    func next() -> U? {
        var v:T? = generator.next()
        var u:U? = nil
        if let v = v {
            u = transform(v)
        }
        return u
    }
}


extension SequenceOf {
    func map<U>(transform: (T) -> U) -> SequenceOf<U> {
        return SequenceOf<U>({ MappedRangeGeneratorType(generator: self.generate(), transform: transform) })
    }
}


extension SequenceOf {
    func toArray() -> [T] {
        var a:Array<T> = []
        var g = self.generate()
        while let v = g.next() {
            a.append(v)
        }
        return a
    }
}

