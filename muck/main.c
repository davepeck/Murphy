///--------------------------------------------------------------------------------------
// Scrolling Demo.c
//
// By Vern Jensen. Created in August of 1995, Updated 7/5/2003 for SWX
///--------------------------------------------------------------------------------------

//#include <SWFPSReport.h>
#include <SWIncludes.h>
#include "SpriteWorldUtils.h"
//#include <SWGameUtils.h>
//#include <SWStats.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "main.h"

#define kTileMapRows                100
#define kTileMapCols                150

#define kWindowWidth                320
#define kWindowHeight               480
#define kDepth                  32
#define kFullScreenWindow           true       // Try me full screen!
#define kOpenGL             true

#define kWorldRectInset             0           // Make the SpriteWorld smaller?
//#define   kInterlacedMode         false       // Skips every other line
//#define kSyncToVBL                false       // Sync SpriteWorld to VBL?
#define kMaxFPS                 200     // Set to 0 for unrestricted speed

#define kSpriteMoveDelta            20      // Try 5, 10, 20, or 40
#define kDiamondSpace               8       // How far apart the diamonds are spaced
// (try 1!)

#define kSpriteMoveDistance         80      // How far the sprite can move from
// the center of the screen, in pixels.
// Try making this value higher!

#define kStatsOffset                85      // Distance the numbers are from the right side
// of the screen.

// Number of ticks to wait before changing tile image; 0 = change every frame
#define kDiamondFrameRate           150     // How often diamond tiles change frames
#define kWallFrameRate              1500        // How often wall changes frames

#define kTileWidth              40
#define kTileHeight             40

#define kStartRow               5       // Starting position of sprite
#define kStartCol               6       // in tile col and row

enum tileIDs
{
    kWallTile,
    kLastWallTile,
    kGrassTile,
    kBlackTile,
    kDiamondTile,
    kDiamondTile2,
    kLastDiamondTile,
    
    kTunnelTile1,
    kTunnelTile2,
    kTunnelTile3,
    kTunnelTile4,
    kTunnelTile5,
    kTunnelTile6,
    kWireTile1,
    kWireTile2,
    
    kMaxNumTiles
};



/***********/
/* Globals */
/***********/

SpriteWorldPtr      gSpriteWorldP;
SpriteLayerPtr      gSpriteLayerP, gNonScrollingSpriteLayerP;
TileMapStructPtr    gTileMapStructP, gTileMapStruct2P;
TileMapPtr      gTileMap, gTileMap2;
SpritePtr       gSimpleSpriteP, gStatsSpriteP, gDiamondMeterSpriteP, gMasterStatsSpriteP;
SWRect          gScreenMidRect;

long            gNumDiamondsInMap = 0;      // Number of diamonds in the TileMap
long            gNumDiamonds = 0;       // Number of diamonds sprite has collected

struct moveKeys     // Keeps track of which keys are up and which are down
{
    int up;
    int right;
    int down;
    int left;
	
} gKeys;


///--------------------------------------------------------------------------------------
// Main
///--------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	CreateSpriteWorld();
	SetUpTiling();
	CreateBallSprite();
	CreateDiamondMeterSprite();
	CreateStatsNumberSprite();
	
	SetUpAnimation();
	RunAnimation();
	ShutDown();
	
	return 0;
}


///--------------------------------------------------------------------------------------
// CreateSpriteWorld
///--------------------------------------------------------------------------------------

void    CreateSpriteWorld( void )
{
    SWError         err;
    SDL_Color       bg = { 100, 100, 100 };
	
	// Initialize the SpriteWorld package
    err = SWEnterSpriteWorld( true );
    FatalError(err);
    
	// Create the SpriteWorld
    err = SWCreateSpriteWorld( &gSpriteWorldP, kWindowWidth, kWindowHeight, kDepth, kFullScreenWindow, kOpenGL );
    FatalError(err);
    
    SWSetBackgroundColor(gSpriteWorldP, bg);
    SDL_FillRect(
				 gSpriteWorldP->backFrameP->originalSurfaceP, //frameSurfaceP,
				 NULL,
				 SDL_MapRGB(
							gSpriteWorldP->backFrameP->originalSurfaceP->format,
							bg.r, bg.g ,bg.b
							)
				 );
    SWUpdateFrame(gSpriteWorldP->backFrameP);
	
	// Limit the frame rate to a specific speed
    SWSetSpriteWorldMaxFPS(gSpriteWorldP, kMaxFPS);
    
	// Create the sprite layers
    err = SWCreateSpriteLayer(&gSpriteLayerP);
    FatalError(err);
    err = SWCreateSpriteLayer(&gNonScrollingSpriteLayerP);
    FatalError(err);
    
	// Add them to the world
    SWAddSpriteLayer(gSpriteWorldP, gSpriteLayerP);         // Bottom
    SWAddSpriteLayer(gSpriteWorldP, gNonScrollingSpriteLayerP); // Top
}


