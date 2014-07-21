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


enum LevelTile : Int, IntRawRepresentable {
    case CircuitTile = 0
    case BlankTile
    case ExitTile
    case GreenDiskTile
    case YellowDiskTile
    case RedDiskTile
    case GoldDiskTile
    case TerminalTile
    case BugTile
    case OliverTile
    case LeftPortTile
    case DownPortTile
    case RightPortTile
    case UpPortTile
    case LeftRightPortTile
    case UpDownPortTile
    case FourWayPortTile
    case ZonkTile
    case UpZonkTile
    case InfotronTile
    case QuarkTile
    case UpScissorTile
    case RightScissorTile
    case DownScissorTile
    case LeftScissorTile
    case ChipTile
    case TopChipTile
    case BottomChipTile
    case LeftChipTile
    case RightChipTile
    case TopLeftBorderTile //30
    case TopBorderTile
    case TopRightBorderTile
    case RightBorderTile
    case LeftBorderTile
    case BottomLeftBorderTile
    case BottomBorderTile
    case BottomRightBorderTile
    case BlankChipTile
    case FirstHardwareTile
    case HardwareTile2 // 40
    case HardwareTile3
    case HardwareTile4
    case HardwareTile5
    case HardwareTile6
    case HardwareTile7
    case HardwareTile8
    case HardwareTile9
    case HardwareTile10
    case HardwareTile11
    case HardwareTile12 // 50
    case HardwareTile13
    case HardwareTile14
    case HardwareTile15
    case HardwareTile16
    case HardwareTile17
    case HardwareTile18
    case HardwareTile19
    case HardwareTile20
    case HardwareTile21
    case HardwareTile22 // 60
    case HardwareTile23
    case HardwareTile24
    case HardwareTile25
    case HardwareTile26
    case LastHardwareTile
    case PhantomTile
    case LaserTile
    case LaserPowerTile
    case ForeMirrorTile
    case BackMirrorTile // 70
    case EmptyPhaseTile
    case FullPhaseTile
    case PhaseTermTile
    case EmptyTile4
    case EmptyTile5
    case EmptyTile6
    case EmptyTile7
    case EmptyTile8
    case EmptyTile9
    case BlankLaserPiece // 80
    case LaserEmptyPiece
    case LaserPowerPiece
    case LaserPiece
    case ForeMirrorPiece
    case BackMirrorPiece
    case TLBRLaserForeMirrorLightPiece
    case RLLaserLightPiece
    case UDLaserLightPiece
    case TLLaserForeMirrorLightPiece
    case BRLaserForeMirrorLightPiece // 90
    case BLLaserBackMirrorLightPiece
    case TRLaserBackMirrorLightPiece
    case BLTRLaserBackMirrorLightPiece
    case FirstInfotronTile
    case InfoTile2
    case InfoTile3
    case InfoTile4
    case InfoTile5
    case InfoTile6
    case InfoTile7 // 100
    case InfoTile8
    case InfoTile9
    case InfoTile10
    case InfoTile11
    case InfoTile12
    case InfoTile13
    case InfoTile14
    case InfoTile15
    case LastInfotronTile
    case FirstTerminalTile // 110
    case TermTile2
    case TermTile3
    case TermTile4
    case TermTile5
    case TermTile6
    case TermTile7
    case TermTile8
    case TermTile9
    case TermTile10
    case TermTile11 // 120
    case LastTerminalTile
    case FirstBugTile
    case BugTile2
    case BugTile3
    case BugTile4
    case BugTile5
    case LastBugTile
    case FirstExplTile
    case ExplTile2
    case ExplTile3
    case IgniteExplTile // the one where chain reactions start...
    case ExplTile5
    case ExplTile6
    case ExplTile7
    case ExplTile8
    case ExplTile9
    case LastExplTile
    case FirstExitTile
    case ExitTile2
    case ExitTile3
    case ExitTile4
    case ExitTile5
    case ExitTile6
    case ExitTile7
    case LastExitTile
    case FirstZapRedDiskTile
    case ZapRedDiskTile1
    case ZapRedDiskTile2
    case ZapRedDiskTile3
    case ZapRedDiskTile4
    case ZapRedDiskTile5
    case ZapRedDiskTile6
    case LastZapRedDiskTile
    case EmptyZapTile8
    case EmptyZapTile9
    case FirstZapCircuitTile
    case ZapCircuitTile11
    case ZapCircuitTile12
    case ZapCircuitTile13
    case ZapCircuitTile14
    case ZapCircuitTile15
    case LastZapCircuitTile
    case EmptyZapTile17
    case EmptyZapTile18
    case EmptyZapTile19
    case FirstZapInfotronTile
    case ZapInfotronTile21
    case ZapInfotronTile22
    case ZapInfotronTile23
    case ZapInfotronTile24 // 170
    case ZapInfotronTile25
    case ZapInfotronTile26
    case ZapInfotronTile27
    case ZapInfotronTile28
    case ZapInfotronTile29
    case ZapInfotronTile30
    case ZapInfotronTile31
    case LastZapInfotronTile
    case FirstRDBlinkTile
    case LastRDBlinkTile
    case FirstZonkTile
    case ZonkTile1
    case ZonkTile2
    case LastZonkTile
    case FirstUpZonkTile
    case UpZonkTile1
    case UpZonkTile2
    case LastUpZonkTile
    case FirstQuarkTile
    case QuarkTile1
    case QuarkTile2
    case QuarkTile3
    case QuarkTile4
    case QuarkTile5
    case QuarkTile6
    case QuarkTile7
    case LastQuarkTile
    case FirstUpScissorTile
    case UpScissorTile1
    case LastUpScissorTile
    case UpRightScissorTile
    case FirstRightScissorTile
    case RightScissorTile1
    case LastRightScissorTile
    case DownRightScissorTile
    case FirstDownScissorTile
    case DownScissorTile1
    case LastDownScissorTile
    case DownLeftScissorTile
    case FirstLeftScissorTile
    case LeftScissorTile1
    case LastLeftScissorTile
    case UpLeftScissorTile
    
