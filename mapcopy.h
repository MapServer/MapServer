/* $Id */

/* ==========================================================================
 * mapcopy.h 
 *
 * Macros for use in copying map objects
 *
 * Provided by Mladen Turk for resolution of bug 701.
 *
 *   http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=701
 *
 * ========================================================================== */

/* Following works for GCC and MSVC.  That's ~98% of our users, if Tyler
 * Mitchell's survey is correct
 */
#define MS_COPYSTELEM(_name) (dst)->/**/_name = (src)->/**/_name

#define MS_MACROBEGIN              do {
#define MS_MACROEND                } while (0) 

#define MS_COPYRECT(_dst, _src)       \
    MS_MACROBEGIN                     \
        (_dst)->minx = (_src)->minx;  \
        (_dst)->miny = (_src)->miny;  \
        (_dst)->maxx = (_src)->maxx;  \
        (_dst)->maxy = (_src)->maxy;  \
    MS_MACROEND

#define MS_COPYPOINT(_dst, _src)      \
    MS_MACROBEGIN                     \
        (_dst)->x = (_src)->x;        \
        (_dst)->y = (_src)->y;        \
        (_dst)->m = (_src)->m;        \
    MS_MACROEND

#define MS_COPYCOLOR(_dst, _src)      \
    MS_MACROBEGIN                     \
        (_dst)->pen   = (_src)->pen;  \
        (_dst)->red   = (_src)->red;  \
        (_dst)->green = (_src)->green;\
        (_dst)->blue  = (_src)->blue; \
    MS_MACROEND

#define MS_COPYSTRING(_dst, _src)     \
    MS_MACROBEGIN                     \
        if ((_dst) != NULL)           \
            msFree((_dst));           \
        if ((_src))                   \
            (_dst) = strdup((_src));  \
        else                          \
            (_dst) = NULL;            \
    MS_MACROEND


