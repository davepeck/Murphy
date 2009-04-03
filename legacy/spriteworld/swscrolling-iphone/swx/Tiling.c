//---------------------------------------------------------------------------------------
//  Tiling.c
//
//  By Vern Jensen. SWLoadTile routines by Karl Bunker.
//
//  Created: 11/15/95, Updated 7/05/03 for SWX
//
//  Description:    Routines to use tiling with SpriteWorld
//---------------------------------------------------------------------------------------

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITEWORLD__
#include <SpriteWorld.h>
#endif

#ifndef __SPRITE__
#include <Sprite.h>
#endif

#ifndef __SPRITEFRAME__
#include <SpriteFrame.h>
#endif

#ifndef __SPRITEWORLDUTILS__
#include <SpriteWorldUtils.h>
#endif

#ifndef __TILING__
#include <Tiling.h>
#endif

#ifndef __BLITKERNEL__
#include <BlitKernel.h>
#endif

    extern SWBoolean gSWUseOpenGL;

/*
    // Prototype for a private function used only by SWDisposeAllTilesFast.
static void _addValueToGWorldArray(
    GWorldPtr gWorldP,
    GWorldPtr *gWorldArray,
    short maxNumElements);
*/

//---------------------------------------------------------------------------------------
//  SWInitTiling - Note: Whenever changing this function, change the sister function in
//  Multi-Screen Scrolling.c too!
//---------------------------------------------------------------------------------------

SWError SWInitTiling(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth,
    short           maxNumTiles)
{
    SWError err = kNoError;
    short   tileIndex;

    SW_ASSERT(spriteWorldP != NULL);

    if (spriteWorldP->tilingIsInitialized)
    {
        err = kTilingAlreadyInitialized;
    }


    if (err == kNoError)
    {
        spriteWorldP->tilingIsOn = true;
        spriteWorldP->tilingIsInitialized = true;
        spriteWorldP->lastActiveTileLayer = 0;
        spriteWorldP->maxNumTiles = maxNumTiles;
        spriteWorldP->tileHeight = tileHeight;
        spriteWorldP->tileWidth = tileWidth;                
    }


    if (err == kNoError)
    {
            // Create both tiling cache and changedTiles array of rects
        err = SWInitTilingCache(spriteWorldP);
    }


    if (err == kNoError)
    {
            // Allocate memory for tileFrameArray
        spriteWorldP->tileFrameArray = (FramePtr *)calloc(maxNumTiles,sizeof(FramePtr));
                if ( !spriteWorldP->tileFrameArray )
                {
                    err = kMemoryAllocationError;
                }
    }
        /*
    if (err == kNoError)
    {
            // Allocate memory for tilePartialFrameArray
        spriteWorldP->tilePartialFrameArray = (FramePtr *)calloc(maxNumTiles,sizeof(FramePtr));
                if ( !spriteWorldP->tilePartialFrameArray )
                {
                    err = kMemoryAllocationError;
                }
    }
        */
        
    if (err == kNoError)
    {
            // Allocate memory for curTileImage array
        spriteWorldP->curTileImage = (short *)calloc(maxNumTiles,sizeof(short));
                if ( !spriteWorldP->curTileImage )
                {
                    err = kMemoryAllocationError;
                }

        if (err == kNoError)
        {
                // Set up values in curTileImage array
            for (tileIndex = 0; tileIndex < maxNumTiles; tileIndex++)
                spriteWorldP->curTileImage[tileIndex] = tileIndex;
        }
    }


    if (err == kNoError)
    {
            // Allocate memory for tileLayerArray
        spriteWorldP->tileLayerArray = (TileMapStructPtr *)calloc(kSWNumTileLayers,sizeof(TileMapStructPtr));
        if ( !spriteWorldP->tileLayerArray )
                {
                    err = kMemoryAllocationError;
                }
    }


    return err;
}


//---------------------------------------------------------------------------------------
//  SWExitTiling
//---------------------------------------------------------------------------------------

void SWExitTiling(
    SpriteWorldPtr  spriteWorldP)
{
    short   tileIndex;
    SWError err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);

        // Was tiling ever initialized?
    if (spriteWorldP->tilingIsInitialized)
    {
        //err = SWDisposeAllTilesFast(spriteWorldP);

            // If there wasn't enough memory to dispose the tiles fast, use the old way...
        if (err != kNoError)
        {
            tileIndex = spriteWorldP->maxNumTiles;
            while (tileIndex--)
            {
                SWDisposeTile(spriteWorldP, tileIndex);
            }
        }

        free(spriteWorldP->tileFrameArray);
        //free(spriteWorldP->tilePartialFrameArray);
        free(spriteWorldP->curTileImage);
        free(spriteWorldP->tileLayerArray);
        free(spriteWorldP->changedTiles);
        spriteWorldP->changedTiles = NULL;

        free(spriteWorldP->tilingCache[0]); // Dispose the data
        free(spriteWorldP->tilingCache);    // Dispose the array of pointers
        spriteWorldP->tilingCache = NULL;


        spriteWorldP->tilingIsInitialized = false;
        spriteWorldP->tilingIsOn = false;
    }
}


//---------------------------------------------------------------------------------------
//  SWDisposeAllTilesFast -- called by SWExitTiling. This disposes a tile's Frame
//  without first checking to see if other tiles are using the same GWorld.
//  This means the tiles are disposed much faster when there are thousands of tileIDs.
//  This function might fail if there is not enough memory to create the 2 temporary arrays.
//---------------------------------------------------------------------------------------
/*
SWError SWDisposeAllTilesFast(
    SpriteWorldPtr  spriteWorldP)
{
    short       tileIndex, numElements;
    FramePtr    tileFrameP;
    GWorldPtr   *sharedGWorldArray = NULL,
                *sharedMaskArray = NULL;
    SWError     err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(spriteWorldP->tilingIsInitialized);

        // First, of maxNumTiles allowed, see how many tiles are actually in use.
    numElements = 0;
    for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
    {
        if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
            numElements++;
    }

        // Next, make two arrays with as many elements as we have tiles,
        // to make sure they can hold as many sharedGWorldPtrs as we might have.
    sharedGWorldArray = (GWorldPtr *)NewPtrClear( (Size)sizeof(GWorldPtr) * numElements + 1);
    sharedMaskArray = (GWorldPtr *)NewPtrClear( (Size)sizeof(GWorldPtr) * numElements + 1);

    if (sharedGWorldArray == NULL)
        err = MemError();

    if (sharedGWorldArray == NULL)
    {
        err = MemError();
        DisposePtr( (Ptr)sharedGWorldArray );
    }

    if (err == kNoError)
    {
            // Next, add to the array a list of each shared tile GWorld.
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
            tileFrameP = spriteWorldP->tileFrameArray[tileIndex];

            if (tileFrameP != NULL && tileFrameP->sharesGWorld == true)
            {
                    // Parameters: theValue, theArray, arrayMaxNumElements
                _addValueToGWorldArray(tileFrameP->framePort, sharedGWorldArray, numElements);

                if (tileFrameP->maskPort != NULL)
                    _addValueToGWorldArray(tileFrameP->maskPort, sharedMaskArray, numElements);
            }
        }
    }

        // Next, dispose of tiles normally, but don't dispose of shared GWorlds yet
    if (err == kNoError)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
                // Dispose of the partialFrame, if one exists.
            if (spriteWorldP->tilePartialFrameArray[tileIndex] != spriteWorldP->tileFrameArray[tileIndex])
                (void)SWDisposeFrame( &spriteWorldP->tilePartialFrameArray[tileIndex] );

                // Dispose of the normal frame
            (void)SWDisposeFrame( &spriteWorldP->tileFrameArray[tileIndex] );
            spriteWorldP->tileFrameArray[tileIndex] = NULL;
            spriteWorldP->tilePartialFrameArray[tileIndex] = NULL;
        }
    }

        // Finally, dispose of the shared GWorlds (by doing it this way, we ensure
        // that each GWorld is disposed only once.)
    if (err == kNoError)
    {
            // Dispose all GWorlds stored in the arrays
        for (tileIndex = 0; sharedGWorldArray[tileIndex] != NULL; tileIndex++)
            DisposeGWorld( sharedGWorldArray[tileIndex] );

        for (tileIndex = 0; sharedMaskArray[tileIndex] != NULL; tileIndex++)
            DisposeGWorld( sharedMaskArray[tileIndex] );
    }

        // Finally, dispose the two arrays we created in this function.
    DisposePtr( (Ptr)sharedGWorldArray );
    DisposePtr( (Ptr)sharedMaskArray );

    return err;
}


//---------------------------------------------------------------------------------------
//  _addValueToGWorldArray - (private function called only by SWDisposeAllTilesFast)
//  Assumes gWorldArray has been created using NewPtrClear.
//---------------------------------------------------------------------------------------

static void _addValueToGWorldArray(
    GWorldPtr gWorldP,
    GWorldPtr *gWorldArray,
    short maxNumElements)
{
    short   n;

    for (n = 0; n < maxNumElements; n++)
    {
        if (gWorldArray[n] == gWorldP)
        {
            return; // This GWorld has already been added to the array
        }
        else if (gWorldArray[n] == NULL)
        {
                // We've reached the first unused element, so add the value and exit.
            gWorldArray[n] = gWorldP;
            return;
        }
    }
}
*/

//---------------------------------------------------------------------------------------
//  SWInitTilingCache - an internal function; called by SWInitTiling and SWChangeTileSize.
//  Creates both the tiling cache and the changedTiles array of rects.
//---------------------------------------------------------------------------------------

SWError SWInitTilingCache(
    SpriteWorldPtr  spriteWorldP)
{
    short   row, col, numTilingCacheCols, numTilingCacheRows, *tilingCacheData;
    SWError err = kNoError;

        // Dispose the old tilingCache if necessary
    if (spriteWorldP->tilingCache != NULL)
    {
        free(spriteWorldP->tilingCache[0]); // Dispose the data
        free(spriteWorldP->tilingCache);        // Dispose the array of pointers
    }

        // Dispose the old changeTiles array of rects if necessary
    if (spriteWorldP->changedTiles != NULL)
    {
        free(spriteWorldP->changedTiles);
    }
                
    numTilingCacheRows = (short)ceil( (double)((float)spriteWorldP->backRect.bottom / (float)spriteWorldP->tileHeight) );
    numTilingCacheCols = (short)ceil( (double)((float)spriteWorldP->backRect.right / (float)spriteWorldP->tileWidth) );
    spriteWorldP->numTilingCacheRows = numTilingCacheRows;
    spriteWorldP->numTilingCacheCols = numTilingCacheCols;


        // Allocate memory for changedTiles array of rects
    spriteWorldP->changedTilesArraySize = (numTilingCacheRows+1) * (numTilingCacheCols+1);
    spriteWorldP->changedTiles = (SWRect *)calloc((numTilingCacheRows+1) * (numTilingCacheCols+1),sizeof(SWRect));
        if ( !spriteWorldP->changedTiles )
        {
            err = kMemoryAllocationError;
        }


    if (err == kNoError)
    {
            // Allocate the array of pointers for the tiling Cache
        spriteWorldP->tilingCache = (short **)calloc(numTilingCacheRows,sizeof(short*));
        if ( !spriteWorldP->tilingCache )
                {
                    err = kMemoryAllocationError;
                }

        if (err == kNoError)
        {
                        // Allocate memory for the actual data of the tiling Cache
                        tilingCacheData = (short *)calloc(numTilingCacheRows * numTilingCacheCols,sizeof(short));
            if ( !tilingCacheData )
                        {
                            err = kMemoryAllocationError;
                        }

                // If there was an error, dispose what we already created earlier
            if (err != kNoError)
                free(spriteWorldP->tilingCache);
        }

        if (err == kNoError)
        {
                // Point each element of the tilingCache array to each row of the data
            for (row = 0; row < numTilingCacheRows; row++)
                spriteWorldP->tilingCache[row] =
                    &tilingCacheData[(unsigned long)row * numTilingCacheCols];

                // Set all elements to -1 (indicating that each tile needs to be drawn)
            for (row = 0; row < numTilingCacheRows; row++)
                for (col = 0; col < numTilingCacheCols; col++)
                    spriteWorldP->tilingCache[row][col] = -1;
        }
    }

    if (err)
    {
        spriteWorldP->tilingCache = NULL;
        spriteWorldP->changedTiles = NULL;
    }

    return err;
}


