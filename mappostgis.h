#ifdef USE_POSTGIS

#include "libpq-fe.h"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 2
#endif

/* HEX = 16 or BASE64 = 64*/
#define TRANSFER_ENCODING 16

/*
** msPostGISLayerInfo
**
** Specific information needed for managing this layer.
*/
typedef struct {
    char        *sql;        /* SQL query to send to database */
    PGconn      *pgconn;     /* Connection to database */
    long        rownum;      /* What row is the next to be read (for random access) */
    PGresult    *pgresult;   /* For fetching rows from the database */
    char        *uid;        /* Name of user-specified unique identifier, if set */
    char        *srid;       /* Name of user-specified SRID: zero-length => calculate; non-zero => use this value! */
    char        *geomcolumn; /* Specified geometry column, eg "THEGEOM from thetable" */
    char        *fromsource; /* Specified record source, ed "thegeom from THETABLE" or "thegeom from (SELECT..) AS FOO" */
    int         endian;      /* Endianness of the mapserver host */
}
msPostGISLayerInfo;

/*
** Prototypes
*/
void msPostGISFreeLayerInfo(layerObj *layer);
msPostGISLayerInfo *msPostGISCreateLayerInfo(void);
char *msPostGISBuildSQL(layerObj *layer, rectObj *rect, long *uid);
int msPostGISParseData(layerObj *layer);

#endif /* USE_POSTGIS */

