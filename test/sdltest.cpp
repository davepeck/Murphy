// sdltest.cpp : test SDL functionality by putting up a GIF

#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
//#include "SDLMain.h"


typedef struct OliverBubbleState
{
	int animChoice;
	int state;
	SDL_Rect from;
	SDL_Rect to;
	SDL_Surface* sprite;
	SDL_Surface* screen;
};


typedef struct MapEntry
{
    int iconX;
    int iconY;
    int trigID;
    int trigIconX;
    int trigIconY;
    int state;
    int kind;
    int levelID;
} MapEntry;


Uint32 Bubble_Callback( Uint32 interval, void* userData )
{
	OliverBubbleState* obs = (OliverBubbleState*) userData;

	// update the current animation state
	obs->state += 1;

	// do we need to choose a new state?
	// (0 = normal, 1 = first frame, 2 = second frame, 3 = first
	// frame, 4 = new)
	if ( obs->state == 4 )
	{
		obs->state = 0;
		obs->animChoice = rand() % 4;
	}

	// set the appropraite FROM rectangle
	if ( obs->state  == 0 )
		obs->from.x = 0;
	else if ( obs->state  == 1 || obs->state  == 3 )
		obs->from.x = 32 + ( 64 * obs->animChoice  );
	else
		obs->from.x = 64 + ( 64 * obs->animChoice  );

	// perform the blit
	SDL_BlitSurface( obs->sprite, &(obs->from), obs->screen, &(obs->to) );
	SDL_UpdateRect( obs->screen, obs->to.x, obs->to.y, obs->to.w, obs->to.h );
	
	// return the interval to continue the "alarm"
	return interval;
}