//---------------------------------------------------------------------------------------
//  SWCreateTileMap
//---------------------------------------------------------------------------------------

SWError SWCreateTileMap(
    TileMapStructPtr    *tileMapStructPP,
    short               numTileMapRows,
    short               numTileMapCols)
{
    TileMapStructPtr    tileMapStructP;
    int             i;
    SWError             err = kNoError;


        // Allocate memory for the TileMapStruct
    tileMapStructP = (TileMapStructPtr)calloc(1,sizeof(TileMapStruct));
        if (!tileMapStructP)
        {
            err = kMemoryAllocationError;
        }

    if (err == kNoError)
    {
        tileMapStructP->numRows = numTileMapRows;
        tileMapStructP->numCols = numTileMapCols;

            // Allocate the tile map rows
        tileMapStructP->tileMap = (short**)calloc(numTileMapRows,sizeof(short*));
        if (!tileMapStructP->tileMap)
                {
                    err = kMemoryAllocationError;
                }
    }

    if (err == kNoError)
    {
            // Allocate memory for the actual data of the TileMap
                for( i = 0; i < numTileMapRows; ++i )
                {
                    tileMapStructP->tileMap[i] = (short *)calloc(numTileMapCols,sizeof(short));
                    
                    if (!tileMapStructP->tileMap[i])
                    {
                        err = kMemoryAllocationError;
                        break;
                    }
                }
    }

    if (err == kNoError)
    {
            SWLockTileMap(tileMapStructP);
    }


    if (err != kNoError)
    {
            // Dispose what we created
        if (tileMapStructP != NULL)
        {
                        for( i = 0; i < numTileMapRows; ++i )
                        {                            
                            if ( tileMapStructP->tileMap[i] != NULL )
                            {
                                free( tileMapStructP->tileMap[i] );
                            }
                        }
                
            free( tileMapStructP->tileMap );
            free(tileMapStructP);
            tileMapStructP = NULL;
        }
    }

        // Return a pointer to the TileMapStruct in the tileMapStructPP variable
    *tileMapStructPP = tileMapStructP;

    return err;
}


//---------------------------------------------------------------------------------------
//  SWDisposeTileMap
//---------------------------------------------------------------------------------------

void SWDisposeTileMap(
    TileMapStructPtr    *tileMapStructPP)
{
    TileMapStructPtr    myTileMapStructP = *tileMapStructPP;
        int i;
        
    if (myTileMapStructP != NULL)
    {
                for( i = 0; i < myTileMapStructP->numRows; ++i )
                {                            
                    if ( myTileMapStructP->tileMap[i] != NULL )
                    {
                        free( myTileMapStructP->tileMap[i] );
                    }
                }
        
                free( myTileMapStructP->tileMap );
                free(myTileMapStructP);
        *tileMapStructPP = NULL;
    }
}


//---------------------------------------------------------------------------------------
//  SWLockTileMap
//---------------------------------------------------------------------------------------

void SWLockTileMap(
    TileMapStructPtr    tileMapStructP)
{
    SW_ASSERT(tileMapStructP != NULL);

    tileMapStructP->isLocked = true;
}


//---------------------------------------------------------------------------------------
//  SWUnlockTileMap
//---------------------------------------------------------------------------------------

void SWUnlockTileMap(
    TileMapStructPtr    tileMapStructP)
{
    SW_ASSERT(tileMapStructP != NULL);

    tileMapStructP->isLocked = false;
}


//---------------------------------------------------------------------------------------
//  SWInstallTileMap
//---------------------------------------------------------------------------------------

void SWInstallTileMap(
    SpriteWorldPtr      spriteWorldP,
    TileMapStructPtr    tileMapStructP,
    short               tileLayer)
{
    short   curLayer;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(tileLayer < kSWNumTileLayers);

        // Install the TileMap
    spriteWorldP->tileLayerArray[tileLayer] = tileMapStructP;
    spriteWorldP->lastActiveTileLayer = 0;

        // Find the last active tile layer
    for (curLayer = 0; curLayer < kSWNumTileLayers; curLayer++)
    {
        if (spriteWorldP->tileLayerArray[curLayer] != NULL)
            spriteWorldP->lastActiveTileLayer = curLayer;
    }

        // Set the appropriate tileRectDrawProc
    if (spriteWorldP->lastActiveTileLayer == 0)
    {
        spriteWorldP->tileRectDrawProc = SWDrawTilesInRect;
    }
    else
    {
        spriteWorldP->tileRectDrawProc = SWDrawTileLayersInRect;
    }
}


//---------------------------------------------------------------------------------------
//    SWLoadTileMap
//---------------------------------------------------------------------------------------

SWError SWLoadTileMap( const char * file, TileMapStructPtr *tileMapStructPP )
{
   FILE                 *fp;
   TileMapStructPtr     tempMapStruct;
   short                numRows, numCols, row, col, i;
   int                  locked;
   SWError                 err = kNoError;
      /*needs a bit more error checking with the fread() etc., but it all works*/
   tempMapStruct = *tileMapStructPP;
      //open the data file
   fp = fopen(file, "rb");
          //read the row and col values
   fread(&numRows, sizeof(short), 1, fp);
   fread(&numCols, sizeof(short), 1, fp);
      //initialize the tile map with the row and col values
   err = SWCreateTileMap(&tempMapStruct, numRows, numCols);
      //read the tileMap values byte by byte
   for (row = 0; row < numRows; row++)
   {
      for (col = 0; col < numCols; col++)
      {
               fread(&i, sizeof(short), 1, fp);
               tempMapStruct->tileMap[row][col] = i;
      }
   }
      //read the lock state... is this necissary?
   fread(&locked, sizeof(int), 1, fp);
   tempMapStruct->isLocked = locked;
      //close
   fclose(fp);
         *tileMapStructPP = tempMapStruct;
    return err;
}

//---------------------------------------------------------------------------------------
//    SWSaveTileMap
//---------------------------------------------------------------------------------------

SWError SWSaveTileMap( const char * file,
   TileMapStructPtr    tileMapStructP )
{
   FILE        *fp;
   short       numRows, numCols, row, col, i;
   int         locked;
   SWError        err = kNoError;

       // Make sure there is a tileMap to save
   if (tileMapStructP == NULL)
       err = kNullTileMapErr;

   if (err == kNoError)
   {
       fp = fopen(file, "wb");
              //place the row col values into regular short variables
       numRows = tileMapStructP->numRows;
       numCols = tileMapStructP->numCols;
              //write them to the data file
        fwrite(&numRows, sizeof(short), 1, fp);
       fwrite(&numCols, sizeof(short), 1, fp);
              //read each value of the tileMap into i then write i to the file
       for (row = 0; row < numRows; row++)
       {
           for (col = 0; col < numCols; col++)
           {
                     i = tileMapStructP->tileMap[row][col];
                     fwrite(&i, sizeof(short), 1, fp);
           }
       }
              //take the isLocked data and write it to the file. Is this necissary?
       locked = tileMapStructP->isLocked;
       fwrite(&locked, sizeof(int), 1, fp);
              //close
       fclose(fp);
   }

   SWSetStickyIfError(err);
   return err;
}


//---------------------------------------------------------------------------------------
//  SWResizeTileMap
//---------------------------------------------------------------------------------------

SWError SWResizeTileMap(
    TileMapStructPtr    oldTileMapStructP,
    short               numNewTileMapRows,
    short               numNewTileMapCols)
{
    TileMapStruct       tempTileMapStruct;
    TileMapStructPtr    newTileMapStructP;
    short               row, numRowsToCopy, numColsToCopy;
    int             arraySize;
    SWError             err = kNoError;

    SW_ASSERT(numNewTileMapRows > 0 && numNewTileMapCols > 0);

    if (oldTileMapStructP == NULL)
        err = kNullTileMapErr;


    if (err == kNoError)
    {
            // Don't do anything if the TileMap is already the requested size.
        if (oldTileMapStructP->numRows == numNewTileMapRows &&
            oldTileMapStructP->numCols == numNewTileMapCols)
        {
            return kNoError;
        }

            // Create the new TileMap
        err = SWCreateTileMap(&newTileMapStructP, numNewTileMapRows, numNewTileMapCols);
    }

    if (err == kNoError)
    {
        if (oldTileMapStructP->isLocked == false)
            SWLockTileMap(oldTileMapStructP);

        numRowsToCopy = SW_MIN(oldTileMapStructP->numRows, numNewTileMapRows);
        numColsToCopy = SW_MIN(oldTileMapStructP->numCols, numNewTileMapCols);

            // Copy the data from the old TileMap to the new TileMap
        arraySize = numColsToCopy * sizeof(short);
        for (row = 0; row < numRowsToCopy; row++)
        {
                        memcpy( &newTileMapStructP->tileMap[row][0],
                                &oldTileMapStructP->tileMap[row][0], arraySize );
            //BlockMoveData(&oldTileMapStructP->tileMap[row][0],
            //  &newTileMapStructP->tileMap[row][0], arraySize);
        }

            // Swap contents of the new and old TileMapStructs
        tempTileMapStruct = *oldTileMapStructP;
        *oldTileMapStructP = *newTileMapStructP;
        *newTileMapStructP = tempTileMapStruct;

            // Dispose the newTileMapStruct, which now contains the old TileMap
        SWDisposeTileMap(&newTileMapStructP);
    }


    return err;
}

#if 0
#pragma mark -
#endif
        
