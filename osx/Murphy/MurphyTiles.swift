//
//  MurphyTiles.swift
//  Murphy
//
//  Created by Dave Peck on 7/20/14.
//  Copyright (c) 2014 Dave Peck. All rights reserved.
//

import Foundation

// XXX all hacked up from ancient MR C code -- 
// who knows what the right thing might be.


let LevelTileTextureNames = [
    "CircuitTile.png",
    "BlankTile.png",
    "ExitTile.png",
    "GreenDiskTile.png",
    "YellowDiskTile.png",
    "RedDiskTile.png",
    "GoldDiskTile.png",
    "TerminalTile.png",
    "BugTile.png",
    "OliverTile.png",
    "LeftPortTile.png",
    "DownPortTile.png",
    "RightPortTile.png",
    "UpPortTile.png",
    "LeftRightPortTile.png",
    "UpDownPortTile.png",
    "FourWayPortTile.png",
    "ZonkTile.png",
    "UpZonkTile.png",
    "InfotronTile.png",
    "QuarkTile.png",
    "UpScissorTile.png",
    "RightScissorTile.png",
    "DownScissorTile.png",
    "LeftScissorTile.png",
    "ChipTile.png",
    "TopChipTile.png",
    "BottomChipTile.png",
    "LeftChipTile.png",
    "RightChipTile.png",
    "TopLeftBorderTile.png", //30
    "TopBorderTile.png",
    "TopRightBorderTile.png",
    "RightBorderTile.png",
    "LeftBorderTile.png",
    "BottomLeftBorderTile.png",
    "BottomBorderTile.png",
    "BottomRightBorderTile.png",
    "BlankChipTile.png",
    "FirstHardwareTile.png",
    "HardwareTile2.png", // 40
    "HardwareTile3.png",
    "HardwareTile4.png",
    "HardwareTile5.png",
    "HardwareTile6.png",
    "HardwareTile7.png",
    "HardwareTile8.png",
    "HardwareTile9.png",
    "HardwareTile10.png",
    "HardwareTile11.png",
    "HardwareTile12.png", // 50
    "HardwareTile13.png",
    "HardwareTile14.png",
    "HardwareTile15.png",
    "HardwareTile16.png",
    "HardwareTile17.png",
    "HardwareTile18.png",
    "HardwareTile19.png",
    "HardwareTile20.png",
    "HardwareTile21.png",
    "HardwareTile22.png", // 60
    "HardwareTile23.png",
    "HardwareTile24.png",
    "HardwareTile25.png",
    "HardwareTile26.png",
    "LastHardwareTile.png",
    "PhantomTile.png",
    "LaserTile.png",
    "LaserPowerTile.png",
    "ForeMirrorTile.png",
    "BackMirrorTile.png", // 70
    "EmptyPhaseTile.png",
    "FullPhaseTile.png",
    "PhaseTermTile.png",
    "EmptyTile4.png",
    "EmptyTile5.png",
    "EmptyTile6.png",
    "EmptyTile7.png",
    "EmptyTile8.png",
    "EmptyTile9.png",
    "BlankLaserPiece.png", // 80
    "LaserEmptyPiece.png",
    "LaserPowerPiece.png",
    "LaserPiece.png",
    "ForeMirrorPiece.png",
    "BackMirrorPiece.png",
    "TLBRLaserForeMirrorLightPiece.png",
    "RLLaserLightPiece.png",
    "UDLaserLightPiece.png",
    "TLLaserForeMirrorLightPiece.png",
    "BRLaserForeMirrorLightPiece.png", // 90
    "BLLaserBackMirrorLightPiece.png",
    "TRLaserBackMirrorLightPiece.png",
    "BLTRLaserBackMirrorLightPiece.png",
    "FirstInfotronTile.png",
    "InfoTile2.png",
    "InfoTile3.png",
    "InfoTile4.png",
    "InfoTile5.png",
    "InfoTile6.png",
    "InfoTile7.png", // 100
    "InfoTile8.png",
    "InfoTile9.png",
    "InfoTile10.png",
    "InfoTile11.png",
    "InfoTile12.png",
    "InfoTile13.png",
    "InfoTile14.png",
    "InfoTile15.png",
    "LastInfotronTile.png",
    "FirstTerminalTile.png", // 110
    "TermTile2.png",
    "TermTile3.png",
    "TermTile4.png",
    "TermTile5.png",
    "TermTile6.png",
    "TermTile7.png",
    "TermTile8.png",
    "TermTile9.png",
    "TermTile10.png",
    "TermTile11.png", // 120
    "LastTerminalTile.png",
    "FirstBugTile.png",
    "BugTile2.png",
    "BugTile3.png",
    "BugTile4.png",
    "BugTile5.png",
    "LastBugTile.png",
    "FirstExplTile.png",
    "ExplTile2.png",
    "ExplTile3.png",
    "IgniteExplTile.png", // the one where chain reactions start...
    "ExplTile5.png",
    "ExplTile6.png",
    "ExplTile7.png",
    "ExplTile8.png",
    "ExplTile9.png",
    "LastExplTile.png",
    "FirstExitTile.png",
    "ExitTile2.png",
    "ExitTile3.png",
    "ExitTile4.png",
    "ExitTile5.png",
    "ExitTile6.png",
    "ExitTile7.png",
    "LastExitTile.png",
    "FirstZapRedDiskTile.png",
    "ZapRedDiskTile1.png",
    "ZapRedDiskTile2.png",
    "ZapRedDiskTile3.png",
    "ZapRedDiskTile4.png",
    "ZapRedDiskTile5.png",
    "ZapRedDiskTile6.png",
    "LastZapRedDiskTile.png",
    "EmptyZapTile8.png",
    "EmptyZapTile9.png",
    "FirstZapCircuitTile.png",
    "ZapCircuitTile11.png",
    "ZapCircuitTile12.png",
    "ZapCircuitTile13.png",
    "ZapCircuitTile14.png",
    "ZapCircuitTile15.png",
    "LastZapCircuitTile.png",
    "EmptyZapTile17.png",
    "EmptyZapTile18.png",
    "EmptyZapTile19.png",
    "FirstZapInfotronTile.png",
    "ZapInfotronTile21.png",
    "ZapInfotronTile22.png",
    "ZapInfotronTile23.png",
    "ZapInfotronTile24.png", // 170
    "ZapInfotronTile25.png",
    "ZapInfotronTile26.png",
    "ZapInfotronTile27.png",
    "ZapInfotronTile28.png",
    "ZapInfotronTile29.png",
    "ZapInfotronTile30.png",
    "ZapInfotronTile31.png",
    "LastZapInfotronTile.png",
    "FirstRDBlinkTile.png",
    "LastRDBlinkTile.png",
    "FirstZonkTile.png",
    "ZonkTile1.png",
    "ZonkTile2.png",
    "LastZonkTile.png",
    "FirstUpZonkTile.png",
    "UpZonkTile1.png",
    "UpZonkTile2.png",
    "LastUpZonkTile.png",
    "FirstQuarkTile.png",
    "QuarkTile1.png",
    "QuarkTile2.png",
    "QuarkTile3.png",
    "QuarkTile4.png",
    "QuarkTile5.png",
    "QuarkTile6.png",
    "QuarkTile7.png",
    "LastQuarkTile.png",
    "FirstUpScissorTile.png",
    "UpScissorTile1.png",
    "LastUpScissorTile.png",
    "UpRightScissorTile.png",
    "FirstRightScissorTile.png",
    "RightScissorTile1.png",
    "LastRightScissorTile.png",
    "DownRightScissorTile.png",
    "FirstDownScissorTile.png",
    "DownScissorTile1.png",
    "LastDownScissorTile.png",
    "DownLeftScissorTile.png",
    "FirstLeftScissorTile.png",
    "LeftScissorTile1.png",
    "LastLeftScissorTile.png",
    "UpLeftScissorTile.png",
]