int main( int argc, char* argv[] )
{

	//----------------------------------------------------------------------
	// Initialize SDL
	//----------------------------------------------------------------------

	if ( SDL_Init( SDL_INIT_VIDEO ) == -1 )
	{
		printf( "Can't init SDL: %s\n", SDL_GetError() );
		exit( -1 );
	}

	SDL_Init( SDL_INIT_TIMER );


	atexit( SDL_Quit );
    
	//----------------------------------------------------------------------
	// Initialize Screen
	//----------------------------------------------------------------------

	SDL_Surface *screen;
    if ( argc > 1 )
        screen = SDL_SetVideoMode( 640, 480, 16, SDL_FULLSCREEN | SDL_HWSURFACE );
    else
        screen = SDL_SetVideoMode( 640, 480, 16, SDL_HWSURFACE );
    
	if ( screen == NULL )
	{
		printf( "Can't set video mode: %s\n", SDL_GetError() );
		exit( -1 );
	}


	//----------------------------------------------------------------------
	// Fade to black
	//----------------------------------------------------------------------

	// SDL_SetGamma( 0, 0, 0 );


	//----------------------------------------------------------------------
	// Load Bitmaps
	//----------------------------------------------------------------------

	SDL_Surface *interfaceImage;
	interfaceImage = SDL_LoadBMP( "main.bmp" );

	SDL_Surface *oliverImages;
	oliverImages = SDL_LoadBMP( "sprite.bmp" );


	//----------------------------------------------------------------------
	// Seed Random number Generator & init oliver state
	//----------------------------------------------------------------------

	srand( SDL_GetTicks() );

	OliverBubbleState obs;
	obs.animChoice = rand() % 4;
	obs.state = -1;
	obs.from.x = 0;
	obs.from.y = 0;
	obs.from.w = 32;
	obs.from.h = 32;
	obs.to.x = 304 + ((640 - interfaceImage->w)/2);
	obs.to.y = 198 + ((480 - interfaceImage->h)/2);
	obs.to.w = 32;
	obs.to.h = 32;
	obs.sprite = SDL_DisplayFormat( oliverImages );
	obs.screen = screen;
	SDL_FreeSurface( oliverImages );


	//----------------------------------------------------------------------
	// Draw Background Rectangles
	//----------------------------------------------------------------------

	SDL_Rect fillRect;
	fillRect.x = 0;
	fillRect.y = 0;
	fillRect.w = 320;
	fillRect.h = 40;
	SDL_FillRect( screen,
                  &fillRect,
                  SDL_MapRGB( screen->format, 153, 153, 204 ) );

	fillRect.x = 320;
	SDL_FillRect( screen,
                  &fillRect,
                  SDL_MapRGB( screen->format, 0, 102, 153 ) );

	fillRect.y = 440;
	SDL_FillRect( screen,
                  &fillRect,
                  SDL_MapRGB( screen->format, 255, 153, 0 ) );

	fillRect.x = 0;
	SDL_FillRect( screen,
                  &fillRect,
                  SDL_MapRGB( screen->format, 255, 204, 153 ) );



	//----------------------------------------------------------------------
	// Draw Primary Image
	//----------------------------------------------------------------------

	SDL_Rect imageRect;
	SDL_Rect screenRect;
	imageRect.x = 0;
	imageRect.y = 0;
	imageRect.w = interfaceImage->w;
	imageRect.h = interfaceImage->h;
	screenRect.y = (480 - interfaceImage->h) / 2;
	screenRect.x = (640 - interfaceImage->w) / 2;

	SDL_BlitSurface( interfaceImage, &imageRect, screen, &screenRect );
	SDL_FreeSurface( interfaceImage );
	SDL_UpdateRect( screen, 0, 0, 0, 0 );


	//----------------------------------------------------------------------
	// Set up bubble timer & callback
	//----------------------------------------------------------------------

	SDL_TimerID bubbleTimer = SDL_AddTimer( 133, Bubble_Callback, &obs );


	//----------------------------------------------------------------------
	// Fade in.
	//----------------------------------------------------------------------

/*	for ( float f = 0.0; f < 1.0; f += 0.02 )
	{
		SDL_SetGamma( f, f, f );
		//SDL_Delay( 15 );
	}
	SDL_SetGamma( 1.0, 1.0, 1.0 );*/


	//----------------------------------------------------------------------
	// Loop: exit on keypress
	//----------------------------------------------------------------------

	SDL_Event event;
	bool done = false;

	while ( !done )
	{
		while ( SDL_PollEvent( &event ) )
		{
			// handle the event, if any
			switch ( event.type )
			{
				case SDL_KEYDOWN:
					done = true;
					break;

				case SDL_QUIT:
					done = true;
					break;
			}
		}
	}


    // Change the location of the bubbling Oliver animation
	obs.to.x = 600;
	obs.to.y = 30;
	obs.to.w = 32;
	obs.to.h = 32;


    //----------------------------------------------------------------------
    // Load the overhead map data
    //----------------------------------------------------------------------


    MapEntry mapData[120][120];
        
    FILE* worldData = fopen( "fwfinal.txt", "r" );
    for ( int j = 0; j < 120; j++ )
    {
        for ( int i = 0; i < 120; i++ )
        {
            for ( int v = 0; v < 8; v++ )
            {
                size_t bufLen;
                char* lineBuf = fgetln( worldData, &bufLen );

                if ( lineBuf == NULL )
                    printf( "error: lineBuf unexpectedly null.\n" );

                int iVal = atoi( lineBuf );

                switch ( v )
                {
                case 0:
                    mapData[i][j].iconX = iVal;
                    break;
                case 1:
                    mapData[i][j].iconY = iVal;
                    break;
                case 2:
                    mapData[i][j].trigID = iVal;
                    break;
                case 3:
                    mapData[i][j].trigIconX = iVal;
                    break;
                case 4:
                    mapData[i][j].trigIconY = iVal;
                    break;
                case 5:
                    mapData[i][j].state = iVal;
                    break;
                case 6:
                    mapData[i][j].kind = iVal;
                    break;
                case 7:
                    mapData[i][j].levelID = iVal;
                    break;
                }
            }
            
        }
    }
    fclose( worldData );
    
    
    //----------------------------------------------------------------------
    // Load in world tile graphics
    //----------------------------------------------------------------------
    
	SDL_Surface *worldTileImage;
	worldTileImage = SDL_LoadBMP( "world-tiles.bmp" );
    SDL_Surface *worldTiles;
    worldTiles = SDL_DisplayFormat( worldTileImage );
    SDL_FreeSurface( worldTileImage );
    
    SDL_Surface *statusBarImage;
    statusBarImage = SDL_LoadBMP( "status-bar-map.bmp" );

    SDL_Surface *mapRoomImage;
    mapRoomImage = SDL_LoadBMP( "bar-map-room.bmp" );

    
    // rectangle in which to display map tiles
    SDL_Rect mapRect;
    mapRect.x = mapRoomImage->w;
    mapRect.y = 0;
    mapRect.w = 640 - (mapRoomImage->w + statusBarImage->w);
    mapRect.h = 480;

    
    // draw map room image
	imageRect.x = 0;
	imageRect.y = 0;
	imageRect.w = mapRoomImage->w;
	imageRect.h = mapRoomImage->h;
	screenRect.y = 0;
	screenRect.x = 0;

	SDL_BlitSurface( mapRoomImage, &imageRect, screen, &screenRect );
	SDL_FreeSurface( mapRoomImage );

    
    // draw status bar image
    imageRect.x = 0;
    imageRect.y = 0;
    imageRect.w = statusBarImage->w;
    imageRect.h = statusBarImage->h;
    screenRect.y = 0;
    screenRect.x = 640 - statusBarImage->w;

    SDL_BlitSurface( statusBarImage, &imageRect, screen, &screenRect );
    SDL_FreeSurface( statusBarImage );


    // clear the map (you decide what)
	SDL_FillRect( screen,
                  &mapRect,
                  SDL_MapRGB( screen->format, 0, 0, 0 ) );
    

    // how many tiles can I fit on the screen?
    const int kTilePixelSize = 24;
    int kStartX = 80;
    int kStartY = 40;
    int mapAcross;
    int mapDown;

    mapAcross = mapRect.w / kTilePixelSize;
    mapDown = mapRect.h / kTilePixelSize;


    // draw some tiles from the map
    SDL_Rect tileFrom;
    SDL_Rect tileTo;
    tileFrom.w = kTilePixelSize;
    tileFrom.h = kTilePixelSize;
    tileTo.w = kTilePixelSize;
    tileTo.h = kTilePixelSize;

    SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );



    // draw all tiles from the map into a separate SDL_SURFACE
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask = 0xff000000;
    Uint32 gmask = 0x00ff0000;
    Uint32 bmask = 0x0000ff00;
    Uint32 amask = 0x000000ff;
    #else
    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;
    #endif

    SDL_Surface* mapOffscreenDummy = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        120 * kTilePixelSize,
        120 * kTilePixelSize,
        16,
        rmask,
        gmask,
        bmask,
        amask );
    SDL_Surface* mapOffscreen = SDL_DisplayFormat( mapOffscreenDummy );
    SDL_FreeSurface( mapOffscreenDummy );
    
    if ( mapOffscreen == NULL )
        return -1;

    for ( int xl = 0; xl < 120; xl++ )
    {
        for ( int yl = 0; yl < 120; yl++ )
        {
            tileFrom.x = mapData[xl][yl].iconX * kTilePixelSize;
            tileFrom.y = mapData[xl][yl].iconY * kTilePixelSize;
            tileTo.x = xl * kTilePixelSize;
            tileTo.y = yl * kTilePixelSize;
            SDL_BlitSurface( worldTiles, &tileFrom, mapOffscreen, &tileTo );
        }
    }    



    // copy the appropriate portion of the map offscreen
    // onto the onscreen as the user presses keystrokes
    SDL_Rect mapFrom;
    SDL_Rect mapTo;
    mapFrom.w = mapRect.w;
    mapFrom.h = mapRect.h;
    mapTo.w = mapRect.w;
    mapTo.h = mapRect.h;
    mapTo.x = mapRect.x;
    mapTo.y = mapRect.y;
    
    
    done = false;
	while ( !done )
	{

        if ( kStartX + mapAcross > 119 )
            kStartX = 119 - mapAcross;
        if ( kStartX < 0 )
            kStartX = 0;
        if ( kStartY + mapDown > 119 )
            kStartY = 119 - mapDown;
        if ( kStartY < 0 )
            kStartY = 0;


        mapFrom.x = kStartX * kTilePixelSize;
        mapFrom.y = kStartY * kTilePixelSize;

        SDL_BlitSurface( mapOffscreen, &mapFrom, screen, &mapTo );

        /*
        for ( int x = 0; x < mapAcross; x++ )
    {
        for ( int y = 0; y < mapDown; y++ )
        {
            tileFrom.x = mapData[x + kStartX][y + kStartY].iconX * kTilePixelSize;
            tileFrom.y = mapData[x + kStartX][y + kStartY].iconY * kTilePixelSize;
            tileTo.x = mapRect.x + (x * kTilePixelSize);
            tileTo.y = mapRect.y + (y * kTilePixelSize);
            SDL_BlitSurface( worldTiles, &tileFrom, screen, &tileTo );
        }
        */
        //}
    
    
    // update the screen, darn it.
    SDL_UpdateRect( screen, 0, 0, 0, 0 );
    
    
    

	//----------------------------------------------------------------------
	// Loop: exit on keypress
	//----------------------------------------------------------------------
    
		while ( SDL_PollEvent( &event ) )
		{
			// handle the event, if any
			switch ( event.type )
			{
				case SDL_KEYDOWN:
                {
                    switch ( event.key.keysym.sym )
                    {
                    case SDLK_UP:
                        kStartY -= 1;
                        break;

                    case SDLK_DOWN:
                        kStartY += 1;
                        break;

                    case SDLK_LEFT:
                        kStartX -= 1;
                        break;

                    case SDLK_RIGHT:
                        kStartX += 1;
                        break;

                    case SDLK_RETURN:
                        done = true;
                        break;
                    }

                }
                break;

				case SDL_QUIT:
					done = true;
					break;
			}
		}
	}



    //----------------------------------------------------------------------
    // Final cleanup
    //----------------------------------------------------------------------


	SDL_FreeSurface( obs.sprite );
    SDL_FreeSurface( worldTiles );
    SDL_FreeSurface( mapOffscreen );

	return 0;
}
