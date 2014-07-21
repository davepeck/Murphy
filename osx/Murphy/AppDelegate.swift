//
//  AppDelegate.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Cocoa
import SpriteKit

let LevelNames = [
    "Achtung!",
    "And So It Begins",
    "Bolder",
    "Catacombs-Aquatron",
    "Combinations",
    "Crystalline",
    "Dash Dash",
    "Daylight",
    "Dot Dot",
    "Excavation",
    "Falling",
    "Going Up",
    "Gold Rush",
    "Golden Rule",
    "Inflamatory",
    "It's Alive!!",
    "Labyrinth",
    "Love Boat",
    "No More Secrets",
    "Out In Out",
    "Robots & Plasmoids",
    "Short Circuit",
    "Stress",
    "Thriller",
    "Trapped Inside",
    "Wizard",
]


class AppDelegate: NSObject, NSApplicationDelegate, MurphyLevelSceneDelegate {
    @IBOutlet var window: NSWindow
    @IBOutlet var skView: SKView
    var currentLevelIndex: Int = 0
    var currentScene: MurphyLevelScene?

    func buildCurrentScene() {
        let level = MurphyLevel.fromResourceNamed(LevelNames[currentLevelIndex])
        
        if level {
            currentScene = MurphyLevelScene(level: level!)
            currentScene!.levelDelegate = self
            currentScene!.scaleMode = .AspectFit
        } else {
            currentScene = nil
        }
    }
    
    func presentCurrentScene() {
        if currentScene {
            self.skView!.presentScene(currentScene!)
            self.skView!.ignoresSiblingOrder = true
            self.skView!.showsFPS = true
            self.skView!.showsNodeCount = true
        }
    }
    
    func levelSceneDidEnd(levelScene: MurphyLevelScene) {
        currentScene!.levelDelegate = nil
        currentLevelIndex = (currentLevelIndex + 1) % LevelNames.count
        buildCurrentScene()
        presentCurrentScene()
    }
    
    func applicationDidFinishLaunching(aNotification: NSNotification?) {
        buildCurrentScene()
        presentCurrentScene()
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(sender: NSApplication) -> Bool {
        return true;
    }
    
}
