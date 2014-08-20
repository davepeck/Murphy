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
        var gridIndex:Int? = nil
        
        if (0 <= x) && (x < width) && (0 <= y) && (y < height) {
            gridIndex = (y * width) + x
        }
        
        return gridIndex
    }
    
    func tileAt(x: Int, y: Int) -> LevelTile? {
        var tile:LevelTile? = nil
        let i:Int? = gridIndexAt(x, y: y)
        
        if let gi = i {
            tile = grid[gi]
        }

        return tile
    }
    
    func textureNameAt(x: Int, y:Int) -> String? {
        var textureName:String? = nil
        
        if let tile = tileAt(x, y: y) {
            textureName = tile.textureName()
        }
        
        return textureName
    }
    
    static func fromResourceNamed(name: String) -> MurphyLevel? {
        let path = NSBundle.mainBundle().pathForResource(name, ofType: "mlv")
        if path != nil {
            return MurphyLevel.fromFileNamed(path!)
        }
        return nil
    }
    
    static func fromFileNamed(path: String) -> MurphyLevel? {
        var error:NSError? = nil
        let data:NSData = NSData.dataWithContentsOfFile(path, options: nil, error: &error)
        if error != nil {
            return nil
        } else {
            return MurphyLevel.fromData(data)
        }
    }
    
    static func fromData(data: NSData) -> MurphyLevel? {
        let scanner = BinaryDataScanner(data: data, littleEndian: false, encoding: NSASCIIStringEncoding)
        
        let name = scanner.readNullTerminatedString()
        let graphicsSetName = scanner.readNullTerminatedString()
        let infotrons = scanner.read16()
        let width = scanner.read16()
        let height = scanner.read16()

        var level:MurphyLevel? = nil
        
        if (name != nil) && (graphicsSetName != nil) && (infotrons != nil) && (width != nil) && (height != nil) {
            let expectedRemaining = Int(width!) * Int(height!) * 2
            
            if scanner.remaining == expectedRemaining {
                var grid = Array<LevelTile>()
                
                for y in 0..<Int(height!) {
                    for x in 0..<Int(width!) {
                        // smooth over stupid thing in old .mbl files
                        let tileMapX = Int(scanner.readByte()!)
                        let tileMapY = Int(scanner.readByte()!)
                        let tileMapRaw = (tileMapY * TILESET_WIDTH) + tileMapX
                        let tile = LevelTile.fromRaw(tileMapRaw)  // XXX I suspect fromRaw() is slow
                        if tile != nil {
                            grid.append(tile!)
                        } else {
                            // XXX maybe labeled break? I hate this kind of control flow
                            return nil
                        }
                    }
                }
                
                level = MurphyLevel(name: name!, graphicsSetName: graphicsSetName!, infotrons: Int(infotrons!), width: Int(width!), height: Int(height!), grid: grid)
            }
        }
        
        return level
    }
}
