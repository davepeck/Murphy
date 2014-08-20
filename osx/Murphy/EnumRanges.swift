//
//  EnumRanges.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

/* 
    just toying with some swift goodies --
    this is nuclear-level overkill for anything real
*/

// XXX Swift has a built-in RawRepresentable but the TypeAlias
// caused me all sorts of compiler pain in b3. Am I missing somthing?
// is it busted?

protocol IntRawRepresentable {
    class func fromRaw(raw: Int) -> Self?
    func toRaw() -> Int
}

class IntRawRepresentableRangeGenerator<T: IntRawRepresentable>: GeneratorType {
    typealias Element = T
    
    var current: Int
    let last: Int
    
    init(first: Int, last: Int) {
        self.current = first
        self.last = last
    }
    
    func next() -> T? {
        var v:T?
        if current <= last {
            v = T.fromRaw(current)
            current += 1
        }
        return v
    }
}

func enumerate<T: IntRawRepresentable>(first: T, last: T) -> SequenceOf<T> {
    return SequenceOf<T>({ IntRawRepresentableRangeGenerator(first: first.toRaw(), last: last.toRaw()) })
}

class MappedRangeGenerator<T, U> : GeneratorType {
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
        if v != nil {
            u = transform(v!)
        }
        return u
    }
}

extension SequenceOf {
    func map<U>(transform: (T) -> U) -> SequenceOf<U> {
        return SequenceOf<U>({ MappedRangeGenerator(generator: self.generate(), transform: transform) })
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