enum LevelTile : Int {
    case circuitTile = 0
    case blankTile
    case exitTile
    case greenDiskTile
    case yellowDiskTile
    case redDiskTile
    case goldDiskTile
    case terminalTile
    case bugTile
    case oliverTile
    case leftPortTile
    case downPortTile
    case rightPortTile
    case upPortTile
    case leftRightPortTile
    case upDownPortTile
    case fourWayPortTile
    case zonkTile
    case upZonkTile
    case infotronTile
    case quarkTile
    case upScissorTile
    case rightScissorTile
    case downScissorTile
    case leftScissorTile
    case chipTile
    case topChipTile
    case bottomChipTile
    case leftChipTile
    case rightChipTile
    case topLeftBorderTile //30
    case topBorderTile
    case topRightBorderTile
    case rightBorderTile
    case leftBorderTile
    case bottomLeftBorderTile
    case bottomBorderTile
    case bottomRightBorderTile
    case blankChipTile
    case firstHardwareTile
    case hardwareTile2 // 40
    case hardwareTile3
    case hardwareTile4
    case hardwareTile5
    case hardwareTile6
    case hardwareTile7
    case hardwareTile8
    case hardwareTile9
    case hardwareTile10
    case hardwareTile11
    case hardwareTile12 // 50
    case hardwareTile13
    case hardwareTile14
    case hardwareTile15
    case hardwareTile16
    case hardwareTile17
    case hardwareTile18
    case hardwareTile19
    case hardwareTile20
    case hardwareTile21
    case hardwareTile22 // 60
    case hardwareTile23
    case hardwareTile24
    case hardwareTile25
    case hardwareTile26
    case lastHardwareTile
    case phantomTile
    case laserTile
    case laserPowerTile
    case foreMirrorTile
    case backMirrorTile // 70
    case emptyPhaseTile
    case fullPhaseTile
    case phaseTermTile
    case emptyTile4
    case emptyTile5
    case emptyTile6
    case emptyTile7
    case emptyTile8
    case emptyTile9
    case blankLaserPiece // 80
    case laserEmptyPiece
    case laserPowerPiece
    case laserPiece
    case foreMirrorPiece
    case backMirrorPiece
    case tlbrLaserForeMirrorLightPiece
    case rlLaserLightPiece
    case udLaserLightPiece
    case tlLaserForeMirrorLightPiece
    case brLaserForeMirrorLightPiece // 90
    case blLaserBackMirrorLightPiece
    case trLaserBackMirrorLightPiece
    case bltrLaserBackMirrorLightPiece
    case firstInfotronTile
    case infoTile2
    case infoTile3
    case infoTile4
    case infoTile5
    case infoTile6
    case infoTile7 // 100
    case infoTile8
    case infoTile9
    case infoTile10
    case infoTile11
    case infoTile12
    case infoTile13
    case infoTile14
    case infoTile15
    case lastInfotronTile
    case firstTerminalTile // 110
    case termTile2
    case termTile3
    case termTile4
    case termTile5
    case termTile6
    case termTile7
    case termTile8
    case termTile9
    case termTile10
    case termTile11 // 120
    case lastTerminalTile
    case firstBugTile
    case bugTile2
    case bugTile3
    case bugTile4
    case bugTile5
    case lastBugTile
    case firstExplTile
    case explTile2
    case explTile3
    case igniteExplTile // the one where chain reactions start...
    case explTile5
    case explTile6
    case explTile7
    case explTile8
    case explTile9
    case lastExplTile
    case firstExitTile
    case exitTile2
    case exitTile3
    case exitTile4
    case exitTile5
    case exitTile6
    case exitTile7
    case lastExitTile
    case firstZapRedDiskTile
    case zapRedDiskTile1
    case zapRedDiskTile2
    case zapRedDiskTile3
    case zapRedDiskTile4
    case zapRedDiskTile5
    case zapRedDiskTile6
    case lastZapRedDiskTile
    case emptyZapTile8
    case emptyZapTile9
    case firstZapCircuitTile
    case zapCircuitTile11
    case zapCircuitTile12
    case zapCircuitTile13
    case zapCircuitTile14
    case zapCircuitTile15
    case lastZapCircuitTile
    case emptyZapTile17
    case emptyZapTile18
    case emptyZapTile19
    case firstZapInfotronTile
    case zapInfotronTile21
    case zapInfotronTile22
    case zapInfotronTile23
    case zapInfotronTile24 // 170
    case zapInfotronTile25
    case zapInfotronTile26
    case zapInfotronTile27
    case zapInfotronTile28
    case zapInfotronTile29
    case zapInfotronTile30
    case zapInfotronTile31
    case lastZapInfotronTile
    case firstRDBlinkTile
    case lastRDBlinkTile
    case firstZonkTile
    case zonkTile1
    case zonkTile2
    case lastZonkTile
    case firstUpZonkTile
    case upZonkTile1
    case upZonkTile2
    case lastUpZonkTile
    case firstQuarkTile
    case quarkTile1
    case quarkTile2
    case quarkTile3
    case quarkTile4
    case quarkTile5
    case quarkTile6
    case quarkTile7
    case lastQuarkTile
    case firstUpScissorTile
    case upScissorTile1
    case lastUpScissorTile
    case upRightScissorTile
    case firstRightScissorTile
    case rightScissorTile1
    case lastRightScissorTile
    case downRightScissorTile
    case firstDownScissorTile
    case downScissorTile1
    case lastDownScissorTile
    case downLeftScissorTile
    case firstLeftScissorTile
    case leftScissorTile1
    case lastLeftScissorTile
    case upLeftScissorTile
    
