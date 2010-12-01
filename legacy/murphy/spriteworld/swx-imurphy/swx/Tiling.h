//---------------------------------------------------------------------------------------
/// @file Tiling.h
/// Constants, structures, and function prototypes for tiling SpriteWorlds.
//
//  By: Vern Jensen 10/27/95, Updated 7/05/03 for SWX
//---------------------------------------------------------------------------------------


#ifndef __TILING__
#define __TILING__

#ifndef __SPRITEWORLD__
#include <SpriteWorld.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------
//  Tiling definitions
//---------------------------------------------------------------------------------------

//#define   kMaskIsPartialMask              true
//#define kMaskIsNotPartialMask         false


typedef enum    // Used to SWCheckSpriteWithTiles
{
    kSWEntireSprite = 0,
    kSWTopSide,
    kSWRightSide,
    kSWBottomSide,
    kSWLeftSide
} SWTileSearchType;

#ifndef kSWNumTileLayers
    #define kSWNumTileLayers        10
#endif

typedef short** TileMapPtr;

    /// defines a map of tiles
struct TileMapStruct
{
    short       numRows;
    short       numCols;
    TileMapPtr  tileMap;
    //short     *tileMapDataP;
    //short     **arrayOfPointersH;
    SWBoolean   isLocked;
};

//---------------------------------------------------------------------------------------
//  tile function prototypes
//---------------------------------------------------------------------------------------

SWError SWInitTiling(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth,
    short           maxNumTiles);

void SWExitTiling(
    SpriteWorldPtr  spriteWorldP);

//SWError SWDisposeAllTilesFast(
//  SpriteWorldPtr  spriteWorldP);

SWError SWInitTilingCache(
    SpriteWorldPtr  spriteWorldP);

SWError SWCreateTileMap(
    TileMapStructPtr    *tileMapStructPP,
    short               numTileMapRows,
    short               numTileMapCols);

void SWDisposeTileMap(
    TileMapStructPtr    *tileMapStructPP);

void SWLockTileMap(
    TileMapStructPtr    tileMapStructP);

void SWUnlockTileMap(
    TileMapStructPtr    tileMapStructP);

void SWInstallTileMap(
    SpriteWorldPtr      spriteWorldP,
    TileMapStructPtr    tileMapStructP,
    short               tileLayer);

SWError SWLoadTileMap( const char * file,
    TileMapStructPtr    *tileMapStructPP );

SWError SWSaveTileMap( const char * file,
    TileMapStructPtr    tileMapStructP );

SWError SWResizeTileMap(
    TileMapStructPtr    tileMapStructP,
    short               numNewTileMapRows,
    short               numNewTileMapCols);

SWError SWLoadTilesFromSingleFileXY( 
    const char* fileName,
    int frameWidth,
    int frameHeight,
    int borderWidth,
    int borderHeight,
    SWBoolean hasOutsideBorder,
    int     numTiles,
        SpriteWorldPtr  spriteWorldP,
    short       startTileID );
        
SWError SWLoadTilesFromFileSequence(
        const char  *filename,
        int         start,
        int     numTiles,
        SpriteWorldPtr  spriteWorldP,
    short       startTileID );


SWError SWCreatePartialTile(
    SpriteWorldPtr  spriteWorldP,
    short           tileID );


void SWDisposeTile(
    SpriteWorldPtr  spriteWorldP,
    short           tileID);

void SWLockTiles(
    SpriteWorldPtr  spriteWorldP);

void SWUnlockTiles(
    SpriteWorldPtr  spriteWorldP);

SWError SWCreateExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *frameRect);

void SWDisposeExtraBackFrame(
    SpriteWorldPtr  spriteWorldP);

void SWSetTilingOn(
    SpriteWorldPtr  spriteWorldP,
    SWBoolean           tilingIsOn);

SWError SWChangeTileSize(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth);

void SWSetSpriteLayerUnderTileLayer(
    SpriteLayerPtr  spriteLayerP,
    short           tileLayer);
/*
SWError SWSetTileMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc);
    
SWError SWSetPartialMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc);
*/

void SWSetCustomTileDrawProc(
    SpriteWorldPtr          spriteWorldP,
    CustomTileDrawProcPtr   customTileDrawProc);

void SWStdCustomTileDrawProc(
    SpriteWorldPtr spriteWorldP,
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    SWRect *srcRectP,
    SWRect *dstRectP,
    short tileLayer,
    short tileID,
    short curTileImage,
        SWBoolean useMask );

void SWSetTileChangeProc(
    SpriteWorldPtr      spriteWorldP,
    TileChangeProcPtr   tileChangeProc);

SWError SWDrawTilesInBackground(
    SpriteWorldPtr  spriteWorldP);

void SWDrawTile(
    SpriteWorldPtr  spriteWorldP,
    short           dstTileLayer,
    short           tileRow,
    short           tileCol,
    short           tileID);

void SWDrawTilesInRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    SWBoolean           faster);

void SWDrawTileLayersInRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    SWBoolean       optimizingOn);

void SWDrawTileLayersInWorkArea(
    SpriteWorldPtr  spriteWorldP);

void SWDrawTilesAboveSprite(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    short           startLayer);

void SWResetTilingCache(
    SpriteWorldPtr  spriteWorldP);

void SWAddChangedRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *changedRectP);

void SWChangeTileImage(
    SpriteWorldPtr  spriteWorldP,
    short           tileID,
    short           currentFrame);

void SWUpdateTileOnScreen(
    SpriteWorldPtr  spriteWorldP,
    short           tileID);

void SWUpdateTilesOnScreenFast(
    SpriteWorldPtr  spriteWorldP,
    short           firstTileID,
    short           lastTileID);

void SWResetCurrentTileImages(
    SpriteWorldPtr  spriteWorldP);

short SWReturnTileUnderPixel(
    SpriteWorldPtr  spriteWorldP,
    short   tileLayer,
    short   pixelCol,
    short   pixelRow);

SWBoolean SWCheckSpriteWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       srcSpriteP,
    SWTileSearchType searchType,
    SWRect          *insetRectP,
    short           startTileLayer,
    short           endTileLayer,
    short           firstTileID,
    short           lastTileID,
    SWBoolean       fixPosition);

void SWWrapRectToWorkArea(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         dstRectP);

void SWWrapRectFromExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *dstRectP);


#ifdef __cplusplus
}

SW_NAMESPACE_BEGIN

#include "Tiling.hpp"

SW_NAMESPACE_END

#endif /* __cplusplus */

#endif /* __TILING__ */


