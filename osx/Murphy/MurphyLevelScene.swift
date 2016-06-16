//
//  MurphyLevelScene.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation
import SpriteKit

// FIXME XXX all sorts of temporary hacks about sizing because I'm lazy
let HALF_TILE_SIZE: CGFloat = 16
let TILE_SIZE: CGFloat = 32
let SPEED: CGFloat = 8
let SCENE_WIDTH: CGFloat = 640
let SCENE_HEIGHT: CGFloat = 480
let STATUS_WIDTH: CGFloat = 40


protocol MurphyLevelSceneDelegate {
    func levelSceneDidEnd(_ levelScene: MurphyLevelScene)
}

class MurphyLevelScene: SKScene {
    var atlas: SKTextureAtlas!
    var font: SKTexture!
    var level: MurphyLevel!
    var levelSize: CGSize!
    var board: SKNode!
    var status: SKSpriteNode!
    
    var levelDelegate: MurphyLevelSceneDelegate?
    
    class func sceneWithLevel(_ level: MurphyLevel) -> MurphyLevelScene {
        let scene = MurphyLevelScene(size:CGSize(width: SCENE_WIDTH, height: SCENE_HEIGHT))
        scene.level = level
        scene.levelSize = CGSize(width: CGFloat(level.width) * TILE_SIZE, height: CGFloat(level.height) * TILE_SIZE)
        scene.atlas = SKTextureAtlas(named: level.graphicsSetName)
        scene.font = SKTexture(imageNamed: "font-small")
        return scene
    }

    override func didMove(to view: SKView) {
        buildAnimations()
        buildBoard()
        buildStatus()
    }

    var infotronAnimation: SKAction!
    var quarkAnimation: SKAction!
    var terminalAnimation: SKAction!
    var scissorAnimation: SKAction!
    var oliverAnimation: SKAction!

    func buildAnimations() {
        // XXX any real code would put this stuff somewhere else
        let infotronTextures = LevelTile.infotrons().map { self.atlas.textureNamed($0.textureName()) }
        infotronAnimation = SKAction.animate(with: infotronTextures, timePerFrame: 0.075)
        
        let quarkTextures = LevelTile.quarks().map { self.atlas.textureNamed($0.textureName()) }
        quarkAnimation = SKAction.animate(with: quarkTextures, timePerFrame: 0.175)
        
        let terminalTextures = LevelTile.terminals().map { self.atlas.textureNamed($0.textureName()) }
        terminalAnimation = SKAction.animate(with: terminalTextures, timePerFrame: 0.175)
        
        let scissorTextures = LevelTile.scissors().map { self.atlas.textureNamed($0.textureName()) }
        scissorAnimation = SKAction.animate(with: scissorTextures, timePerFrame: 0.175)
        
        let oliverTextures = OliverTile.bubs().map { self.atlas.textureNamed($0.textureName()) }
        oliverAnimation = SKAction.animate(with: oliverTextures, timePerFrame: 0.175)
    }
    
    func buildStatus() {
        status = SKSpriteNode(imageNamed: "status-bar-default")
        status.anchorPoint = CGPoint.zero
        self.addChild(status)
        status.position = CGPoint(x: SCENE_WIDTH - STATUS_WIDTH, y: 0)
        
        buildStatusLevelName()
    }
    
    func buildStatusLevelName() {
        // TODO draw the level name using the font
        // XXX probably should be broken into another class
    }
    
    func buildBoard() {
        board = SKNode()
        board.position = CGPoint(x: 0, y: SCENE_HEIGHT)  // I hate GL coordinate systems
        addChild(board)

        for x in 0..<level.width {
            for y in 0..<level.height {
                let tile = level.tileAt(x, y: y)!
                let textureName = tile.textureName()
                let texture = atlas.textureNamed(textureName)
                let node = SKSpriteNode(texture: texture)
                node.position = CGPoint(x: CGFloat(x) * TILE_SIZE, y: CGFloat(-y) * TILE_SIZE)
                board.addChild(node)
                
                // build animations for a few cases
                switch (tile) {
                case .infotronTile:
                    node.run(SKAction.repeatForever(infotronAnimation))
                    
                case .quarkTile:
                    node.run(SKAction.repeatForever(quarkAnimation))
                    
                case .terminalTile:
                    node.run(SKAction.repeatForever(terminalAnimation))

                case .upScissorTile:
                    fallthrough
                case .leftScissorTile:
                    fallthrough
                case .rightScissorTile:
                    fallthrough
                case .downScissorTile:
                    node.run(SKAction.repeatForever(scissorAnimation))
                    
                case .oliverTile:
                    node.run(SKAction.repeatForever(oliverAnimation))
                                        
                default:
                    break
                }
            }
        }
    }
    
    override func keyDown(_ theEvent: NSEvent) {
        handleKeyEvent(theEvent, pressed: true)
    }
    
    override func keyUp(_ theEvent: NSEvent) {
        handleKeyEvent(theEvent, pressed: false)
    }

    override func mouseUp(_ theEvent: NSEvent) {
        levelDelegate?.levelSceneDidEnd(self)
    }

    var motionX: CGFloat = 0
    var motionY: CGFloat = 0
    
    func handleKeyEvent(_ event: NSEvent, pressed: Bool) {
        // XXX where oh where are the key constants I seek?
        if event.modifierFlags.contains(NSEventModifierFlags.numericPad) {
            if let scalars = event.charactersIgnoringModifiers?.unicodeScalars {
                for keyChar in scalars {
                    switch UInt32(keyChar) {
                    case 0xF700: // up
                        motionY = pressed ? -SPEED : 0
                        
                    case 0xF701: // down
                        motionY = pressed ? SPEED : 0
                        
                    case 0xF702: // left
                        motionX = pressed ? SPEED : 0
                        
                    case 0xF703: // right
                        motionX = pressed ? -SPEED : 0
                        
                    default:
                        break
                    }
                }
            }
        }
    }
    
    override func update(_ currentTime: TimeInterval) {
        var rawPosition = CGPoint(x: board.position.x + motionX, y: board.position.y + motionY)
        
        // stoopid bounds code
        if rawPosition.x >= 0 {
            rawPosition.x = 0
        } else if rawPosition.x <= -(self.levelSize.width - SCENE_WIDTH + HALF_TILE_SIZE) {
            rawPosition.x = -(self.levelSize.width - SCENE_WIDTH + HALF_TILE_SIZE)
        }
        
        if rawPosition.y <= SCENE_HEIGHT {
            rawPosition.y = SCENE_HEIGHT
        } else if rawPosition.y >= (self.levelSize.height - HALF_TILE_SIZE) {
            rawPosition.y = self.levelSize.height - HALF_TILE_SIZE
        }
        
        board.position = rawPosition
    }
    
}