///--------------------------------------------------------------------------------------
// SetUpTiling
///--------------------------------------------------------------------------------------

const char* GetRawFileName(const char* name, const char* type)
{
	NSBundle *main_bundle = [NSBundle mainBundle];
	NSString *ns_name = [NSString stringWithCString: name encoding:NSASCIIStringEncoding];
	NSString *ns_type = [NSString stringWithCString:type encoding: NSASCIIStringEncoding];
	NSString *path = [main_bundle pathForResource:ns_name ofType:ns_type];	
	const char* path_cstr = [path cStringUsingEncoding:NSASCIIStringEncoding];
	return strdup(path_cstr);
}

void SetUpTiling( void )    
{
    short       row, col;
    SWError     err = kNoError;
	
    err = SWInitTiling(gSpriteWorldP, kTileHeight, kTileWidth, kMaxNumTiles);
    FatalError(err);
    
    err = SWCreateTileMap(&gTileMapStructP, kTileMapRows, kTileMapCols);
    FatalError(err);
    
	err = SWCreateTileMap(&gTileMapStruct2P, kTileMapRows, kTileMapCols);
    FatalError(err);
	
    SWInstallTileMap(gSpriteWorldP, gTileMapStructP, 0);
	SWInstallTileMap(gSpriteWorldP, gTileMapStruct2P, 1);
	
    gTileMap = gTileMapStructP->tileMap;
    gTileMap2 = gTileMapStruct2P->tileMap;
	
    SWSetSpriteLayerUnderTileLayer(gSpriteLayerP, 1);
	
	// Load first set of tiles
	const char *tiles_name = GetRawFileName("tiles", "tga");
	err = SWLoadTilesFromSingleFileXY( 
									  tiles_name,
									  kTileWidth,
									  kTileHeight,
									  1, // horizBorderWidth
									  1, // vertBorderHeight
									  false, // hasOutsideBorder
									  7, // numTiles
									  gSpriteWorldP,
									  kWallTile //startTileID
									  );
    FatalError(err);
    
	// Load masked set of tiles
	const char *tiles2_name = GetRawFileName("tiles2", "tga");
	err = SWLoadTilesFromSingleFileXY( 
									  tiles2_name,
									  kTileWidth,
									  kTileHeight,
									  1, // horizBorderWidth
									  1, // vertBorderHeight
									  false, // hasOutsideBorder
									  7, // numTiles
									  gSpriteWorldP,
									  kTunnelTile1 //startTileID
									  );
    FatalError(err);
	
	// Set up tileMap
    for (row = 0; row < kTileMapRows; row++)
    {
        for (col = 0; col < kTileMapCols; col++)
        {
            if (row == 0 || col == 0 || row == kTileMapRows-1 || col == kTileMapCols-1)
                gTileMap[row][col] = kWallTile;
            //else if (row > kTileMapRows / 2)
            //  gTileMap[row][col] = kWireTile1;
            else if ((row/kDiamondSpace)*kDiamondSpace == row && 
                     (col/kDiamondSpace)*kDiamondSpace == col)
            {
                gTileMap[row][col] = kDiamondTile;
                gNumDiamondsInMap++;
            }
            else
                gTileMap[row][col] = kGrassTile;
			
			
			// Add wires
			if (row-1 > kTileMapRows / 2 && 
				!(row == 0 || col == 0 || row == kTileMapRows-1 || col == kTileMapCols-1))
                gTileMap2[row][col] = kWireTile1;
			else {
				gTileMap2[row][col] = -1;
			}
        }
    }
    
	
	// Add tunnel to tileMap
    row = kTileMapRows / 2;
	
    for (col = 1; col < kTileMapCols-1; col += 2)
    {
        gTileMap[row][col] = kTunnelTile1;
        gTileMap2[row][col+1] = kTunnelTile2;
        gTileMap[row+1][col] = kTunnelTile4;
        gTileMap2[row+1][col+1] = kTunnelTile5;
    }
}