    func textureName() -> String {
        return LevelTileTextureNames[self.toRaw()]  // XXX I suspect toRaw() is slow
    }
    
    static func infotrons() -> SequenceOf<LevelTile> {
        return enumerate(.FirstInfotronTile, .LastInfotronTile)
    }
    
    static func quarks() -> SequenceOf<LevelTile> {
        return enumerate(.FirstQuarkTile, .LastQuarkTile)
    }
    
    static func terminals() -> SequenceOf<LevelTile> {
        return enumerate(.FirstTerminalTile, .LastTerminalTile)
    }
    
    static func scissors() -> SequenceOf<LevelTile> {
        return enumerate(.FirstUpScissorTile, .UpLeftScissorTile)
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


enum OliverTile : Int, IntRawRepresentable {
    case BubFrame = 220
    case ABubFrame1
    case ABubFrame2
    case BBubFrame1
    case BBubFrame2
    case CBubFrame1
    case CBubFrame2
    case DBubFrame1
    case DBubFrame2
    case LeftBubFirstFrame
    case LeftBubFrame2
    case LeftBubLastFrame
    case RightBubFirstFrame
    case RightBubFrame2
    case RightBubLastFrame
    case EmptyBubFrame1
    case PushLeftBubFirstFrame
    case PushLeftBubLastFrame
    case PushRightBubFirstFrame
    case PushRightBubLastFrame
    case PushDownBubFirstFrame
    case PushDownBubLastFrame
    case PushUpBubFirstFrame
    case PushUpBubLastFrame
    case ZapLeftBubFrame
    case ZapRightBubFrame
    case ZapDownBubFrame
    case ZapUpBubFrame
    case EmptyBubFrame2
    case EmptyBubFrame3
    case FadeDeathBubFirstFrame
    case FadeDeathBubFrame2
    case FadeDeathBubFrame3
    case FadeDeathBubFrame4
    case FadeDeathBubFrame5
    case FadeDeathBubFrame6
    case FadeDeathBubFrame7
    case FadeDeathBubFrame8
    case FadeDeathBubFrame9
    case FadeDeathBubLastFrame
    case BubbleDeathBubFirstFrame
    case BubbleDeathBubFrame2
    case BubbleDeathBubFrame3
    case BubbleDeathBubFrame4
    case BubbleDeathBubFrame5
    case BubbleDeathBubFrame6
    case BubbleDeathBubFrame7
    case BubbleDeathBubFrame8
    case BubbleDeathBubFrame9
    case BubbleDeathBubLastFrame
    case StaticDeathBubFirstFrame
    case StaticDeathBubFrame2
    case StaticDeathBubFrame3
    case StaticDeathBubFrame4
    case StaticDeathBubFrame5
    case StaticDeathBubFrame6
    case StaticDeathBubFrame7
    case StaticDeathBubFrame8
    case StaticDeathBubFrame9
    case StaticDeathBubLastFrame
    case YawnBubFirstFrame
    case YawnBubFrame2
    case YawnBubFrame3
    case YawnBubFrame4
    case YawnBubFrame5
    case YawnBubFrame6
    case YawnBubFrame7
    case YawnBubFrame8
    case YawnBubFrame9
    case YawnBubFrame10
    case YawnBubFrame11
    case YawnBubLastFrame
    case ShockDeathBubFirstFrame
    case ShockDeathBubFrame2
    case ShockDeathBubFrame3
    case ShockDeathBubFrame4
    case ShockDeathBubFrame5
    case ShockDeathBubFrame6
    case ShockDeathBubLastFrame
    case EmptyBubFrame4
    case EurekaBubFrame
    case SadBubFrame
    case FuckYouBubFrame
    case ExLaxBubFirstFrame
    case ExLaxBubLastFrame
    
    func textureName() -> String {
        return OliverTileTextureNames[self.toRaw() - 220]
    }
    
    static func bubs() -> SequenceOf<OliverTile> {
        return enumerate(.BubFrame, .DBubFrame2)
    }
}

