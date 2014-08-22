//
//  BinaryDataScanner.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

/* 
    Toying with tools to help read binary formats.

    I've seen lots of approaches in swift that create
    an intermediate object per-read (usually another NSData)
    but even if these are lightweight under the hood,
    it seems like overkill. Plus this taught me about <()>
 
    And it would be nice to have an extension to
    NSFileHandle too that does much the same.
*/

protocol BinaryReadable {
    var littleEndian: Self { get }
    var bigEndian: Self { get }
}

extension UInt8: BinaryReadable {
    var littleEndian: UInt8 { return self }
    var bigEndian: UInt8 { return self }
}

extension UInt16: BinaryReadable {}

extension UInt32: BinaryReadable {}

extension UInt64: BinaryReadable {}

class BinaryDataScanner {
    let data: NSData
    let littleEndian: Bool
    let encoding: NSStringEncoding

    var current: UnsafePointer<Void>
    var remaining: Int
    
    init(data: NSData, littleEndian: Bool, encoding: NSStringEncoding) {
        self.data = data
        self.littleEndian = littleEndian
        self.encoding = encoding

        self.current = self.data.bytes
        self.remaining = self.data.length
    }
    
    func read<T: BinaryReadable>() -> T? {
        if remaining < sizeof(T) {
            return nil
        }
        
        let tCurrent = UnsafePointer<T>(current)
        let v = tCurrent.memory
        current = UnsafePointer<Void>(tCurrent.successor())
        remaining -= sizeof(T)
        return littleEndian ? v.littleEndian : v.bigEndian
    }
    
    func readNullTerminatedString() -> String? {
        var string:String? = nil
        var tCurrent = UnsafePointer<UInt8>(current)
        var count: Int = 0
        
        // scan
        while (remaining > 0 && tCurrent.memory != 0) {
            remaining -= 1
            count += 1
            tCurrent = tCurrent.successor()
        }
        
        // create string if available
        if (remaining > 0 && tCurrent.memory == 0) {
            string = NSString(bytes: current, length: count, encoding: encoding) as String
            current = UnsafePointer<()>(tCurrent.successor())
            remaining -= 1
        }
        
        return string
    }
}