///--------------------------------------------------------------------------------------
// CreateBallSprite
///--------------------------------------------------------------------------------------

void CreateBallSprite( void )
{
    SWError err;
    
	const char *ball_name = GetRawFileName("ball", "tga");	
    err = SWCreateSpriteFromFile( &gSimpleSpriteP, NULL, ball_name, 1 );
    FatalError(err);
	
	// Set up the ball sprite
    SWAddSprite(gSpriteLayerP, gSimpleSpriteP);
    SWSetSpriteMoveProc(gSimpleSpriteP, KeySpriteMoveProc);
    SWSetSpriteLocation(gSimpleSpriteP, kStartCol * kTileWidth, kStartRow * kTileHeight);
    SWSetSpriteMoveDelta(gSimpleSpriteP, 0, 0);        
}


///--------------------------------------------------------------------------------------
// CreateDiamondMeterSprite
///--------------------------------------------------------------------------------------

void CreateDiamondMeterSprite( void )
{
    SWError     err;
    
	// Create the diamond meter sprite
	const char *meter_name = GetRawFileName("meter", "tga");	
	err = SWCreateSpriteFromFile( &gDiamondMeterSpriteP, NULL, meter_name, 1 );
    FatalError(err);
    
    SWAddSprite(gNonScrollingSpriteLayerP, gDiamondMeterSpriteP);
    
    SWSetSpriteMoveProc(gDiamondMeterSpriteP, DiamondMeterSpriteMoveProc);
    SWSetSpriteLocation(gDiamondMeterSpriteP, 10, 10);
    
	// Mark this layer as a non-scrolling layer
    SWSetLayerAsNonScrolling(gNonScrollingSpriteLayerP, true);
}


///--------------------------------------------------------------------------------------
// CreateStatsNumberSprite
///--------------------------------------------------------------------------------------

void    CreateStatsNumberSprite( void )
{
    SWError     err = kNoError;
    
	// Load gMasterStatsSpriteP
	const char *numbers_name = GetRawFileName("numbers", "tga");
	err = SWCreateSpriteFromSingleFileXY( 
										 &gMasterStatsSpriteP,
										 NULL,
										 numbers_name,
										 24,
										 31,
										 0,
										 0,
										 false,
										 10 );
	
    FatalError(err);
	
    SWLockSprite(gMasterStatsSpriteP);
	
	// Create the gNumLivesStatsSpriteP
	/*
    gStatsSpriteP = CreateStatsSpriteClone(gMasterStatsSpriteP, 3, kRightJustify, false);
    err = SetUpStatsSprite(gStatsSpriteP, gNonScrollingSpriteLayerP, SWStdSpriteDrawProc,
						   gSpriteWorldP->screenRect.right - kStatsOffset, 8, 0);
    FatalError(err);
	
    SWSetSpriteMoveProc(gStatsSpriteP, StatsSpriteMoveProc);
	 */
}


///--------------------------------------------------------------------------------------
// SetUpAnimation
///--------------------------------------------------------------------------------------