SWError SWLoadTilesFromSingleFileXY( 
    const char* fileName,
    int frameWidth,
    int frameHeight,
    int borderWidth,
    int borderHeight,
    SWBoolean hasOutsideBorder,
    int     numTiles,
        SpriteWorldPtr  spriteWorldP,
    short       startTileID )
{
    SWError         err = kNoError;
    SDL_Surface     *surface;
    int             widthStart = 0, heightStart = 0;
    SWRect          frameRect;
    int         frame;
    FramePtr        newFrameP;
        short           tileIndex = startTileID;
        
        // We must be loading at least one tile
        SW_ASSERT( numTiles > 0 );
        
        // Load the image
    surface = BKLoadSurface( fileName );
    if( !surface )
        {
            err = kSDLCreateSurfaceFromFile;
    }
        
    if( err == kNoError )
    {
        if ( hasOutsideBorder )
        {
            widthStart = borderWidth / 2;
            heightStart = borderHeight / 2;
        }
        else
        {
            widthStart = 0;
            heightStart = 0;
        }
    }
                    
    if( err == kNoError )
    {
        frameRect.left = widthStart;
        frameRect.top = heightStart;
        frameRect.right = widthStart + frameWidth;
        frameRect.bottom = heightStart + frameHeight;
        
        for (frame = 0; frame < numTiles; frame++)
        {
                        // Check tile index
                        if (tileIndex >= spriteWorldP->maxNumTiles)
                        {
                                err = kOutOfRangeErr;
                        }
                        
                        if (err == kNoError)
            {
                            err = SWCreateFrameFromSurfaceAndRect( &newFrameP, surface, &frameRect );
            }
                        
            if (err == kNoError)
            {
                                // Are we replacing an old tile?
                                if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
                                {
                                        SWDisposeTile( spriteWorldP, tileIndex );
                                }
                                spriteWorldP->tileFrameArray[tileIndex] = newFrameP;
                                newFrameP->useCount++;
                                
                if ( frame < numTiles-1 )
                {
                    frameRect.left += frameWidth + borderWidth;
                    frameRect.right = frameRect.left + frameWidth;
                    if ( frameRect.right > surface->w )
                    {
                        frameRect.left = widthStart;
                        frameRect.right = widthStart + frameWidth;

                        frameRect.top += frameHeight + borderHeight;
                        frameRect.bottom = frameRect.top + frameHeight;
                        if ( frameRect.bottom > surface->h )
                        {
                            err = kOutOfRangeErr;
                        }
                    }
                }
            }
            
            if ( err != kNoError )
            {
                            SWDisposeTile( spriteWorldP, tileIndex );
                            break;
            }
                        
                        tileIndex++;
        }
    }
    
    SWSetStickyIfError( err );
    return err;
}

SWError SWLoadTilesFromFileSequence(
        const char  *filename,
        int         start,
        int     numTiles,
        SpriteWorldPtr  spriteWorldP,
    short       startTileID )
{
    SWError err = kNoError;
    char tmp_file[255];
    int x;
    short tileIndex = startTileID;

    FramePtr tempFrameP;
    
    SW_ASSERT( spriteWorldP != NULL );
    SW_ASSERT( filename != NULL );
    SW_ASSERT( numTiles > 0 );
    SW_ASSERT( startTileID >= 0 && startTileID < spriteWorldP->maxNumTiles );
    
    

    for( x = 0; x < numTiles; x++ )
    {
            sprintf(tmp_file, filename, x+start);
            
            // Check tile index
            if (tileIndex >= spriteWorldP->maxNumTiles)
            {
                    err = kOutOfRangeErr;
            }
            
            // Create the frame
            if (err == kNoError)
            {
                err = SWCreateFrameFromFile(&tempFrameP, tmp_file);
            }
            
            // Set the tile
            if (err == kNoError)
            {
                // Are we replacing an old tile?
                if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
                {
                    SWDisposeTile( spriteWorldP, tileIndex );
                }
                spriteWorldP->tileFrameArray[tileIndex] = tempFrameP;
                tempFrameP->useCount++;
            }
            
            if (err != kNoError) 
            {
                    SWDisposeTile( spriteWorldP, tileIndex );
                    break;
            }
            
            tileIndex++;
    }
    
    
    SWSetStickyIfError(err);
    return err;
}

/*
//---------------------------------------------------------------------------------------
//  SWCreatePartialTile
//---------------------------------------------------------------------------------------

SWError SWCreatePartialTile(
    SpriteWorldPtr  spriteWorldP,
    short           tileID )
{
    SWError         err = kNoError;
    FramePtr        frameP;
    //FramePtr      partialFrameP;

        SW_ASSERT( spriteWorldP != 0 );
        
        frameP = spriteWorldP->tileFrameArray[tileID];

                // dispose of old partial-only frame, if any
        if ( spriteWorldP->tilePartialFrameArray[tileID] != frameP)
                SWDisposeFrame(&spriteWorldP->tilePartialFrameArray[tileID]);

        // Just use the same frame
        spriteWorldP->tilePartialFrameArray[tileID] = frameP;
        
    return err;
}
*/

//---------------------------------------------------------------------------------------
//  SWDisposeTile
//---------------------------------------------------------------------------------------

void SWDisposeTile(
    SpriteWorldPtr  spriteWorldP,
    short           tileID)
{
    short       tileIndex;
    int     surfaceStillInUse;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(spriteWorldP->tilingIsInitialized);

        // see if any other tile is using this tile's GWorld
    surfaceStillInUse = false;
    tileIndex = spriteWorldP->maxNumTiles;
    while ( tileIndex-- )
    {
        if ( tileIndex != tileID )
        {
            if ( spriteWorldP->tileFrameArray[tileIndex] != NULL &&
                ((spriteWorldP->tileFrameArray[tileIndex])->frameSurfaceP ==
                (spriteWorldP->tileFrameArray[tileID])->frameSurfaceP) )
            {
                surfaceStillInUse = true;
            }
        }
    }
        // set flag that tells SWDisposeFrame whether to dispose of GWorld
    (spriteWorldP->tileFrameArray[tileID])->sharesSurface = surfaceStillInUse;
    /*if (spriteWorldP->tilePartialFrameArray[tileID] != spriteWorldP->tileFrameArray[tileID])
    {
        (void)SWDisposeFrame( &spriteWorldP->tilePartialFrameArray[tileID] );
    }*/

    (void)SWDisposeFrame( &spriteWorldP->tileFrameArray[tileID] );

    spriteWorldP->tileFrameArray[tileID] = NULL;
    //spriteWorldP->tilePartialFrameArray[tileID] = NULL;
}


//---------------------------------------------------------------------------------------
//  SWLockTiles
//---------------------------------------------------------------------------------------

void SWLockTiles(
    SpriteWorldPtr  spriteWorldP)
{
    short       tileIndex;

    SW_ASSERT(spriteWorldP != NULL);

        // Tiling might not be initialized if this was called from SWLockSpriteWorld
    if (spriteWorldP->tilingIsInitialized)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
            if (spriteWorldP->tileFrameArray != NULL && spriteWorldP->tileFrameArray[tileIndex] != NULL)
                SWLockFrame(spriteWorldP->tileFrameArray[tileIndex]);

            /*if (spriteWorldP->tilePartialFrameArray != NULL && 
                            spriteWorldP->tilePartialFrameArray[tileIndex] != NULL)
                        {
                SWLockFrame(spriteWorldP->tilePartialFrameArray[tileIndex]);
                        }*/

        }
    }
}


//---------------------------------------------------------------------------------------
//  SWUnlockTiles
//---------------------------------------------------------------------------------------

void SWUnlockTiles(
    SpriteWorldPtr  spriteWorldP)
{
    short       tileIndex;

    SW_ASSERT(spriteWorldP != NULL);

        // Tiling might not be initialized if this was called from SWLockSpriteWorld
    if (spriteWorldP->tilingIsInitialized)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
            if (spriteWorldP->tileFrameArray != NULL && spriteWorldP->tileFrameArray[tileIndex] != NULL)
                SWUnlockFrame(spriteWorldP->tileFrameArray[tileIndex]);
                                
            /*if (spriteWorldP->tilePartialFrameArray != NULL &&
                                spriteWorldP->tilePartialFrameArray[tileIndex] != NULL)
                        {
                SWUnlockFrame(spriteWorldP->tilePartialFrameArray[tileIndex]);
                        }*/
                        
        }
    }
}

//---------------------------------------------------------------------------------------
//  SWCreateExtraBackFrame
//---------------------------------------------------------------------------------------

SWError SWCreateExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *frameRect)
{
    SWError err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(frameRect->right > frameRect->left && frameRect->bottom > frameRect->top);

    if (spriteWorldP->extraBackFrameP != NULL)
    {
        SWDisposeFrame(&spriteWorldP->extraBackFrameP);
    }

        // Make sure rect starts at 0,0
    //OffsetRect(frameRect, -frameRect->left, -frameRect->top);

        err = SWCreateBlankFrame( &spriteWorldP->extraBackFrameP,
                                   SW_RECT_WIDTH( *frameRect ),
                                    SW_RECT_HEIGHT( *frameRect ),
                                    spriteWorldP->pixelDepth, false );
                                    
    //err = SWCreateFrame(spriteWorldP->mainSWGDH, &spriteWorldP->extraBackFrameP,
    //      frameRect, spriteWorldP->pixelDepth, kCreateGWorld);

    return err;
}


//---------------------------------------------------------------------------------------
//  SWDisposeExtraBackFrame
//---------------------------------------------------------------------------------------

void SWDisposeExtraBackFrame(
    SpriteWorldPtr  spriteWorldP)
{
    SW_ASSERT(spriteWorldP != NULL);

    if (spriteWorldP->extraBackFrameP != NULL)
    {
        SWDisposeFrame(&spriteWorldP->extraBackFrameP);
        spriteWorldP->extraBackFrameP = NULL;
    }
}

//---------------------------------------------------------------------------------------
//  SWSetTilingOn
//---------------------------------------------------------------------------------------

void SWSetTilingOn(
    SpriteWorldPtr  spriteWorldP,
    SWBoolean       tilingIsOn)
{
    SW_ASSERT(spriteWorldP != NULL);
    spriteWorldP->tilingIsOn = tilingIsOn;
}

//---------------------------------------------------------------------------------------
//  SWChangeTileSize
//---------------------------------------------------------------------------------------

SWError SWChangeTileSize(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth)
{
    SWError err;

    SW_ASSERT(spriteWorldP != NULL);

    spriteWorldP->tileHeight = tileHeight;
    spriteWorldP->tileWidth = tileWidth;

        // Dispose and rebuild the tiling cache, based on the new tile width & height
    err = SWInitTilingCache(spriteWorldP);

    return err;
}


//---------------------------------------------------------------------------------------
//  SWSetSpriteLayerUnderTileLayer
//---------------------------------------------------------------------------------------

void SWSetSpriteLayerUnderTileLayer( SpriteLayerPtr spriteLayerP, short tileLayer )
{
    SW_ASSERT(spriteLayerP != NULL);
    spriteLayerP->tileLayer = tileLayer;
}

