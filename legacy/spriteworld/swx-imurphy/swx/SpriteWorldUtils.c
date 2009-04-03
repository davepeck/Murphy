
#include <time.h>
#include <stdlib.h>

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SWUTILS__
#include "SpriteWorldUtils.h"
#endif 

//---------------------------------------------------------------------------------------
//  Randomize
//---------------------------------------------------------------------------------------

void Randomize(void)
{
    srand( (unsigned int)time(NULL) );
}

//---------------------------------------------------------------------------------------
//  GetRandom - generates pseudo random number. min and max are inclusive
//---------------------------------------------------------------------------------------

int GetRandom( int min, int max )
{
    int num;
    float random;
    
    random = rand()/(RAND_MAX + 1.0); //a number from 0.0 to 0.9999
    num =  (int)((1+ max-min) * random) ;
    num += min;
    
    return num;
}

//---------------------------------------------------------------------------------------
//  ReportError
//---------------------------------------------------------------------------------------

void ReportError( SWError err, char* fileName, char* functionName, int lineNumber)
{
    fprintf( stderr, "Fatal error (#%d).\n  File:%s\n  Function:%s\n  Line:%d\n",
        (int) err, fileName, functionName, lineNumber );
    exit(1);
}