void SetUpAnimation( void )
{
    SWRect      moveBoundsRect;
    
    SWLockSpriteWorld(gSpriteWorldP);
    
	// Set up data used by the SmoothScrollingWorldMoveProc
    gScreenMidRect = gSimpleSpriteP->curFrameP->frameRect;
    SW_CENTER_RECT( gScreenMidRect, gSpriteWorldP->backRect );
    
    SWSetSpriteWorldMaxFPS(gSpriteWorldP, kMaxFPS);
    
	// movement boundary = size of tileMap
    SW_SET_RECT(moveBoundsRect, 0,0, kTileMapCols * kTileWidth, kTileMapRows * kTileHeight);
    
    SWSetScrollingWorldMoveBounds(gSpriteWorldP, &moveBoundsRect);
    SWSetScrollingWorldMoveProc(gSpriteWorldP, SmoothScrollingWorldMoveProc, gSimpleSpriteP);
    
    SWSetTileChangeProc(gSpriteWorldP, TileChangeProc);
    
	// Move visScrollRect to starting sprite position
    SWMoveVisScrollRect(gSpriteWorldP, 
						SWGetSpriteHorizLoc(gSpriteWorldP->followSpriteP) - gSpriteWorldP->backRect.right/2,
						SWGetSpriteVertLoc(gSpriteWorldP->followSpriteP) - gSpriteWorldP->backRect.bottom/2);
    
    SWDrawTilesInBackground(gSpriteWorldP);
    
	SWUpdateScrollingSpriteWorld(gSpriteWorldP, true);
}


///--------------------------------------------------------------------------------------
//  RunAnimation
///--------------------------------------------------------------------------------------

void RunAnimation( void )
{
    unsigned long then, now, frames = 0;
    SDL_Event event;
	
    //StartTimer();
	
    FatalError( SWStickyError() ); // Make sure no errors got past us during setup
    
	then = SDL_GetTicks();
	
    do
    {
		SWProcessScrollingSpriteWorld(gSpriteWorldP);
		FatalError( SWStickyError() );  // Make sure no errors occurred during a MoveProc, etc.
		SWAnimateScrollingSpriteWorld(gSpriteWorldP);
		
		//SetStatsSpriteNumber(gStatsSpriteP, gNumDiamonds);
		
		if (gSpriteWorldP->frameHasOccurred)
		{
			frames++;
		}
		
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
			break;
		
    } while( ! SDL_GetMouseState( 0, NULL, NULL ) );
    
    now = SDL_GetTicks();
    if ( now > then ) {
        printf("%2.2f frames per second\n",
			   ((double)frames*1000)/(now-then));
    }
}


#pragma mark -
///--------------------------------------------------------------------------------------
//  ShutDown (clean up and dispose of the SpriteWorld)
///--------------------------------------------------------------------------------------

void    ShutDown( void )
{
    SWDisposeTileMap(&gTileMapStructP);
    SWDisposeSprite(&gMasterStatsSpriteP);
	
    SWDisposeSpriteWorld(&gSpriteWorldP);
    SWExitSpriteWorld();
}

///--------------------------------------------------------------------------------------
//  TileChangeProc
///--------------------------------------------------------------------------------------

void TileChangeProc(
					SpriteWorldPtr spriteWorldP)
{
    short           curImage;
    static short    wallDelay = 0, diamondDelay = 0;
    static short    oldTicks = 0;
    short           ticksPassed, ticks;
    
	// Initialize oldTicks the first time this function is called
    if (oldTicks == 0)
        oldTicks = SWGetMilliseconds( spriteWorldP );
    
    ticks = SWGetMilliseconds( spriteWorldP );
    ticksPassed = ticks - oldTicks;     // Number of ticks passed since last call
    oldTicks = ticks;
    
	// kWallTile
    wallDelay += ticksPassed;
    if (wallDelay >= kWallFrameRate)
    {
        curImage = spriteWorldP->curTileImage[kWallTile];
        if (curImage < kLastWallTile)
            curImage++;
        else
            curImage = kWallTile;
        
        SWChangeTileImage(spriteWorldP, kWallTile, curImage);
        wallDelay = 0;
    }
    
    
	// kDiamondTile
    diamondDelay += ticksPassed;
    if (diamondDelay >= kDiamondFrameRate)
    {
        curImage = spriteWorldP->curTileImage[kDiamondTile];
        if (curImage < kLastDiamondTile)
            curImage++;
        else
            curImage = kDiamondTile;
        
        SWChangeTileImage(spriteWorldP, kDiamondTile, curImage);
        diamondDelay = 0;
    }
}

///--------------------------------------------------------------------------------------
//  KeySpriteMoveProc
///--------------------------------------------------------------------------------------