/*
//---------------------------------------------------------------------------------------
//  SWSetTileMaskDrawProc
//---------------------------------------------------------------------------------------

SWError SWSetTileMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc)
{
    SWError err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(drawProc != NULL);

        spriteWorldP->tileMaskDrawProc = drawProc;

    return err;
}


//---------------------------------------------------------------------------------------
//  SWSetPartialMaskDrawProc
//---------------------------------------------------------------------------------------

SWError SWSetPartialMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc)
{
    SWError err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(drawProc != NULL);

        spriteWorldP->partialMaskDrawProc = drawProc;

    return err;
}
*/

//---------------------------------------------------------------------------------------
//  SWSetCustomTileDrawProc - By passing the DrawProc the tileID and tileLayer, users
//  can do special effects, like lighting and shadows, based on the tileID or layer.
//---------------------------------------------------------------------------------------

void SWSetCustomTileDrawProc(
    SpriteWorldPtr          spriteWorldP,
    CustomTileDrawProcPtr   customTileDrawProc)
{
    SW_ASSERT(spriteWorldP != NULL);
    spriteWorldP->customTileDrawProc = customTileDrawProc;
}


//---------------------------------------------------------------------------------------
//  SWStdCustomTileDrawProc
//---------------------------------------------------------------------------------------

void SWStdCustomTileDrawProc(
    SpriteWorldPtr spriteWorldP,
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    SWRect *srcRectP,
    SWRect *dstRectP,
    short tileLayer,
    short tileID,
    short curTileImage,
    SWBoolean useMask )
{
    SW_UNUSED(tileLayer);
    SW_UNUSED(tileID);
    SW_UNUSED(curTileImage);

    if (useMask)
            (*spriteWorldP->tileMaskDrawProc)(srcFrameP, dstFrameP, srcRectP, dstRectP);
    else
            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP, srcRectP, dstRectP);
                
        /*
    SWError err = kNoError;
    SDL_Rect srcSDLRect;
    SDL_Rect dstSDLRect;

    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked);

    SW_CONVERT_SW_TO_SDL_RECT( (*srcRectP), srcSDLRect );
    SW_CONVERT_SW_TO_SDL_RECT( (*dstRectP), dstSDLRect );
                
    err = SDL_BlitSurface(
        srcFrameP->frameSurfaceP,
        &srcSDLRect,
        dstFrameP->frameSurfaceP,
        &dstSDLRect );
        
    SWSetStickyIfError( err );
        */
}


//---------------------------------------------------------------------------------------
//  SWSetTileChangeProc
//---------------------------------------------------------------------------------------

void SWSetTileChangeProc(
    SpriteWorldPtr      spriteWorldP,
    TileChangeProcPtr   tileChangeProc)
{
    SW_ASSERT(spriteWorldP != NULL);
    spriteWorldP->tileChangeProc = tileChangeProc;
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//  SWDrawTilesInBackground
//---------------------------------------------------------------------------------------

SWError SWDrawTilesInBackground(
    SpriteWorldPtr  spriteWorldP)
{
    SWError         err = kNoError;

    SW_ASSERT(spriteWorldP != NULL);


    if ( !spriteWorldP->tilingIsInitialized )
        err = kTilingNotInitialized;

    if (err == kNoError)
    {
        (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &spriteWorldP->visScrollRect, true);
    }

    return err;
}


//---------------------------------------------------------------------------------------
//  SWDrawTile - sets value in tileMap and draws tile if visible in visScrollRect.
//  The main core of this function is very similar to the inner loop of
//  SWTileDrawLayersInRect, except that this calls SWAddChangedRect, has code to set the
//  tiling cache, and makes sure the tile is visible on screen before drawing it.
//  Oh, and SWDrawTile *sets* the tileID in the tileMap, instead of reading it. :-)
//---------------------------------------------------------------------------------------

void SWDrawTile(
    SpriteWorldPtr  spriteWorldP,
    short           dstTileLayer,
    short           tileRow,
    short           tileCol,
    short           tileID)
{
    short       row, col, offscreenTileRow, offscreenTileCol, tileLayer, firstLayerToDraw;
    SWRect*     visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect*     backRectP = &spriteWorldP->backRect;
    short       tileHeight = spriteWorldP->tileHeight;
    short       tileWidth = spriteWorldP->tileWidth;
    SWRect      srcRect, dstRect;
    FramePtr    tileFrameP = NULL;
    int     tileClipped, tileHasMask;

        // We must have a TileMap installed in the dstTileLayer to draw in it!
    if (spriteWorldP->tileLayerArray[dstTileLayer] == NULL)
        return;

        // Check SpriteWorldRec
    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(spriteWorldP->tilingIsInitialized);
    SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL);
//  SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);
    SW_ASSERT(spriteWorldP->tileLayerArray[dstTileLayer] != NULL);
    SW_ASSERT(spriteWorldP->tileLayerArray[dstTileLayer]->isLocked);

        // Check parameters
    SW_ASSERT(dstTileLayer >= 0);
    SW_ASSERT(dstTileLayer <= spriteWorldP->lastActiveTileLayer);
    SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[dstTileLayer]->numRows);
    SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[dstTileLayer]->numCols);
    SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

        // Note: we can not return if the tileID is what is already in the TileMap,
        // since tile animation would then not work.


        // Store the new tileID in the TileMap of the dstTileLayer
    spriteWorldP->tileLayerArray[dstTileLayer]->tileMap[tileRow][tileCol] = tileID;


        // Determine if the tile being drawn has a mask, so we know if we need to
        // redraw the tiles under it.
    if (tileID >= 0)
    {
        tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
        SW_ASSERT(tileFrameP != NULL);
        SW_ASSERT(tileFrameP->isFrameLocked);

            // Determine whether the tile has a mask with background showing through
        tileHasMask = (tileFrameP->maskType != kNoMask);
    }
    else
        tileHasMask = true; // Consider an empty tile to have an "empty" (see-through) mask.

        // If the tile being drawn does NOT have a mask, we can skip drawing all tiles
        // in the layers under it. Although level designers can typically avoid this, they
        // can't avoid having the extraBackFrame redrawn each time a tile in a higher layer
        // is animated. This code handles this, giving a nice speed boost.
    if (tileHasMask)
        firstLayerToDraw = 0;
    else
        firstLayerToDraw = dstTileLayer;

    row = tileRow * tileHeight;
    col = tileCol * tileWidth;

    dstRect.bottom = row + tileHeight;
    dstRect.right = col + tileWidth;

        // Clip tile dstRect with visScrollRect //
    tileClipped = false;
    if (row < visScrollRectP->top)
    {
        dstRect.top = visScrollRectP->top;
        tileClipped = true;
    }
    else
        dstRect.top = row;

    if (col < visScrollRectP->left)
    {
        dstRect.left = visScrollRectP->left;
        tileClipped = true;
    }
    else
        dstRect.left = col;

    if (dstRect.bottom > visScrollRectP->bottom)
    {
        dstRect.bottom = visScrollRectP->bottom;
        tileClipped = true;
    }

    if (dstRect.right > visScrollRectP->right)
    {
        dstRect.right = visScrollRectP->right;
        tileClipped = true;
    }


        // Draw tile if visible on screen (in visScrollRect)
    if (dstRect.left < dstRect.right && dstRect.top < dstRect.bottom)
    {
            // Save rect as having been changed
        SWAddChangedRect(spriteWorldP, &dstRect);

            // Now get the tileID of this row and col in tileLayer 0
        if (spriteWorldP->tileLayerArray[0] == NULL)
            tileID = -1;
        else
            tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            // Now we redraw all tiles in this location
                    
        if (tileID >= 0)
        {
                        tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                        SW_ASSERT(tileFrameP != NULL);
                        SW_ASSERT(tileFrameP->isFrameLocked);

                        // Determine whether the tile has a mask with background showing through
            tileHasMask = (tileFrameP->maskType != kNoMask);
        }
        else
            tileHasMask = false;
                                
            // Copy a piece from the extraBackFrameP only if there is no tile, or the
            // tile has a mask with background showing through the unmasked part.
        if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL &&
             firstLayerToDraw == 0 )
        {
                // There is no tile in this spot, or there is a masked tile and
                // there is an extraBackFrameP, so copy a piece from extraBackFrameP
                // Note: the function below wraps the dstRect for us, and leaves it
                // that way when it returns, so we don't have to wrap it ourselves.
            SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
        }
        else
        {
                // Make the tile's dest rect local to the offscreen area
            dstRect.top -= spriteWorldP->vertScrollRectOffset;
            dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            dstRect.left -= spriteWorldP->horizScrollRectOffset;
            dstRect.right -= spriteWorldP->horizScrollRectOffset;

                // Wrap tile to top or bottom of offscreen area
            if (dstRect.bottom > backRectP->bottom)
            {
                dstRect.top -= backRectP->bottom;
                dstRect.bottom -= backRectP->bottom;
            }
            else if (dstRect.top < backRectP->top)
            {
                dstRect.top += backRectP->bottom;
                dstRect.bottom += backRectP->bottom;
            }

                // Wrap tile to left or right side of offscreen area
            if (dstRect.right > backRectP->right)
            {
                dstRect.left -= backRectP->right;
                dstRect.right -= backRectP->right;
            }
            else if (dstRect.left < backRectP->left)
            {
                dstRect.left += backRectP->right;
                dstRect.right += backRectP->right;
            }
        }


        if (tileID >= 0)
        {
            srcRect = tileFrameP->frameRect;

                // Clip new srcRect
            if (row < visScrollRectP->top)
                srcRect.top += visScrollRectP->top - row;
            if (col < visScrollRectP->left)
                srcRect.left += visScrollRectP->left - col;
            if (row + tileHeight > visScrollRectP->bottom)
                srcRect.bottom -= row + tileHeight - visScrollRectP->bottom;
            if (col + tileWidth > visScrollRectP->right)
                srcRect.right -= col + tileWidth - visScrollRectP->right;


            {
                int useMask;

                useMask = (tileHasMask && spriteWorldP->extraBackFrameP != NULL);
                                
                    // Call the function that draws this tile.
                (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                        tileFrameP, spriteWorldP->backFrameP,
                        &srcRect, &dstRect, 0, tileID,
                        spriteWorldP->curTileImage[tileID], useMask );
            }
        }

            // We may have already drawn layer 0; don't draw it again.
        if (firstLayerToDraw == 0)
            firstLayerToDraw = 1;

            // Draw tiles in higher layers
        for (tileLayer = firstLayerToDraw; tileLayer <= spriteWorldP->lastActiveTileLayer;
            tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows);
            SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols);

            tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
            if (tileID < 0)
                continue;

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

            tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
            SW_ASSERT(tileFrameP != NULL);
            SW_ASSERT(tileFrameP->isFrameLocked);

            srcRect = tileFrameP->frameRect;

                // Clip new srcRect
            if (row < visScrollRectP->top)
                srcRect.top += visScrollRectP->top - row;
            if (col < visScrollRectP->left)
                srcRect.left += visScrollRectP->left - col;
            if (row + tileHeight > visScrollRectP->bottom)
                srcRect.bottom -= row + tileHeight - visScrollRectP->bottom;
            if (col + tileWidth > visScrollRectP->right)
                srcRect.right -= col + tileWidth - visScrollRectP->right;


            {
                int useMask;

                useMask = (tileFrameP->maskType != kNoMask);

                    // Call the function that draws this tile.
                (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                        tileFrameP, spriteWorldP->backFrameP,
                        &srcRect, &dstRect, tileLayer, tileID,
                        spriteWorldP->curTileImage[tileID], useMask );
            }
        }

            // Copy tiles from back area to work area
        (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                spriteWorldP->workFrameP, &dstRect, &dstRect);

            // Update tiling cache info only if only one tile layer is used
        if (spriteWorldP->lastActiveTileLayer == 0)
        {
            offscreenTileRow = dstRect.top / spriteWorldP->tileHeight;
            offscreenTileCol = dstRect.left / spriteWorldP->tileWidth;

                // Set new tile value in tilingCache
            if (tileClipped || tileID < 0)
            {
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
            }
            else
            {
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] =
                    spriteWorldP->curTileImage[tileID];
            }
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWDrawTilesInRect - draw only one layer of tiles in updateRect of backFrame. Note:
//  The only reason we use this instead of SWDrawTilesLayersInRect is because with this,
//  we can use the tiling cache.
//---------------------------------------------------------------------------------------

void SWDrawTilesInRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    SWBoolean       optimizingOn)
{
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       offscreenTileRow, offscreenTileCol;
    SWRect      srcRect, dstRect;
    FramePtr    tileFrameP = NULL;
    SWBoolean   tileClipped, tileHasMask;
    SWRect*     visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect*     backRectP = &spriteWorldP->backRect;
    SWRect      updateRect = *updateRectP;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
                
    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    //SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);

    SW_ASSERT(spriteWorldP->tileLayerArray[0] != NULL);
    SW_ASSERT(spriteWorldP->tileLayerArray[0]->isLocked);
    SW_ASSERT(tileWidth * spriteWorldP->tileLayerArray[0]->numCols < 32767);
    SW_ASSERT(tileHeight * spriteWorldP->tileLayerArray[0]->numRows < 32767);


        // Convert pixel row and col into tile row and col
    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;


    row = startRow * tileHeight;
    for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
    {
        col = startCol * tileWidth;
        for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
        {
            if (spriteWorldP->tileLayerArray[0] != NULL)
            {
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[0]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[0]->numCols);
            }

            dstRect.bottom = row + tileHeight;
            dstRect.right = col + tileWidth;

                // Is tile completely visible on screen?
            if (row >= visScrollRectP->top && col >= visScrollRectP->left &&
                dstRect.bottom <= visScrollRectP->bottom &&
                dstRect.right <= visScrollRectP->right)
            {
                tileClipped = false;
            }
            else
            {
                tileClipped = true;
            }

                // Clip tile dstRect with updateRect //
            if (row < updateRect.top)
                dstRect.top = updateRect.top;
            else
                dstRect.top = row;

            if (col < updateRect.left)
                dstRect.left = updateRect.left;
            else
                dstRect.left = col;

            if (dstRect.bottom > updateRect.bottom)
                dstRect.bottom = updateRect.bottom;

            if (dstRect.right > updateRect.right)
                dstRect.right = updateRect.right;

            if (spriteWorldP->tileLayerArray[0] == NULL)
                tileID = -1;
            else
                tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles);


            if (tileID >= 0)
            {
                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL);
                SW_ASSERT(tileFrameP->isFrameLocked);

                    // Determine whether the tile has a mask with background showing through
                tileHasMask = (tileFrameP->maskType != kNoMask);
            }
            else
                tileHasMask = false;

                // Copy a piece from the extraBackFrameP only if there is no tile, or the
                // tile has a mask with background showing through the unmasked part.
            if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
            {                                
                    // There is no tile in this spot, or there is a masked tile and
                    // there is an extraBackFrameP, so copy a piece from extraBackFrameP
                    // Note: the function below wraps the dstRect for us, and leaves it
                    // that way when it returns, so we don't have to wrap it ourselves.
                SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
            }
            else
            {
                    // Make the tile's dest rect local to the offscreen area
                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                    // Wrap tile to top or bottom of offscreen area
                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                    // Wrap tile to left or right side of offscreen area
                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }
            }

            offscreenTileRow = dstRect.top / tileHeight;
            offscreenTileCol = dstRect.left / tileWidth;
            if (tileID >= 0)
            {
                srcRect = tileFrameP->frameRect;

                    // Clip new srcRect
                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;
                                
                if (tileHasMask && spriteWorldP->extraBackFrameP != NULL)
                {                                        
                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->backFrameP,
                            &srcRect, &dstRect, 0, tileID,
                            spriteWorldP->curTileImage[tileID], tileHasMask );

                        // Since the background is showing here, set this cache spot to -1
                    spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
                }

                else
                {
                        // Save time by not drawing tile if already the same
                    if (spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] !=
                        spriteWorldP->curTileImage[tileID] || !optimizingOn)
                    {
                            // Call the function that draws this tile.
                        (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                                tileFrameP, spriteWorldP->backFrameP,
                                &srcRect, &dstRect, 0, tileID,
                                spriteWorldP->curTileImage[tileID], tileHasMask );
                                                                
                            // Set new tile value in tilingCache
                        if (tileClipped)
                        {
                            spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
                        }
                        else
                        {
                            spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] =
                                spriteWorldP->curTileImage[tileID];
                        }
                    }
                }
            }
            else
            {
                    // Since no tile is here, this spot in the tiling Cache should be set to -1
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;

                    // Cause assertion failure if tileID is < 0 and there is no extraBackFrameP
                SW_ASSERT(spriteWorldP->extraBackFrameP != NULL);
            }
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWDrawTileLayersInRect - draws all tile layers in updateRect of backFrame
//---------------------------------------------------------------------------------------

void SWDrawTileLayersInRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    SWBoolean       optimizingOn)
{
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       tileLayer, startPixelCol;
    SWRect      srcRect, dstRect;
    FramePtr    tileFrameP = NULL;
//  SWRect*     visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect*     backRectP = &spriteWorldP->backRect;
    SWRect      updateRect = *updateRectP;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
    SWBoolean   tileHasMask;

    SW_UNUSED(optimizingOn);    // This is used in our sister function, SWDrawTilesInRect

    SW_UNUSED(optimizingOn);    // This is used in our sister function, SWDrawTilesInRect

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    //SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);

        // Convert pixel row and col into tile row and col
    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    startPixelCol = startCol * tileWidth;

    row = startRow * tileHeight;
    for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
    {
        col = startPixelCol;
        for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
        {
            if (spriteWorldP->tileLayerArray[0] != NULL)
            {
                SW_ASSERT(spriteWorldP->tileLayerArray[0]->isLocked);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[0]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[0]->numCols);
            }

            dstRect.bottom = row + tileHeight;
            dstRect.right = col + tileWidth;

                // Clip tile dstRect with updateRect //
            if (row < updateRect.top)
                dstRect.top = updateRect.top;
            else
                dstRect.top = row;

            if (col < updateRect.left)
                dstRect.left = updateRect.left;
            else
                dstRect.left = col;

            if (dstRect.bottom > updateRect.bottom)
                dstRect.bottom = updateRect.bottom;

            if (dstRect.right > updateRect.right)
                dstRect.right = updateRect.right;

            if (spriteWorldP->tileLayerArray[0] == NULL)
                tileID = -1;
            else
                tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

            if (tileID >= 0)
            {
                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL);
                SW_ASSERT(tileFrameP->isFrameLocked);

                    // Determine whether the tile has a mask with background showing through
                tileHasMask = (tileFrameP->maskType != kNoMask);
            }
            else
                tileHasMask = false;

                // Copy a piece from the extraBackFrameP only if there is no tile, or the
                // tile has a mask with background showing through the unmasked part.
            if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
            {
                    // There is no tile in this spot, or there is a masked tile and
                    // there is an extraBackFrameP, so copy a piece from extraBackFrameP
                    // Note: the function below wraps the dstRect for us, and leaves it
                    // that way when it returns, so we don't have to wrap it ourselves.
                SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
            }
            else
            {
                    // Make the tile's dest rect local to the offscreen area
                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                    // Wrap tile to top or bottom of offscreen area
                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                    // Wrap tile to left or right side of offscreen area
                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }
            }


            if (tileID >= 0)
            {
                srcRect = tileFrameP->frameRect;

                    // Clip new srcRect
                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;

                {
                    int useMask;

                    useMask = (tileHasMask && spriteWorldP->extraBackFrameP != NULL);

                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->backFrameP,
                            &srcRect, &dstRect, 0, tileID,
                            spriteWorldP->curTileImage[tileID], useMask );
                }
            }


                // Draw tiles in higher layers
            for (tileLayer = 1; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
            {
                if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                    continue;

                SW_ASSERT(spriteWorldP->tileLayerArray[tileLayer]->isLocked);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols);

                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
                if (tileID < 0)
                    continue;

                SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL);
                SW_ASSERT(tileFrameP->isFrameLocked);

                srcRect = tileFrameP->frameRect;

                    // Clip new srcRect
                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;

                {
                    int useMask;

                    useMask = (tileFrameP->maskType != kNoMask);

                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->backFrameP,
                            &srcRect, &dstRect, tileLayer, tileID,
                            spriteWorldP->curTileImage[tileID], useMask );
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------
//  SWDrawTileLayersInWorkArea - draws all tile layers in the work area directly
//---------------------------------------------------------------------------------------

