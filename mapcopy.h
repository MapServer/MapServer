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
#define MS_COPYSTELEM(name) (dst)->/**/name = (src)->/**/name

#define MS_MACROBEGIN              do {
#define MS_MACROEND                } while (0) 

#define MS_COPYRECT(dst, src)       \
    MS_MACROBEGIN                   \
        (dst)->minx = (src)->minx;  \
        (dst)->miny = (src)->miny;  \
        (dst)->maxx = (src)->maxx;  \
        (dst)->maxy = (src)->maxy;  \
    MS_MACROEND

#define MS_COPYPOINT(dst, src)      \
    MS_MACROBEGIN                   \
        (dst)->x = (src)->x;        \
        (dst)->y = (src)->y;        \
        (dst)->m = (src)->m;        \
    MS_MACROEND

#define MS_COPYCOLOR(dst, src)      \
    MS_MACROBEGIN                   \
        (dst)->pen   = (src)->pen;  \
        (dst)->red   = (src)->red;  \
        (dst)->green = (src)->green;\
        (dst)->blue  = (src)->blue; \
    MS_MACROEND

#define MS_COPYSTRING(dst, src)     \
    MS_MACROBEGIN                   \
        if ((dst) != NULL)          \
            msFree((dst));          \
        if (src)                    \
            (dst) = strdup((src));  \
        else                        \
            (dst) = NULL;           \
    MS_MACROEND