void KeySpriteMoveProc(SpritePtr srcSpriteP)
{        
    short           x,y;
    short           row, col;
    short           rowDelta, colDelta;
    short           tile;
    
    //if (!gSpriteWorldP->pseudoFrameHasFired) return;
    
    UpdateKeys();   // Put the latest key values in the keys structure
    
    x = SWGetSpriteHorizLoc(srcSpriteP);
    y = SWGetSpriteVertLoc(srcSpriteP);
    
    row = y / kTileHeight;
    col = x / kTileWidth;
	
    if ( (row * kTileHeight == y) && (col * kTileWidth == x) )
    {
        tile = gTileMap[(int) row][(int) col];
        
		// Leave black trail behind sprite
        if (tile == kGrassTile || tile == kDiamondTile)
            SWDrawTile(gSpriteWorldP, 0, row, col, kBlackTile);
        else if (tile == kTunnelTile2/* || tile == kTunnelTile5 || tile == kWireTile1*/)
            SWDrawTile(gSpriteWorldP, 0, row, col, tile+1);
		
        if (tile == kDiamondTile)
        {
            gNumDiamonds++;         // Increase number of diamonds sprite has collected
            UpdateDiamondMeter();   // Update meter
        }
        
        rowDelta = 0;
        colDelta = 0;
        
        if (gKeys.left)
            colDelta = -1;
        else if (gKeys.right)
            colDelta = +1;
        else if (gKeys.up)
            rowDelta = -1;
        else if (gKeys.down)
            rowDelta = +1;
        
        tile = gTileMap[(int) (row + rowDelta)][(int) (col + colDelta)];
        
        if (tile != kWallTile && tile != kTunnelTile1 && tile != kTunnelTile4)
        {
            srcSpriteP->drawData->vertMoveDelta = rowDelta * kSpriteMoveDelta;
            srcSpriteP->drawData->horizMoveDelta = colDelta * kSpriteMoveDelta;
        }
        else
        {
            srcSpriteP->drawData->vertMoveDelta = 0;
            srcSpriteP->drawData->horizMoveDelta = 0;
        }
    }
    
    SWOffsetSprite(srcSpriteP, srcSpriteP->drawData->horizMoveDelta, srcSpriteP->drawData->vertMoveDelta);
    
    srcSpriteP->needsToBeDrawn = true;  // Otherwise the particles will erase parts of our sprite when it is idle
}


///--------------------------------------------------------------------------------------
//  DiamondMeterSpriteMoveProc
///--------------------------------------------------------------------------------------

void DiamondMeterSpriteMoveProc(SpritePtr srcSpriteP)
{
    SWMoveSprite(srcSpriteP, 
				 10 + gSpriteWorldP->visScrollRect.left, 
				 10 + gSpriteWorldP->visScrollRect.top);
}


///--------------------------------------------------------------------------------------
//  StatsSpriteMoveProc
///--------------------------------------------------------------------------------------

void StatsSpriteMoveProc(SpritePtr srcSpriteP)
{
    SWMoveSprite(srcSpriteP, 
				 gSpriteWorldP->screenRect.right - kStatsOffset + gSpriteWorldP->visScrollRect.left, 
				 8 + gSpriteWorldP->visScrollRect.top);
}


///--------------------------------------------------------------------------------------
//  UpdateDiamondMeter - change the meter Sprite's image and mask
///--------------------------------------------------------------------------------------

void UpdateDiamondMeter( void )
{
    /*
	 double  percent;
	 Rect    meterRect;
	 
	 percent = (double)gNumDiamonds / gNumDiamondsInMap;
	 
	 // 108 = length of meter; 17 = offset from start of meter
	 SW_SET_RECT(meterRect, 17, 2, (108 * percent) + 17, 14);
	 
	 
	 // Set port to our sprite's framePort GWorld
	 SetGWorld(gDiamondMeterSpriteP->curFrameP->framePort, nil);
	 
	 ForeColor(magentaColor);
	 PaintRect(&meterRect);
	 
	 // IMPORTANT: Set the color back when done! (In case CopyBits is used later)
	 ForeColor(blackColor);
	 
	 
	 // Update the pixel mask directly if we are using a pixelMask.
	 if (gDiamondMeterSpriteP->frameDrawProc != SWStdSpriteDrawProc)
	 {
	 // Set port to our sprite's pixel mask GWorld
	 SetGWorld(gDiamondMeterSpriteP->curFrameP->maskPort, nil);
	 
	 // Mask image is inverted when in 8-bit or less
	 if (gSpriteWorldP->pixelDepth <= 8)
	 ForeColor(whiteColor);
	 else
	 ForeColor(blackColor);
	 
	 PaintRect(&meterRect);
	 ForeColor(blackColor);
	 }
	 else
	 {
	 // Otherwise, use SWUpdateFrameMasks to update the region mask.
	 SWUpdateFrameMasks(gDiamondMeterSpriteP->curFrameP);
	 }
	 
	 // Set sprite to be redrawn, since we've changed its image
	 gDiamondMeterSpriteP->needsToBeDrawn = true;
	 */
}