void SWDrawTileLayersInWorkArea(
    SpriteWorldPtr  spriteWorldP)
{
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       tileLayer;
    SWRect      srcRect, dstRect;
    FramePtr    tileFrameP = NULL;
    SWRect      updateRect;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
    short       hOffset, vOffset;
    int     tileHasMask;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL);

    updateRect.top = spriteWorldP->backRect.top + spriteWorldP->visScrollRect.top;
    updateRect.left = spriteWorldP->backRect.left + spriteWorldP->visScrollRect.left;
    updateRect.bottom = spriteWorldP->backRect.bottom + spriteWorldP->visScrollRect.top;
    updateRect.right = spriteWorldP->backRect.right + spriteWorldP->visScrollRect.left;

        // Convert pixel row and col into tile row and col
    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    vOffset = - (updateRect.top % tileHeight);
    hOffset = - (updateRect.left % tileWidth);

    row = vOffset;
    for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
    {
        col = hOffset;
        for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
        {
            if (spriteWorldP->tileLayerArray[0] != NULL)
            {
                SW_ASSERT(spriteWorldP->tileLayerArray[0]->isLocked);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[0]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[0]->numCols);
            }

            dstRect.top = row;
            dstRect.left = col;
            dstRect.bottom = row + tileHeight;
            dstRect.right = col + tileWidth;

            if (spriteWorldP->tileLayerArray[0] == NULL)
                tileID = -1;
            else
                tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

            if (tileID >= 0)
            {
                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL);
                SW_ASSERT(tileFrameP->isFrameLocked);

                    // Determine whether the tile has a mask with background showing through
                tileHasMask = (tileFrameP->maskType != kNoMask);
            }
            else
                tileHasMask = false;

                // Copy a piece from the extraBackFrameP only if there is no tile, or the
                // tile has a mask with background showing through the unmasked part.
            if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
            {
                    // There is no tile in this spot, or there is a masked tile and
                    // there is an extraBackFrameP, so copy a piece from extraBackFrameP
                    // Note: the function below wraps the dstRect for us, and leaves it
                    // that way when it returns, so we don't have to wrap it ourselves.
                SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
            }

            if (tileID >= 0)
            {
                {
                    int useMask;

                    useMask = (tileHasMask && spriteWorldP->extraBackFrameP != NULL);

                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->backFrameP,
                            &srcRect, &dstRect, 0, tileID,
                            spriteWorldP->curTileImage[tileID], useMask );
                }
            }


                // Draw tiles in higher layers
            for (tileLayer = 1; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
            {
                if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                    continue;

                SW_ASSERT(spriteWorldP->tileLayerArray[tileLayer]->isLocked);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols);

                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
                if (tileID < 0)
                    continue;

                SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL);
                SW_ASSERT(tileFrameP->isFrameLocked);

                srcRect = tileFrameP->frameRect;

                {
                    int useMask;

                    useMask = (tileFrameP->maskType != kNoMask);

                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->backFrameP,
                            &srcRect, &dstRect, tileLayer, tileID,
                            spriteWorldP->curTileImage[tileID], useMask );
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------
//  SWDrawTilesAboveSprite - draw tiles over sprite using the tiles' masks.
//  Assumes that updateRect fits within the bounds of the tileMap.
//---------------------------------------------------------------------------------------

