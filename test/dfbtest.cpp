// dfbtest.cpp: test DirectFB code to display an image.

#include <stdio.h>
#include <unistd.h>
#include <directfb.h>

static IDirectFB *dfb = NULL;
static IDirectFBSurface *primary = NULL;
static unsigned int screen_width = 0;
static unsigned int screen_height = 0;
static IDirectFBSurface *logo = NULL;

int main ( int argc, char** argv )
{
	int i;
	DFBSurfaceDescription dsc;
	IDirectFBImageProvider *provider;


	// Initialize
	DirectFBInit( &argc, &argv );
	DirectFBCreate( &dfb );

	// Set to fullscreen
	dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );
	dsc.flags = DSDESC_CAPS;
	dsc.caps = (DFBSurfaceCapabilities) (DSCAPS_PRIMARY | DSCAPS_FLIPPING);

	dfb->CreateSurface( dfb, &dsc, &primary );
	primary->GetSize( primary, &screen_width, &screen_height );


	// Load image
	dfb->CreateImageProvider( dfb, "/home/dave/dev/MR/test/main.png", &provider );
	provider->GetSurfaceDescription( provider, &dsc );

	// Create a surface from image
	dfb->CreateSurface( dfb, &dsc, &logo );

	// render it
	provider->RenderTo( provider, logo, NULL );

	// clean up
	provider->Release( provider );


	// animate the image, I think.
	for ( i = -dsc.width; i < screen_width; i++ )
	{
		primary->FillRectangle( primary, 0, 0, screen_width, screen_height );
		primary->Blit( primary, logo, NULL, i, (screen_height-dsc.height) / 2 );
		primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
	}


	// now really clean up.
	logo->Release( logo );
	primary->Release( primary );
	dfb->Release( dfb );

	return 0;
}
