/* $Id$ */
/* This file contain utility functions that come from the GDAL/OGR cpl
  library. The idea behind it is to have access in mapserver to all these
  utilities, without being constarined to link with GDAL/OGR.
  Note : Names of functions used here are the same as those in the cpl
         library with the exception the the CPL prefix is changed to ms
         (eg : CPLGetBasename() would become msGetBasename())
*/

#include <assert.h>
#include "map.h"

/* should be size of larged possible filename */
#define MS_PATH_BUF_SIZE 2048
static char     szStaticResult[MS_PATH_BUF_SIZE]; 


/************************************************************************/
/*                        msFindFilenameStart()                         */
/************************************************************************/

static int msFindFilenameStart( const char * pszFilename )

{
    int         iFileStart;

    for( iFileStart = strlen(pszFilename);
         iFileStart > 0
             && pszFilename[iFileStart-1] != '/'
             && pszFilename[iFileStart-1] != '\\';
         iFileStart-- ) {}

    return iFileStart;
}

/************************************************************************/
/*                           msGetBasename()                            */
/************************************************************************/

/**
 * Extract basename (non-directory, non-extension) portion of filename.
 *
 * Returns a string containing the file basename portion of the passed
 * name.  If there is no basename (passed value ends in trailing directory
 * separator, or filename starts with a dot) an empty string is returned.
 *
 * <pre>
 * msGetBasename( "abc/def.xyz" ) == "def"
 * msGetBasename( "abc/def" ) == "def"
 * msGetBasename( "abc/def/" ) == ""
 * </pre>
 *
 * @param pszFullFilename the full filename potentially including a path.
 *
 * @return just the non-directory, non-extension portion of the path in
 * an internal string which must not be freed.  The string
 * may be destroyed by the next ms filename handling call.
 */

const char *msGetBasename( const char *pszFullFilename )

{
    int iFileStart = msFindFilenameStart( pszFullFilename );
    int iExtStart, nLength;

    for( iExtStart = strlen(pszFullFilename);
         iExtStart > iFileStart && pszFullFilename[iExtStart] != '.';
         iExtStart-- ) {}

    if( iExtStart == iFileStart )
        iExtStart = strlen(pszFullFilename);

    nLength = iExtStart - iFileStart;

    assert( nLength < MS_PATH_BUF_SIZE );

    strncpy( szStaticResult, pszFullFilename + iFileStart, nLength );
    szStaticResult[nLength] = '\0';

    return szStaticResult;
}
