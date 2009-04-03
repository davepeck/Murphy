//---------------------------------------------------------------------------------------
/// @file SpriteWorldUtils.h
/// Constants, structures, and function prototypes for helper utilities
//---------------------------------------------------------------------------------------

#ifndef __SWUTILS__
#define __SWUTILS__

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FatalError(err)         if (err) ReportError(err, __FILE__, (char *)__FUNCTION__, __LINE__);

void Randomize(void);
int GetRandom( int min, int max );

void ReportError( SWError err, char* fileName, char* functionName, int lineNumber);

#ifdef __cplusplus
}
#endif

#endif // __SWUTILS__