///--------------------------------------------------------------------------------------
//  SmoothScrollingWorldMoveProc - our scrolling WorldMoveProc
///--------------------------------------------------------------------------------------

void SmoothScrollingWorldMoveProc(
								  SpriteWorldPtr spriteWorldP,
								  SpritePtr followSpriteP)
{   
    short   screenMidRectTop, screenMidRectLeft;
    
    screenMidRectTop = gScreenMidRect.top + spriteWorldP->visScrollRect.top;
    screenMidRectLeft = gScreenMidRect.left + spriteWorldP->visScrollRect.left;
    
    spriteWorldP->horizScrollDelta = (kSpriteMoveDelta * 
									  (SWGetSpriteHorizLoc(followSpriteP) - screenMidRectLeft) ) / kSpriteMoveDistance;
	
    spriteWorldP->vertScrollDelta = (kSpriteMoveDelta * 
									 (SWGetSpriteVertLoc(followSpriteP) - screenMidRectTop) ) / kSpriteMoveDistance;
}


///--------------------------------------------------------------------------------------
//  UpdateKeys (Put the latest key values in the keys structure)
///--------------------------------------------------------------------------------------

void UpdateKeys( void )
{        
	return;
	/*
    Uint8 *keystate = SDL_GetKeyboardState( NULL );
    
    gKeys.left = ( keystate[SDLK_LEFT] || keystate[SDLK_KP4] );
    gKeys.right = ( keystate[SDLK_RIGHT] || keystate[SDLK_KP6] );
    gKeys.down = ( keystate[SDLK_DOWN] || keystate[SDLK_KP2] );
    gKeys.up = ( keystate[SDLK_UP] || keystate[SDLK_KP8] );
	 */
}

	



/*




 *	rectangles.c
 *	written by Holmes Futrell
 *	use however you want

#include "SDL.h"
#include <time.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

int
randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void
render(void)
{
    Uint8 r, g, b;
    //  Come up with a random rectangle
    SDL_Rect rect;
    rect.w = randomInt(64, 128);
    rect.h = randomInt(64, 128);
    rect.x = randomInt(0, SCREEN_WIDTH);
    rect.y = randomInt(0, SCREEN_HEIGHT);
	
    // Come up with a random color
    r = randomInt(50, 255);
    g = randomInt(50, 255);
    b = randomInt(50, 255);
	
    // Fill the rectangle in the color
	SDL_SetRenderDrawColor(r, g, b, 255);
    SDL_RenderFill(&rect);
	
    // update screen
    SDL_RenderPresent();
}

int
main(int argc, char *argv[])
{
	
    SDL_WindowID windowID;
    int done;
    SDL_Event event;
	
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Could not initialize SDL\n");
    }
	
    // seed random number generator
    srand(time(NULL));
	
	// turn off the depth buffer -- big performance win on the phone itself
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);	
	
    // create window and renderer
    windowID =
	SDL_CreateWindow(NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
					 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (windowID == 0) {
        printf("Could not initialize Window\n");
    }
    if (SDL_CreateRenderer(windowID, -1, 0) != 0) {
        printf("Could not create renderer\n");
    }
	
    // Fill screen with black
	SDL_SetRenderDrawColor(0, 0, 0, 0);
    SDL_RenderFill(NULL);
	
    // Enter render loop, waiting for user to quit
    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        render();
        SDL_Delay(0.05);
    }
	
    // shutdown SDL
    SDL_Quit();
	
    return 0;
}

*/