void SWDrawTilesAboveSprite(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    short           startLayer)
{
    SWRect*     backRectP = &spriteWorldP->backRect;
    SWRect      updateRect = *updateRectP;
    short       tileLayer;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       startPixelRow, startPixelCol;
    SWRect      srcRect, dstRect;
    FramePtr    tileFrameP;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(startLayer >= 0);

        // Convert pixel row and col into tile row and col
    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    startPixelRow = startRow * tileHeight;
    startPixelCol = startCol * tileWidth;

    for (tileLayer = startLayer; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
            continue;

        row = startPixelRow;
        for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
        {
            col = startPixelCol;
            for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
            {
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols);

                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
                if (tileID < 0)
                    continue;
                SW_ASSERT(tileID < spriteWorldP->maxNumTiles);

                tileID = spriteWorldP->curTileImage[tileID];
                SW_ASSERT(tileID >= 0 && tileID < spriteWorldP->maxNumTiles);

                tileFrameP = spriteWorldP->tileFrameArray[tileID];
                                /*
                if (!tileFrameP->tileMaskIsSolid && spriteWorldP->tilePartialFrameArray[tileID] != NULL)
                    tileFrameP = spriteWorldP->tilePartialFrameArray[tileID];
                                */
                SW_ASSERT(tileFrameP != NULL);
                                
                                
                    // Skip tiles in the bottom layer that have no mask, and therefore
                    // aren't above the sprites, as long as no extraBackFrameP is installed.
                if (tileLayer == 0 && spriteWorldP->extraBackFrameP == NULL && 
                                    tileFrameP->maskType == kNoMask /*&&
                    tileFrameP->maskPort == NULL && tileFrameP->maskRgn == NULL &&
                    tileFrameP->tileMaskIsSolid == false*/)
                {
                    continue;
                }
                                

                srcRect = tileFrameP->frameRect;
                dstRect.bottom = row + tileHeight;
                dstRect.right = col + tileWidth;

                    // Clip tile dstRect with updateRect //
                if (row < updateRect.top)
                {
                    dstRect.top = updateRect.top;
                    srcRect.top += updateRect.top - row;
                }
                else
                    dstRect.top = row;

                if (col < updateRect.left)
                {
                    dstRect.left = updateRect.left;
                    srcRect.left += updateRect.left - col;
                }
                else
                    dstRect.left = col;


                if (dstRect.bottom > updateRect.bottom)
                {
                    srcRect.bottom -= dstRect.bottom - updateRect.bottom;
                    dstRect.bottom = updateRect.bottom;
                }

                if (dstRect.right > updateRect.right)
                {
                    srcRect.right -= dstRect.right - updateRect.right;
                    dstRect.right = updateRect.right;
                }

                    // Make the tile's dest rect local to the offscreen area
                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                    // Wrap tile to top or bottom of offscreen area
                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                    // Wrap tile to left or right side of offscreen area
                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }

                if (tileLayer == 0 && spriteWorldP->extraBackFrameP == NULL)
                {
                        // The tile is in the bottom layer
                    //if (tileFrameP->tileMaskIsSolid)
                    //{
                            // Draw the tile without using a mask
                        (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                            spriteWorldP->workFrameP, &srcRect, &dstRect);
                    //}
                    //else
                    //{
                            // Draw the masked part of the tile
                    //  (*spriteWorldP->partialMaskDrawProc)(tileFrameP,
                    //          spriteWorldP->workFrameP, &srcRect, &dstRect);
                    //}
                }
                else
                {
                    int useMask;

                    useMask = (tileFrameP->maskType != kNoMask);

                        // Call the function that draws this tile.
                    (*spriteWorldP->customTileDrawProc)(spriteWorldP,
                            tileFrameP, spriteWorldP->workFrameP,
                            &srcRect, &dstRect, tileLayer, tileID,
                            spriteWorldP->curTileImage[tileID], useMask );
                }
            }
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWResetTilingCache
//---------------------------------------------------------------------------------------

void SWResetTilingCache(
    SpriteWorldPtr  spriteWorldP)
{
    short   row, col;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(spriteWorldP->tilingIsInitialized);

        // Set all elements to -1 (indicating that each tile needs to be drawn)
    for (row = 0; row < spriteWorldP->numTilingCacheRows; row++)
    {
        for (col = 0; col < spriteWorldP->numTilingCacheCols; col++)
        {
            spriteWorldP->tilingCache[row][col] = -1;
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWAddChangedRect - used by SWDrawTile
//---------------------------------------------------------------------------------------

void SWAddChangedRect(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *changedRectP)
{
    short   index;
    SWRect  *changedTileP;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(changedRectP->top >= 0 && changedRectP->left >= 0 &&
        changedRectP->right > changedRectP->left && changedRectP->bottom > changedRectP->top);

    changedTileP = spriteWorldP->changedTiles;
    for ( index = 0; index < spriteWorldP->numTilesChanged; index++, changedTileP++ )
    {
            // check for changedRectP entirely contained by changedTileP
        if ( changedTileP->left <= changedRectP->left &&
             changedTileP->top <= changedRectP->top &&
             changedTileP->right >= changedRectP->right &&
             changedTileP->bottom >= changedRectP->bottom )
        {
            return;
        }
    }

    changedTileP = spriteWorldP->changedTiles;
    for ( index = 0; index < spriteWorldP->numTilesChanged; index++, changedTileP++ )
    {
            // check for changedRectP horizontally adjacent to changedTileP
        if ( changedTileP->top == changedRectP->top &&
             changedTileP->bottom == changedRectP->bottom )
        {
            if ( (changedRectP->left <= changedTileP->left &&       // changedRectP is to the left of changedTileP
                 changedRectP->right >= changedTileP->left) ||      // or
                 (changedRectP->left <= changedTileP->right &&      // changedRectP is to the right of changedTileP
                 changedRectP->right >= changedTileP->right) )
            {
                changedTileP->left = SW_MIN( changedTileP->left, changedRectP->left );
                changedTileP->right = SW_MAX( changedTileP->right, changedRectP->right );
                return;
            }
        }

            // check for changedRectP vertically adjacent to changedTileP
        if ( changedTileP->left == changedRectP->left &&
             changedTileP->right == changedRectP->right )
        {
            if ( (changedRectP->top <= changedTileP->top &&             // changedRectP is above changedTileP
                 changedRectP->bottom >= changedTileP->top) ||      // or
                 (changedRectP->top <= changedTileP->bottom &&      // changedRectP is below changedTileP
                 changedRectP->bottom >= changedTileP->bottom) )
            {
                changedTileP->top = SW_MIN( changedTileP->top, changedRectP->top );
                changedTileP->bottom = SW_MAX( changedTileP->bottom, changedRectP->bottom );
                return;
            }
        }
    }

    if (spriteWorldP->numTilesChanged < spriteWorldP->changedTilesArraySize)
    {
        spriteWorldP->changedTiles[spriteWorldP->numTilesChanged++] = *changedRectP;
    }
    else
    {
            // This shouldn't ever happen, but if it does, we'll trip our Assert routine.
        SW_ASSERT(0);
        SWFlagRectAsChanged(spriteWorldP, changedRectP);
    }
}


//---------------------------------------------------------------------------------------
//  SWChangeTileImage
//---------------------------------------------------------------------------------------

void SWChangeTileImage(
    SpriteWorldPtr  spriteWorldP,
    short           tileID,
    short           newImage)
{
    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(tileID < spriteWorldP->maxNumTiles);
    SW_ASSERT(newImage < spriteWorldP->maxNumTiles);

        // Set the current image
    spriteWorldP->curTileImage[tileID] = newImage;

        // Update the tile image on screen
    SWUpdateTileOnScreen(spriteWorldP, tileID);
}


//---------------------------------------------------------------------------------------
//  SWUpdateTileOnScreen - render new tile image in offscreen areas
//---------------------------------------------------------------------------------------

void SWUpdateTileOnScreen(
    SpriteWorldPtr  spriteWorldP,
    short           tileID)
{
    short       tileRow, tileCol, tileLayer;
    short       startRow, startCol, stopRow, stopCol;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);

        // Convert pixel row and col into tile row and col
    startRow = spriteWorldP->visScrollRect.top / spriteWorldP->tileHeight;
    startCol = spriteWorldP->visScrollRect.left / spriteWorldP->tileWidth;
    stopRow = (spriteWorldP->visScrollRect.bottom-1) / spriteWorldP->tileHeight;
    stopCol = (spriteWorldP->visScrollRect.right-1) / spriteWorldP->tileWidth;

    for (tileLayer = 0; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
            continue;

        for (tileRow = startRow; tileRow <= stopRow; tileRow++)
        {
            for (tileCol = startCol; tileCol <= stopCol; tileCol++)
            {
                if (tileID == spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol])
                    SWDrawTile(spriteWorldP, tileLayer, tileRow, tileCol, tileID);
            }
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWUpdateTilesOnScreenFast - updates a *range* of tileIDs. This makes it much faster
//  than SWUpdateTileOnScreen if there are a number of tileLayers, and a range of tiles
//  that all need updating.
//---------------------------------------------------------------------------------------

void SWUpdateTilesOnScreenFast(
    SpriteWorldPtr  spriteWorldP,
    short           firstTileID,
    short           lastTileID)
{
    short       tileRow, tileCol, tileLayer;
    short       startRow, startCol, stopRow, stopCol, tileID;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);

        // Convert pixel row and col into tile row and col
    startRow = spriteWorldP->visScrollRect.top / spriteWorldP->tileHeight;
    startCol = spriteWorldP->visScrollRect.left / spriteWorldP->tileWidth;
    stopRow = (spriteWorldP->visScrollRect.bottom-1) / spriteWorldP->tileHeight;
    stopCol = (spriteWorldP->visScrollRect.right-1) / spriteWorldP->tileWidth;

    for (tileLayer = 0; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
            continue;

        for (tileRow = startRow; tileRow <= stopRow; tileRow++)
        {
            for (tileCol = startCol; tileCol <= stopCol; tileCol++)
            {
                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];

                if (tileID >= firstTileID && tileID <= lastTileID)
                    SWDrawTile(spriteWorldP, tileLayer, tileRow, tileCol, tileID);
            }
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWResetCurrentTileImages
//---------------------------------------------------------------------------------------

void SWResetCurrentTileImages( SpriteWorldPtr spriteWorldP )
{
    short tileIndex;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);

    if (spriteWorldP->tilingIsInitialized)
    {
            for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
                spriteWorldP->curTileImage[tileIndex] = tileIndex;
    }
}


//---------------------------------------------------------------------------------------
//   SWReturnTileUnderPixel
//---------------------------------------------------------------------------------------

short SWReturnTileUnderPixel(
    SpriteWorldPtr  spriteWorldP,
    short   tileLayer,
    short   pixelCol,
    short   pixelRow)
{
    short   row, col;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(tileLayer < kSWNumTileLayers);

    if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
        return -1;  // No tile here, since there is no tileMap!

    row = pixelRow / spriteWorldP->tileHeight;
    col = pixelCol / spriteWorldP->tileWidth;

    if (row < 0 || row >= spriteWorldP->tileLayerArray[tileLayer]->numRows ||
        col < 0 || col >= spriteWorldP->tileLayerArray[tileLayer]->numCols )
    {
        return -1;  // Pixel location is outside TileMap bounds!
    }
    else
    {
        return spriteWorldP->tileLayerArray[tileLayer]->tileMap[row][col];
    }
}


//---------------------------------------------------------------------------------------
//   SWCheckSpriteWithTiles
//---------------------------------------------------------------------------------------

SWBoolean SWCheckSpriteWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       srcSpriteP,
    SWTileSearchType searchType,
    SWRect          *insetRectP,
    short           startTileLayer,
    short           endTileLayer,
    short           firstTileID,
    short           lastTileID,
    SWBoolean       fixPosition)
{
    short           row, col, startRow, stopRow, startCol, stopCol;
    TileMapPtr      tileMap;
    SWRect          oldFrameRect, destFrameRect;
    SWBoolean       foundTile = false;
    SWBoolean       rowLoopTest, colLoopTest;
    short           temp, tileID, tileLayer, rowIncrement, colIncrement;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized);
    SW_ASSERT(startTileLayer >= 0 && endTileLayer < kSWNumTileLayers);
    SW_ASSERT(srcSpriteP != NULL);

    row = -1;
    col = -1;

    oldFrameRect = srcSpriteP->oldFrameRect;
    destFrameRect = srcSpriteP->destFrameRect;

    if (insetRectP != NULL)
    {
        oldFrameRect.left += insetRectP->left;
        oldFrameRect.top += insetRectP->top;
        oldFrameRect.right -= insetRectP->right;
        oldFrameRect.bottom -= insetRectP->bottom;

        destFrameRect.left += insetRectP->left;
        destFrameRect.top += insetRectP->top;
        destFrameRect.right -= insetRectP->right;
        destFrameRect.bottom -= insetRectP->bottom;
    }

        // We must do this so sprites hanging off the top or left side of the TileMap
        // are still handled correctly. (The conversion from pixel to row won't work
        // correctly for negative numbers, so we "fix" the problem here.)
    if (oldFrameRect.top < 0)
        oldFrameRect.top -= spriteWorldP->tileHeight;
    if (oldFrameRect.bottom <= 0)
        oldFrameRect.bottom -= spriteWorldP->tileHeight;

    if (oldFrameRect.left <= 0)
        oldFrameRect.left -= spriteWorldP->tileWidth;
    if (oldFrameRect.right <= 0)
        oldFrameRect.right -= spriteWorldP->tileWidth;

    if (destFrameRect.left < 0)
        destFrameRect.left -= spriteWorldP->tileWidth;
    if (destFrameRect.right <= 0)
        destFrameRect.right -= spriteWorldP->tileWidth;

    if (destFrameRect.top < 0)
        destFrameRect.top -= spriteWorldP->tileHeight;
    if (destFrameRect.bottom <= 0)
        destFrameRect.bottom -= spriteWorldP->tileHeight;


        // startRow = the tile the oldFrameRect.side was about to run into.
        // stopRow = the tile the destFrameRect.side is currently in.
        // Function returns early if the sprite didn't move over
        // a tile's bounds since last frame.

    if (searchType == kSWTopSide)
    {
        startRow = (oldFrameRect.top / spriteWorldP->tileHeight);
        stopRow = destFrameRect.top / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        if (fixPosition)
            startRow--;     // Check tile just above startRow
        if (stopRow > startRow)
            return false;
    }
    else if (searchType == kSWBottomSide)
    {
        startRow = ((oldFrameRect.bottom-1) / spriteWorldP->tileHeight);
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        if (fixPosition)    // Check tile just below startRow
            startRow++;
        if (stopRow < startRow)
            return false;
    }
    else if (searchType == kSWRightSide)
    {
        startCol = ((oldFrameRect.right-1) / spriteWorldP->tileWidth);
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        if (fixPosition)    // Check tile just to the right of startCol
            startCol++;
        if (stopCol < startCol)
            return false;
    }
    else if (searchType == kSWLeftSide)
    {
        startCol = oldFrameRect.left / spriteWorldP->tileWidth;
        stopCol = destFrameRect.left / spriteWorldP->tileWidth;
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        if (fixPosition)                // Check tile just to the left of startCol
            startCol--;
        if (stopCol > startCol)
            return false;
    }
    else    // searchType == kSWEntireSprite
    {
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
    }

    if (startRow <= stopRow)
        rowIncrement = 1;
    else
        rowIncrement = -1;

    if (startCol <= stopCol)
        colIncrement = 1;
    else
        colIncrement = -1;


        // Find the first tileLayer that's not NULL
    for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] != NULL)
            break;
    }


        // Make sure things are within bounds (in case Sprite is hanging off edge of TileMap)
    if (rowIncrement > 0)
    {
        if (stopRow < 0)
            return false;
        else if (stopRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            stopRow = spriteWorldP->tileLayerArray[tileLayer]->numRows-1;

        if (startRow < 0)
            startRow = 0;
        else if (startRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            return false;
    }
    else    // rowIncrement < 0
    {
        if (startRow < 0)
            return false;
        else if (startRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            startRow = spriteWorldP->tileLayerArray[tileLayer]->numRows-1;

        if (stopRow < 0)
            stopRow = 0;
        else if (stopRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            return false;
    }

    if (colIncrement > 0)
    {
        if (stopCol < 0)
            return false;
        else if (stopCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            stopCol = spriteWorldP->tileLayerArray[tileLayer]->numCols-1;

        if (startCol < 0)
            startCol = 0;
        else if (startCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            return false;
    }
    else    // colIncrement < 0
    {
        if (startCol < 0)
            return false;
        else if (startCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            startCol = spriteWorldP->tileLayerArray[tileLayer]->numCols-1;

        if (stopCol < 0)
            stopCol = 0;
        else if (stopCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            return false;
    }



        // Look for the tiles in each layer. We have two separate loops: one that scans
        // through each row, then each col, and one that scans through each col, then each
        // row. You must use the correct type depending on which direction the sprite is moving.
        // (horizontally or vertically) for things to work correctly.

    if (searchType == kSWTopSide || searchType == kSWBottomSide || searchType == kSWEntireSprite)
    {
        for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            tileMap = spriteWorldP->tileLayerArray[tileLayer]->tileMap;

                // Scan through all cols in a row before moving to next row
            rowLoopTest = true;
            for (row = startRow; rowLoopTest; row += rowIncrement)
            {
                rowLoopTest = row != stopRow;

                colLoopTest = true;
                for (col = startCol; colLoopTest; col += colIncrement)
                {
                    colLoopTest = col != stopCol;
                    tileID = tileMap[row][col];
                    if (tileID >= firstTileID && tileID <= lastTileID)
                    {
                        foundTile = true;
                        goto exit;
                    }
                }
            }
        }
    }
    else
    {
        for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            tileMap = spriteWorldP->tileLayerArray[tileLayer]->tileMap;

                // Scan through all rows in a col before moving to next col
            colLoopTest = true;
            for (col = startCol; colLoopTest; col += colIncrement)
            {
                colLoopTest = col != stopCol;

                rowLoopTest = true;
                for (row = startRow; rowLoopTest; row += rowIncrement)
                {
                    rowLoopTest = row != stopRow;
                    tileID = tileMap[row][col];
                    if (tileID >= firstTileID && tileID <= lastTileID)
                    {
                        foundTile = true;
                        goto exit;
                    }
                }
            }
        }
    }

exit:


    if (foundTile && fixPosition)
    {
        if (searchType == kSWTopSide)
        {
                // (tile's bottom side) - (Sprite's top side)
            temp = (row+1) * spriteWorldP->tileHeight - destFrameRect.top;
            SWOffsetSprite(srcSpriteP, 0, temp);
        }
        else if (searchType == kSWBottomSide)
        {
            temp = destFrameRect.bottom - row * spriteWorldP->tileHeight;
            SWOffsetSprite(srcSpriteP, 0, -temp);
        }
        else if (searchType == kSWRightSide)
        {
            temp = destFrameRect.right - col * spriteWorldP->tileWidth;
            SWOffsetSprite(srcSpriteP, -temp, 0);
        }
        else if (searchType == kSWLeftSide)
        {
            temp = (col+1) * spriteWorldP->tileWidth - destFrameRect.left;
            SWOffsetSprite(srcSpriteP, temp, 0);
        }
    }

    return foundTile;
}


//---------------------------------------------------------------------------------------
//  SWWrapRectToWorkArea - I think this is identical to SWEraseWrappedSprite, and is here
//  simply so Scrolling.c isn't required during compiles.
//---------------------------------------------------------------------------------------

void SWWrapRectToWorkArea(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         destRectP)
{
    SWRect      destRect = *destRectP;
    SWRect      tempDestRect;
    FramePtr    srcFrameP = spriteWorldP->backFrameP;
    FramePtr    dstFrameP = spriteWorldP->workFrameP;

    SW_ASSERT(spriteWorldP != NULL);
//  SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);

        // Make destRect local to the offscreen area
    destRect.top -= spriteWorldP->vertScrollRectOffset;
    destRect.bottom -= spriteWorldP->vertScrollRectOffset;
    destRect.left -= spriteWorldP->horizScrollRectOffset;
    destRect.right -= spriteWorldP->horizScrollRectOffset;


        // Draw main image
    (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP, &destRect, &destRect);

    if (!gSWUseOpenGL)
    {

        // Wrap to top //
    if (destRect.bottom > dstFrameP->frameRect.bottom)
    {
        tempDestRect.top = destRect.top - dstFrameP->frameRect.bottom;
        tempDestRect.bottom = destRect.bottom - dstFrameP->frameRect.bottom;
        tempDestRect.left = destRect.left;
        tempDestRect.right = destRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);

            // Wrap to upper left or right corner //
        if (destRect.right > dstFrameP->frameRect.right)
        {
            tempDestRect.left -= dstFrameP->frameRect.right;
            tempDestRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
        else if (destRect.left < dstFrameP->frameRect.left)
        {
            tempDestRect.left += dstFrameP->frameRect.right;
            tempDestRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
    }

            // Wrap to left or right side //
    if (destRect.right > dstFrameP->frameRect.right)
    {
        tempDestRect.top = destRect.top;
        tempDestRect.bottom = destRect.bottom;
        tempDestRect.left = destRect.left - dstFrameP->frameRect.right;
        tempDestRect.right = destRect.right - dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);
    }
    else if (destRect.left < dstFrameP->frameRect.left)
    {
        tempDestRect.top = destRect.top;
        tempDestRect.bottom = destRect.bottom;
        tempDestRect.left = destRect.left + dstFrameP->frameRect.right;
        tempDestRect.right = destRect.right + dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);
    }


            // Wrap to bottom //
    if (destRect.top < dstFrameP->frameRect.top)
    {
        tempDestRect.top = destRect.top + dstFrameP->frameRect.bottom;
        tempDestRect.bottom = destRect.bottom + dstFrameP->frameRect.bottom;
        tempDestRect.left = destRect.left;
        tempDestRect.right = destRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);

            // Wrap to lower left or right corner //
        if (destRect.right > dstFrameP->frameRect.right)
        {
            tempDestRect.left -= dstFrameP->frameRect.right;
            tempDestRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
        else if (destRect.left < dstFrameP->frameRect.left)
        {
            tempDestRect.left += dstFrameP->frameRect.right;
            tempDestRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
    }
    
    } // OpenGL
}


//---------------------------------------------------------------------------------------
//  SWWrapRectFromExtraBackFrame - similar to SWWrapRectToScreen, just modified so the
//  srcFramePtr is the extraBackFramePtr, the dstFrameP is the backFrameP, and the src
//  and dst rects are what they should be. Note: the dstRect you pass to this function is
//  the tile's rect *before* it's made local to the offscreen area and wrapped. It is also
//  important that the actual dstRectP is modified, not a copy, since the callers expect this.
//---------------------------------------------------------------------------------------

void SWWrapRectFromExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *dstRectP)
{
    FramePtr    srcFrameP = spriteWorldP->extraBackFrameP;
    FramePtr    dstFrameP = spriteWorldP->backFrameP;
    SWRect      srcRect, tempSrcRect, tempDstRect;
    short       topClip=0, rightClip=0, bottomClip=0, leftClip=0;
    short       vertBackRectOffset, horizBackRectOffset;

    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(srcFrameP->isFrameLocked);
    SW_ASSERT(dstFrameP->isFrameLocked);

        // The code below is ripped from SWCalculateOffscreenScrollRect
        // It is modified to do a similar function for the extraBackFrameP.
    vertBackRectOffset = spriteWorldP->extraBackFrameP->frameRect.bottom *
        (dstRectP->top / spriteWorldP->extraBackFrameP->frameRect.bottom);

    horizBackRectOffset = spriteWorldP->extraBackFrameP->frameRect.right *
        (dstRectP->left / spriteWorldP->extraBackFrameP->frameRect.right);

    srcRect.top = dstRectP->top - vertBackRectOffset;
    srcRect.bottom = dstRectP->bottom - vertBackRectOffset;
    srcRect.left = dstRectP->left - horizBackRectOffset;
    srcRect.right = dstRectP->right - horizBackRectOffset;

    // We must do the dstRect calculation below *after* the code above!
    if (!gSWUseOpenGL)
    {

        // Make the tile's dest rect local to the offscreen area
    dstRectP->top -= spriteWorldP->vertScrollRectOffset;
    dstRectP->bottom -= spriteWorldP->vertScrollRectOffset;
    dstRectP->left -= spriteWorldP->horizScrollRectOffset;
    dstRectP->right -= spriteWorldP->horizScrollRectOffset;

        // Wrap dstRect to top or bottom of offscreen area
    if (dstRectP->bottom > dstFrameP->frameRect.bottom)
    {
        dstRectP->top -= dstFrameP->frameRect.bottom;
        dstRectP->bottom -= dstFrameP->frameRect.bottom;
    }
    else if (dstRectP->top < dstFrameP->frameRect.top)
    {
        dstRectP->top += dstFrameP->frameRect.bottom;
        dstRectP->bottom += dstFrameP->frameRect.bottom;
    }

        // Wrap dstRect to left or right side of offscreen area
    if (dstRectP->right > dstFrameP->frameRect.right)
    {
        dstRectP->left -= dstFrameP->frameRect.right;
        dstRectP->right -= dstFrameP->frameRect.right;
    }
    else if (dstRectP->left < dstFrameP->frameRect.left)
    {
        dstRectP->left += dstFrameP->frameRect.right;
        dstRectP->right += dstFrameP->frameRect.right;
    }


        // Clip the source rect, and save what we clipped for wrapping later //

        // clip off the top
    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

        // clip off the bottom
    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

        // clip off the left
    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

        // clip off the right
    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }

    }
    else // OpenGL
    {
        topClip = bottomClip = leftClip = rightClip = 0;
    }

                    // Here we do the wrapping and drawing //

        // Draw top section //

    if (topClip)
    {
                // Calculate top piece //

            // Wrap source rect to bottom side
        tempSrcRect.right = srcRect.right;                  // Copy clipped source rect
        tempSrcRect.left = srcRect.left;
        tempSrcRect.bottom = srcFrameP->frameRect.bottom;
        tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;

            // Position dest rect at top side
        tempDstRect.top = dstRectP->top;
        tempDstRect.bottom = dstRectP->top + topClip;
        tempDstRect.left = dstRectP->left + leftClip;
        tempDstRect.right = dstRectP->right - rightClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);


        if (leftClip)   // Calculate top-left piece
        {
                // Wrap source rect to lower-right corner
            tempSrcRect.bottom = srcFrameP->frameRect.bottom;
            tempSrcRect.right = srcFrameP->frameRect.right;
            tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;
            tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at top-left corner
            tempDstRect.left = dstRectP->left;
            tempDstRect.top = dstRectP->top;
            tempDstRect.right = dstRectP->left + leftClip;
            tempDstRect.bottom = dstRectP->top + topClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
        else if (rightClip)     // Calculate top-right piece
        {
                // Wrap source rect to lower-left corner
            tempSrcRect.bottom = srcFrameP->frameRect.bottom;
            tempSrcRect.left = srcFrameP->frameRect.left;
            tempSrcRect.right = srcFrameP->frameRect.left + rightClip;
            tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;

                // Position dest rect at top-right corner
            tempDstRect.top = dstRectP->top;
            tempDstRect.right = dstRectP->right;
            tempDstRect.bottom = dstRectP->top + topClip;
            tempDstRect.left = dstRectP->right - rightClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
    }


            // Draw middle section //

        // Calculate main middle piece (not wrapped)
    tempDstRect.left = dstRectP->left + leftClip;
    tempDstRect.top = dstRectP->top + topClip;
    tempDstRect.right = dstRectP->right - rightClip;
    tempDstRect.bottom = dstRectP->bottom - bottomClip;

    (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &tempDstRect);


    if (leftClip)   // Draw left piece
    {
            // Wrap source rect to right side
        tempSrcRect.top = srcRect.top;              // Copy clipped source rect
        tempSrcRect.bottom = srcRect.bottom;
        tempSrcRect.right = srcFrameP->frameRect.right;
        tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

            // Position dest rect at left side
        tempDstRect.left = dstRectP->left;
        tempDstRect.right = dstRectP->left + leftClip;
        tempDstRect.top = dstRectP->top + topClip;
        tempDstRect.bottom = dstRectP->bottom - bottomClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
    }
    else if (rightClip)     // Draw right piece
    {
            // Wrap source rect to left side
        tempSrcRect.top = srcRect.top;              // Copy clipped source rect
        tempSrcRect.bottom = srcRect.bottom;
        tempSrcRect.left = srcFrameP->frameRect.left;
        tempSrcRect.right = srcFrameP->frameRect.left + rightClip;

            // Position dest rect at right side
        tempDstRect.right = dstRectP->right;
        tempDstRect.left = dstRectP->right - rightClip;
        tempDstRect.top = dstRectP->top + topClip;
        tempDstRect.bottom = dstRectP->bottom - bottomClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
    }


        // Draw bottom section //

    if (bottomClip)
    {
            // Calculate bottom piece //

            // Wrap source rect to top side
        tempSrcRect.right = srcRect.right;              // Copy clipped source rect
        tempSrcRect.left = srcRect.left;
        tempSrcRect.top = srcFrameP->frameRect.top;
        tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;

            // Position dest rect at bottom side
        tempDstRect.bottom = dstRectP->bottom;
        tempDstRect.top = dstRectP->bottom - bottomClip;
        tempDstRect.left = dstRectP->left + leftClip;
        tempDstRect.right = dstRectP->right - rightClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);

        if (leftClip)   // Draw bottom-left piece
        {
                // Wrap source rect to upper-right corner
            tempSrcRect.top = srcFrameP->frameRect.top;
            tempSrcRect.right = srcFrameP->frameRect.right;
            tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;
            tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at bottom-left corner
            tempDstRect.bottom = dstRectP->bottom;
            tempDstRect.left = dstRectP->left;
            tempDstRect.top = dstRectP->bottom - bottomClip;
            tempDstRect.right = dstRectP->left + leftClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
        else if (rightClip)     // Draw bottom-right piece
        {
                // Wrap source rect to upper-left corner
            tempSrcRect.top = srcFrameP->frameRect.top;
            tempSrcRect.left = srcFrameP->frameRect.left;
            tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;
            tempSrcRect.right = srcFrameP->frameRect.left + rightClip;

                // Position dest rect at bottom-right corner
            tempDstRect.bottom = dstRectP->bottom;
            tempDstRect.right = dstRectP->right;
            tempDstRect.top = dstRectP->bottom - bottomClip;
            tempDstRect.left = dstRectP->right - rightClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
    }
}