    func textureName() -> String {
        return LevelTileTextureNames[self.rawValue]
    }
    
    static func infotrons() -> AnySequence<LevelTile> {
        return (.firstInfotronTile)...(.lastInfotronTile)
    }
    
    static func quarks() -> AnySequence<LevelTile> {
        return (.firstQuarkTile)...(.lastQuarkTile)
    }
    
    static func terminals() -> AnySequence<LevelTile> {
        return (.firstTerminalTile)...(.lastTerminalTile)
    }
    
    static func scissors() -> AnySequence<LevelTile> {
        return (.firstUpScissorTile)...(.upLeftScissorTile)
    }
}


let OliverTileTextureNames = [
    "BubFrame.png",
    "ABubFrame1.png",
    "ABubFrame2.png",
    "BBubFrame1.png",
    "BBubFrame2.png",
    "CBubFrame1.png",
    "CBubFrame2.png",
    "DBubFrame1.png",
    "DBubFrame2.png",
    "LeftBubFirstFrame.png",
    "LeftBubFrame2.png",
    "LeftBubLastFrame.png",
    "RightBubFirstFrame.png",
    "RightBubFrame2.png",
    "RightBubLastFrame.png",
    "EmptyBubFrame1.png",
    "PushLeftBubFirstFrame.png",
    "PushLeftBubLastFrame.png",
    "PushRightBubFirstFrame.png",
    "PushRightBubLastFrame.png",
    "PushDownBubFirstFrame.png",
    "PushDownBubLastFrame.png",
    "PushUpBubFirstFrame.png",
    "PushUpBubLastFrame.png",
    "ZapLeftBubFrame.png",
    "ZapRightBubFrame.png",
    "ZapDownBubFrame.png",
    "ZapUpBubFrame.png",
    "EmptyBubFrame2.png",
    "EmptyBubFrame3.png",
    "FadeDeathBubFirstFrame.png",
    "FadeDeathBubFrame2.png",
    "FadeDeathBubFrame3.png",
    "FadeDeathBubFrame4.png",
    "FadeDeathBubFrame5.png",
    "FadeDeathBubFrame6.png",
    "FadeDeathBubFrame7.png",
    "FadeDeathBubFrame8.png",
    "FadeDeathBubFrame9.png",
    "FadeDeathBubLastFrame.png",
    "BubbleDeathBubFirstFrame.png",
    "BubbleDeathBubFrame2.png",
    "BubbleDeathBubFrame3.png",
    "BubbleDeathBubFrame4.png",
    "BubbleDeathBubFrame5.png",
    "BubbleDeathBubFrame6.png",
    "BubbleDeathBubFrame7.png",
    "BubbleDeathBubFrame8.png",
    "BubbleDeathBubFrame9.png",
    "BubbleDeathBubLastFrame.png",
    "StaticDeathBubFirstFrame.png",
    "StaticDeathBubFrame2.png",
    "StaticDeathBubFrame3.png",
    "StaticDeathBubFrame4.png",
    "StaticDeathBubFrame5.png",
    "StaticDeathBubFrame6.png",
    "StaticDeathBubFrame7.png",
    "StaticDeathBubFrame8.png",
    "StaticDeathBubFrame9.png",
    "StaticDeathBubLastFrame.png",
    "YawnBubFirstFrame.png",
    "YawnBubFrame2.png",
    "YawnBubFrame3.png",
    "YawnBubFrame4.png",
    "YawnBubFrame5.png",
    "YawnBubFrame6.png",
    "YawnBubFrame7.png",
    "YawnBubFrame8.png",
    "YawnBubFrame9.png",
    "YawnBubFrame10.png",
    "YawnBubFrame11.png",
    "YawnBubLastFrame.png",
    "ShockDeathBubFirstFrame.png",
    "ShockDeathBubFrame2.png",
    "ShockDeathBubFrame3.png",
    "ShockDeathBubFrame4.png",
    "ShockDeathBubFrame5.png",
    "ShockDeathBubFrame6.png",
    "ShockDeathBubLastFrame.png",
    "EmptyBubFrame4.png",
    "EurekaBubFrame.png",
    "SadBubFrame.png",
    "FuckYouBubFrame.png",
    "ExLaxBubFirstFrame.png",
    "ExLaxBubLastFrame.png",
]


