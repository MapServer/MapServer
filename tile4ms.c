/******************************************************************************
* tile4ms.mc  
*
* Version 1.0
* Author Herbie Freytag hfreytag@dlwc.nsw.gov.au
*
* Create shapefile of rectangles from extents of several shapefiles (=tiles)
* Create DBF with file names for shape tiles, in column LOCATION as required by mapserv.
* For use with Mapserv tiling capability.
* Issues: resulting shape files do not display in ArcView.
*
* This code is based on shapelib API by Frank Warmerdam.
* Released without restrictions except the disclaimer below
* into custodianship of mapserv committers.
*
* Disclaimer:
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
******************************************************************************
 *
 * requires shapelib 1.2
 *
 * 
 * to compile:
 * gcc -I <path to shapelib>  -o tile4ms tile4ms.c <path to shapelib>shpopen.o <path to shapelib>dbfopen.o
 *
 * 
 */


#include <shapefil.h>
#include <string.h>


/***********************************************************************/
int process_shapefiles(char *metaFileNameP, char *tileFileNameP) 
{
int		nShapeType, nEntities, nVertices, nParts, i, iPart;
double		adfBndsMin[4], adfBndsMax[4];
SHPHandle	hSHP, tileSHP;
SHPObject	*extentRect;
DBFHandle	tileDBF;

FILE		*metaFP = NULL;
char		*p;
char		tileshapeName[256];
char		tiledbfName[256];
char		shapeFileName[256];
double		shapeX[5];
double		shapeY[5];
int		entityNum;

int		tilesFound = 0;
int		tilesProcessed = 0;


  // open metafile
  // -------------
  if (NULL==(metaFP=fopen(metaFileNameP, "r"))) {
	printf( "Unable to open:%s\n", metaFileNameP);
	return(1);
	}


  // create new tileindex shapefiles and create a header
  // --------------------------------------------------
  sprintf(tileshapeName, "%s.shp", tileFileNameP);
  if(NULL==(tileSHP=SHPCreate(tileFileNameP, SHPT_POLYGON))) {
	fclose(metaFP);
	printf("Unable to create %s.shp (.shx)\n", tileshapeName);
	return(1);
	}

  
  // create new tileindex dbf-file
  // -----------------------------
  sprintf(tiledbfName, "%s.dbf", tileFileNameP);
  if (NULL==(tileDBF=DBFCreate(tiledbfName))) {
	fclose(metaFP);
	SHPClose(tileSHP);
	printf("DBFCreate(%s) failed.\n", tiledbfName);
	return(1);
	}

   if(DBFAddField(tileDBF, "LOCATION", FTString, 255, 0 )== -1 ) {
	fclose(metaFP);
	SHPClose(tileSHP);
	DBFClose(tileDBF);
	printf("DBFAddField(fieldname='LOCATION') failed.\n");
	return(1);
	}



  // loop through files listed in metafile
  // =====================================
  while (fgets(shapeFileName, 255, metaFP)) {
	

	if (p=strchr(shapeFileName, '\n')) *p='\0';

	if (!strlen(shapeFileName))
		break;

	tilesFound++;


	// read extent from shapefile
	// --------------------------
	hSHP = SHPOpen(shapeFileName, "rb");

	if( hSHP == NULL )  {
		printf( "Aborted. Unable to open:%s\n", shapeFileName);
		break;
		}

	SHPGetInfo(hSHP, &nEntities, &nShapeType, adfBndsMin, adfBndsMax);

	//printf("File:  %s Bounds 0/1: (%15.10lg,%15.10lg)\n\t(%15.10lg,%15.10lg)\n", shapeFileName, adfBndsMin[0], adfBndsMin[1], adfBndsMax[0], adfBndsMax[1] );

	SHPClose(hSHP);


	// create rectangle describing current shapefile extent
	// ----------------------------------------------------

	shapeX[0] = shapeX[4] = adfBndsMin[0]; // bottom left
	shapeY[0] = shapeY[4] = adfBndsMin[1];
	shapeX[1] = adfBndsMin[0]; // top left
	shapeY[1] = adfBndsMax[1];
	shapeX[2] = adfBndsMax[0]; // top left
	shapeY[2] = adfBndsMax[1];
	shapeX[3] = adfBndsMax[0]; // bottom right
	shapeY[3] = adfBndsMin[1];


	// create and add shape object.  Returns link to entry in DBF file
	// ---------------------------------------------------------------

	extentRect = SHPCreateSimpleObject(SHPT_POLYGON, 5, shapeX, shapeY, NULL);

	entityNum = SHPWriteObject(tileSHP, -1, extentRect);
	
	SHPDestroyObject(extentRect);


	// store filepath of current shapefile as attribute of rectangle
	// -------------------------------------------------------------

	DBFWriteStringAttribute(tileDBF, entityNum, 0, shapeFileName);

	tilesProcessed++;

	}

  SHPClose(tileSHP);
  DBFClose(tileDBF);

  fclose(metaFP);

  
  printf("Processed %i of %i files\n", tilesProcessed, tilesFound);

  
  return (0);

}

/***********************************************************************/
void print_usage_and_exit() {

	printf("\nusage: tile4ms <meta-file> <tile-file>\n" );
	printf("<meta-file>\tINPUT  file containing list of shapefile names\n\t\t(complete paths 255 chars max, no extension)\n");
	printf("<tile-file>\tOUTPUT shape file of extent rectangles and names\n\t\tof tiles in <tile-file>.dbf\n\n");
	exit(1);
}



/***********************************************************************/
int main( int argc, char **argv )
{

  // stun user with existence of help 
  // --------------------------------
  if ((argc == 2)&&(strstr(argv[1], "-h"))) {

	print_usage_and_exit();
	}


  // check arguments
  // ---------------
  if( argc != 3 )  {

	print_usage_and_exit();
	}


  process_shapefiles(argv[1], argv[2]);


  exit(0);
}
