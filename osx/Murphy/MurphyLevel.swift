//
//  MurphyLevel.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation


let TILESET_WIDTH = 10

struct MurphyLevel {
    let name: String
    let graphicsSetName: String
    let infotrons: Int
    let width: Int
    let height: Int
    let grid: Array<LevelTile>
    
    func gridIndexAt(x: Int, y: Int) -> Int? {
        guard (0 <= x) && (x < width) && (0 <= y) && (y < height) else {
            return nil
        }
        
        return (y * width) + x
    }
    
    func tileAt(x: Int, y: Int) -> LevelTile? {
        guard let i = gridIndexAt(x, y: y) else {
            return nil
        }
        
        return grid[i]
    }
    
    func textureNameAt(x: Int, y:Int) -> String? {
        guard let tile = tileAt(x, y: y) else {
            return nil
        }

        return tile.textureName()
    }
    
    static func fromResourceNamed(name: String) -> MurphyLevel? {
        guard let path = NSBundle.mainBundle().pathForResource(name, ofType: "mlv") else {
            return nil
        }
        
        return MurphyLevel.fromFileNamed(path)
    }
    
    static func fromFileNamed(path: String) -> MurphyLevel? {
        var result:MurphyLevel? = nil
        do {
            let data:NSData = try NSData(contentsOfFile:path, options: [])
            result = MurphyLevel.fromData(data)
        } catch {}
        return result
    }
    
    static func fromData(data: NSData) -> MurphyLevel? {
        let scanner = BinaryDataScanner(data: data, littleEndian: false, encoding: NSASCIIStringEncoding)
        
        guard let name = scanner.readNullTerminatedString(),
            let graphicsSetName = scanner.readNullTerminatedString(),
            let infotrons:UInt16 = scanner.read(),
            let width:UInt16 = scanner.read(),
            let height:UInt16 = scanner.read() else {
                return nil
        }
        
        let expectedRemaining = Int(width) * Int(height) * 2
        
        guard scanner.remaining == expectedRemaining else {
            return nil
        }
        
        var grid = Array<LevelTile>()
        
        for _ in 0..<Int(height) {
            for _ in 0..<Int(width) {
                // smooth over stupid thing in old .mbl files
                let tileMapX:UInt8 = scanner.read()!
                let tileMapY:UInt8 = scanner.read()!
                let tileMapRaw = (Int(tileMapY) * TILESET_WIDTH) + Int(tileMapX)
                guard let tile = LevelTile(rawValue:tileMapRaw) else {
                    return nil
                }
                grid.append(tile)
            }
        }
        
        return MurphyLevel(name: name, graphicsSetName: graphicsSetName, infotrons: Int(infotrons), width: Int(width), height: Int(height), grid: grid)
    }
}