enum OliverTile : Int {
    case bubFrame = 220
    case aBubFrame1
    case aBubFrame2
    case bBubFrame1
    case bBubFrame2
    case cBubFrame1
    case cBubFrame2
    case dBubFrame1
    case dBubFrame2
    case leftBubFirstFrame
    case leftBubFrame2
    case leftBubLastFrame
    case rightBubFirstFrame
    case rightBubFrame2
    case rightBubLastFrame
    case emptyBubFrame1
    case pushLeftBubFirstFrame
    case pushLeftBubLastFrame
    case pushRightBubFirstFrame
    case pushRightBubLastFrame
    case pushDownBubFirstFrame
    case pushDownBubLastFrame
    case pushUpBubFirstFrame
    case pushUpBubLastFrame
    case zapLeftBubFrame
    case zapRightBubFrame
    case zapDownBubFrame
    case zapUpBubFrame
    case emptyBubFrame2
    case emptyBubFrame3
    case fadeDeathBubFirstFrame
    case fadeDeathBubFrame2
    case fadeDeathBubFrame3
    case fadeDeathBubFrame4
    case fadeDeathBubFrame5
    case fadeDeathBubFrame6
    case fadeDeathBubFrame7
    case fadeDeathBubFrame8
    case fadeDeathBubFrame9
    case fadeDeathBubLastFrame
    case bubbleDeathBubFirstFrame
    case bubbleDeathBubFrame2
    case bubbleDeathBubFrame3
    case bubbleDeathBubFrame4
    case bubbleDeathBubFrame5
    case bubbleDeathBubFrame6
    case bubbleDeathBubFrame7
    case bubbleDeathBubFrame8
    case bubbleDeathBubFrame9
    case bubbleDeathBubLastFrame
    case staticDeathBubFirstFrame
    case staticDeathBubFrame2
    case staticDeathBubFrame3
    case staticDeathBubFrame4
    case staticDeathBubFrame5
    case staticDeathBubFrame6
    case staticDeathBubFrame7
    case staticDeathBubFrame8
    case staticDeathBubFrame9
    case staticDeathBubLastFrame
    case yawnBubFirstFrame
    case yawnBubFrame2
    case yawnBubFrame3
    case yawnBubFrame4
    case yawnBubFrame5
    case yawnBubFrame6
    case yawnBubFrame7
    case yawnBubFrame8
    case yawnBubFrame9
    case yawnBubFrame10
    case yawnBubFrame11
    case yawnBubLastFrame
    case shockDeathBubFirstFrame
    case shockDeathBubFrame2
    case shockDeathBubFrame3
    case shockDeathBubFrame4
    case shockDeathBubFrame5
    case shockDeathBubFrame6
    case shockDeathBubLastFrame
    case emptyBubFrame4
    case eurekaBubFrame
    case sadBubFrame
    case fuckYouBubFrame
    case exLaxBubFirstFrame
    case exLaxBubLastFrame
    
    func textureName() -> String {
        return OliverTileTextureNames[self.rawValue - 220]
    }
    
    static func bubs() -> AnySequence<OliverTile> {
        return (.bubFrame)...(.dBubFrame2)
    }
}

