/**
 *	filename: mappdf.c
 *	created : Thu Oct  4 09:58:19 2001
 *	@author :  <jwall@webpeak.com> , <jspielberg@webpeak.com>
 *	LastEditDate Was "Thu Oct 11 18:25:45 2001"
 *
 * [$Author$ $Date$]
 * [$Revision$]
 *
 **/

#include "map.h"

static double inchesPerUnitPDF[6]={1, 12, 63360.0, 39.3701, 39370.1, 4374754};
//int labelCount = 0;
//#define MS_MAP2PDF_Y(y,miny,cy) (MS_NINT((y - miny)/cy))

void draw_highway_shieldPDF(PDF *pdf, float center_x, float center_y){

    float text_width; // = PDF_stringwidth(pdf,"C470",font,font_size)+2;
    float text_height; // = font_size * 1.5;

// This is going to be 6 separate curves
    text_width = 25;
    text_height = 12;

    center_y -= 5;

    PDF_setlinewidth(pdf,.4);
    PDF_setrgbcolor_fill(pdf, (float)204/255, 0, 0);
    PDF_setrgbcolor_stroke(pdf, 1, 1, 1);
    PDF_moveto(pdf, center_x, center_y - .25*text_height);
    PDF_curveto(pdf,
                center_x, center_y-.125*text_height,
                center_x-.3*text_width, center_y-.125*text_height,
                center_x-.3*text_width, center_y-.25*text_height);

    PDF_lineto(pdf, center_x-.4*text_width, center_y);
    PDF_lineto(pdf, center_x+.4*text_width, center_y);

    PDF_lineto(pdf, center_x+.3*text_width, center_y-.25*text_height);
    PDF_curveto(pdf,
                center_x+.3*text_width, center_y-.125*text_height,
                center_x, center_y-.125*text_height,
                center_x, center_y - .25*text_height);
    PDF_fill_stroke(pdf);

    PDF_setrgbcolor_fill(pdf, 0, 0, (float)204/255);
    PDF_moveto(pdf, center_x-.4*text_width, center_y);
    PDF_curveto(pdf,
                center_x -.5*text_width, center_y+.9*text_height,
                center_x, center_y + 1.25*text_height,
                center_x, center_y + 1.2*text_height);
    PDF_curveto(pdf,
                center_x, center_y + 1.25*text_height,
                center_x+.5*text_width, center_y+.9*text_height,
                center_x+.4*text_width, center_y);
    PDF_closepath_fill_stroke(pdf);

    PDF_setrgbcolor_fill(pdf, 1, 1, 1);
    PDF_setrgbcolor_stroke(pdf, 0, 0, 0);
}

void draw_federal_shieldPDF(PDF *pdf, float center_x, float center_y){

    float text_width; // = PDF_stringwidth(pdf,text,font,font_size)+2;
    float height; //  = font_size;
    float pad;

//    text_width = PDF_stringwidth(pdf,"240",font,font_size)+1;
//    if (height < .75 * text_width){
//        height = .75* text_width;
//    }
// This is going to be a bunch of separate curves

    text_width = 21;
    height = 17;

    center_y -= 5;

//    pad = (height - font_size)/2;
    pad = (height - 9)/2;
    PDF_setlinewidth(pdf,.4);
    PDF_setrgbcolor_fill(pdf, 1, 1, 1);
    PDF_setrgbcolor_stroke(pdf, 0, 0, 0);

    PDF_moveto(pdf, center_x, center_y-pad);
    PDF_curveto(pdf,
                center_x-.25*text_width, center_y+1-pad,
                center_x-.25*text_width, center_y+1-pad,
                center_x-.5*text_width, center_y-pad);

    PDF_lineto(pdf, center_x-.5*text_width-1, center_y+1-pad);
    PDF_lineto(pdf, center_x-.5*text_width, center_y+.5*height-pad);
    PDF_lineto(pdf, center_x-.5*text_width-1, center_y+.8*height-pad);

    PDF_curveto(pdf,
                center_x -.7*text_width, center_y+1.5*height-pad,
                center_x, center_y + 1*height-pad,
                center_x, center_y + 1.4*height-pad);

    PDF_curveto(pdf,
                center_x, center_y + 1*height-pad,
                center_x+ .7*text_width, center_y+1.5*height-pad,
                center_x+.5*text_width+1, center_y+.8*height-pad);

    PDF_lineto(pdf, center_x+.5*text_width, center_y+.5*height-pad);

    PDF_lineto(pdf, center_x+.5*text_width+1, center_y+1-pad);
    PDF_lineto(pdf, center_x+.5*text_width, center_y-pad);
    PDF_curveto(pdf,
                center_x+.25*text_width, center_y+1-pad,
                center_x+.25*text_width, center_y+1-pad,
                center_x, center_y-pad);


    PDF_fill_stroke(pdf);
    PDF_setrgbcolor(pdf, 0,0,0);
}

/*
** Simply draws a label based on the label point and the supplied label object.
*/
static int draw_text_PDF(PDF *pdf, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, mapObj *map, hashTableObj fontHash)
{
    int x, y, x1, y1;
    int font;
    float phi = label->angle;
    colorObj  fc, oc;
    char *wrappedString;
    char *fontKey, *fontValue;

/// Do color initialization stuff
    if (label->color != -1)
        getRgbColor(map, label->color,        &fc.red, &fc.green, &fc.blue);
    else
        fc.red = fc.blue = fc.green = 0;

    if (label->outlinecolor != -1)
        getRgbColor(map, label->outlinecolor, &oc.red, &oc.green, &oc.blue);
    else
        oc.red = oc.blue = oc.green = 0;

    if(!string)
        return(0); /* not an error, just don't want to do anything */

    if(strlen(string) == 0)
        return(0); /* not an error, just don't want to do anything */

    x = MS_NINT(labelPnt.x);
    y = MS_NINT(labelPnt.y);
    x1 = x; y1 = y;


    PDF_setrgbcolor_stroke(pdf,(float)oc.red/255,(float)oc.green/255,(float)oc.blue/255);
    PDF_setrgbcolor_fill(pdf,(float)fc.red/255,(float)fc.green/255,(float)fc.blue/255);
    PDF_setlinewidth(pdf,.3);
    if (label->font){
        fontKey = label->font;
    }
    else {
        fontKey = "Helvetica";
    }

    if ((fontValue = msLookupHashTable(fontHash, fontKey)) != NULL){
            // we have a match.. set the fonthandle
        font = atoi(fontValue);
    }
    else {
            // there was no match so insert a key value pair into the table
            // this is so that only one font is searched per file
        char buffer[5];

        font = PDF_findfont(pdf, fontKey, "winansi",0);
        sprintf(buffer, "%d",font);
        msInsertHashTable(fontHash, fontKey, buffer);
    }

    PDF_setfont(pdf,font,label->sizescaled+2);

    if (phi!=0){
//        PDF_save(pdf);
        PDF_translate(pdf, x, y);
        PDF_rotate(pdf, -phi);
        x = y = 0;
    }

    PDF_scale(pdf,1,-1);
    if ((wrappedString = index(string,'\r')) == NULL){
        PDF_show_xy(pdf,string,x,-y);
    }
    else{
        char *headPtr;
        headPtr = string;
            // break the string into pieces separated by \r\n
        while(wrappedString){
            char *piece;
            int length = wrappedString - headPtr;
            piece = malloc(length+1);bzero(piece,length+1);
            strncpy(piece, headPtr, length);

            if (headPtr == string){
                PDF_show_xy(pdf,piece,x,-y);
            }
            else {
                PDF_continue_text(pdf,piece);
            }
            free(piece);
            headPtr = wrappedString+2;
            wrappedString = index(headPtr,'\r');
        }
        PDF_continue_text(pdf,headPtr);
    }
    PDF_scale(pdf,1,-1);
    if (phi!=0){
        PDF_rotate(pdf, phi);
        PDF_translate(pdf, -x1, -y1);
//        PDF_restore(pdf);
    }
//    sprintf(ms_error.message, "(%d)", labelCount);
//    msSetError(MS_MISCERR, ms_error.message, "drawText()");
    PDF_setlinewidth(pdf,1);
    return(0);
}

/* ------------------------------------------------------------------------------- */
/*       Draw a single marker symbol of the specified size and color               */
/* ------------------------------------------------------------------------------- */
void msDrawMarkerSymbolPDF(symbolSetObj *symbolset, PDF *pdf, pointObj *p, int sy, colorObj *fc, colorObj *bc, colorObj *oc, double sz, hashTableObj fontHash)
{
    symbolObj *symbol;
    int offset_x, offset_y, x, y;
    int j,font_id;
    char *font,symbolBuffer[2],*fontValue;
    double scale=1.0;


    if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
        return;

    if(sz < 1) /* size too small */
        return;

    if (!oc) oc=fc;// if now outline color, make the stroke and fill the same

    PDF_setrgbcolor_stroke(pdf,(float)oc->red/255,(float)oc->green/255,(float)oc->blue/255);
    PDF_setrgbcolor_fill(pdf,(float)fc->red/255,(float)fc->green/255,(float)fc->blue/255);

    symbol = &(symbolset->symbol[sy]);
    scale = sz/symbol->sizey;

    switch(symbol->type) {
        case(MS_SYMBOL_TRUETYPE):

            font = msLookupHashTable(symbolset->fontset->fonts, symbol->font);
            if(!font) return;
                //plot using pdf
            sprintf(symbolBuffer,"%c",(char)*symbol->character);

            if ((fontValue = msLookupHashTable(fontHash, font)) != NULL){
                    // we have a match.. set the fonthandle
                font_id = atoi(fontValue);
            }
            else {
                    // there was no match so insert a key value pair into the table
                    // this is so that only one font is searched per file
                char buffer[5];

                font_id = PDF_findfont(pdf, symbol->font ,"winansi",1);
                sprintf(buffer, "%d",font_id);
                msInsertHashTable(fontHash, font, buffer);
            }


            PDF_setfont(pdf,font_id,sz+2);
            x = p->x - (int)(.5*PDF_stringwidth(pdf,symbolBuffer,font_id,sz));
            y = p->y;
            PDF_setlinewidth(pdf,.15);
            PDF_scale(pdf,1,-1);
            PDF_show_xy(pdf,symbolBuffer,x,-y);
            PDF_scale(pdf,1,-1);

            break;
        case(MS_SYMBOL_PIXMAP):
        {
                // Kludge for highways because
                // IMAGES DON'T LOOK GOOD

            if (strstr(symbol->name,"usuh.png")){
                    // we are in the draw highway shield
                draw_federal_shieldPDF(pdf,p->x,p->y);
            }
            else if (strstr(symbol->name,"ushw.png")){
                    // we are in the draw highway shield
                draw_highway_shieldPDF(pdf,p->x,p->y);
            }
            else {
                int length;
                char *data;
                int result;

                length = 0;
                data = gdImageJpegPtr(symbol->img, &length, 90);
                result = PDF_open_image(pdf, "jpeg", "memory", data, (long)length, symbol->img->sx, symbol->img->sy, 3, 8, NULL);
                gdFree(data);
                PDF_place_image(pdf,result,p->x-symbol->img->sx/2,p->y-symbol->img->sy/2,1);
                PDF_close_image(pdf,result);
//            gdImageSetTile(img, symbol->img);
//            msImageFilledPolygon(img, p, gdTiled);
//            if(oc>-1)
//                msImagePolyline(img, p, oc);
            }

        }
        break;
        case(MS_SYMBOL_ELLIPSE):
                // ok, this is going to be just a circle

            x = MS_NINT(symbol->sizex*scale)+1;
            y = MS_NINT(symbol->sizey*scale)+1;

            PDF_circle(pdf, p->x, p->y, x/2);

            if(symbol->filled)
                PDF_fill_stroke(pdf);
            else
                PDF_stroke(pdf);

            break;
        case(MS_SYMBOL_VECTOR):

            offset_x = MS_NINT(p->x - scale*.5*symbol->sizex);
            offset_y = MS_NINT(p->y - scale*.5*symbol->sizey);

            PDF_moveto(pdf, MS_NINT(scale*symbol->points[0].x + offset_x),MS_NINT((scale*symbol->points[0].y + offset_y)));
            for(j=0;j < symbol->numpoints;j++) {
                PDF_lineto(pdf, MS_NINT(scale*symbol->points[j].x + offset_x),MS_NINT((scale*symbol->points[j].y + offset_y)));
            }

            if(symbol->filled) { /* if filled */
                PDF_fill_stroke(pdf);
            } else  { /* NOT filled */
                PDF_stroke(pdf);
            } /* end if-then-else */
            break;
        default:
            break;
    } /* end switch statement */

    return;
}

void billboardPDF(PDF *pdf, shapeObj *shape, labelObj *label)
{
  int i;
  shapeObj temp;

  msInitShape(&temp);
  msAddLine(&temp, &shape->line[0]);

  if(label->backgroundshadowcolor >= 0) {
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x += label->backgroundshadowsizex;
      temp.line[0].point[i].y += label->backgroundshadowsizey;
    }
//    msImageFilledPolygon(pdf, &temp, label->backgroundshadowcolor);
    for(i=0; i<temp.line[0].numpoints; i++) {
      temp.line[0].point[i].x -= label->backgroundshadowsizex;
      temp.line[0].point[i].y -= label->backgroundshadowsizey;
    }
  }

//  msImageFilledPolygonPDF(pdf, &temp, label->backgroundcolor);

  msFreeShape(&temp);
}

int msDrawLabelCachePDF(PDF *pdf, mapObj *map, hashTableObj fontHash)
{
    pointObj p;
    int i, j, l;

    rectObj r;

    labelObj label;
    labelCacheMemberObj *cachePtr=NULL;
    classObj *classPtr=NULL;
    layerObj *layerPtr=NULL;

    int draw_marker;
    int marker_width, marker_height;
    int marker_offset_x, marker_offset_y;
    rectObj marker_rect;

    for(l=map->labelcache.numlabels-1; l>=0; l--) {

        cachePtr = &(map->labelcache.labels[l]); /* point to right spot in cache */

        layerPtr = &(map->layers[cachePtr->layeridx]); /* set a couple of other pointers, avoids nasty references */
        classPtr = &(layerPtr->class[cachePtr->classidx]);

        if(!cachePtr->string)
            continue; /* not an error, just don't want to do anything */

        if(strlen(cachePtr->string) == 0)
            continue; /* not an error, just don't want to do anything */

        label = classPtr->label;
        label.sizescaled = label.size = cachePtr->size;
        label.angle = cachePtr->angle;

        if(msGetLabelSize(cachePtr->string, &label, &r, &(map->fontset)) == -1)
            return(-1);

        if(label.autominfeaturesize && ((r.maxx-r.minx) > cachePtr->featuresize))
            continue; /* label too large relative to the feature */

        draw_marker = marker_offset_x = marker_offset_y = 0; /* assume no marker */
        if((layerPtr->type == MS_LAYER_ANNOTATION || layerPtr->type == MS_LAYER_POINT) && (classPtr->color >= 0 || classPtr->outlinecolor > 0)) { /* there *is* a marker */

            msGetMarkerSize(&map->symbolset, classPtr, &marker_width, &marker_height);
            marker_offset_x = MS_NINT(marker_width/2.0);
            marker_offset_y = MS_NINT(marker_height/2.0);

            marker_rect.minx = MS_NINT(cachePtr->point.x - .5 * marker_width);
            marker_rect.miny = MS_NINT(cachePtr->point.y - .5 * marker_height);
            marker_rect.maxx = marker_rect.minx + (marker_width-1);
            marker_rect.maxy = marker_rect.miny + (marker_height-1);

            if(layerPtr->type == MS_LAYER_ANNOTATION) draw_marker = 1; /* actually draw the marker */
        }

        if(label.position == MS_AUTO) {

            if(layerPtr->type == MS_LAYER_LINE) {
                int position = MS_UC;

                for(j=0; j<2; j++) { /* Two angles or two positions, depending on angle. Steep angles will use the angle approach, otherwise we'll rotate between UC and LC. */

                    msFreeShape(cachePtr->poly);
                    cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

                    if(j == 1) {
                        if(fabs(cos(label.angle)) < LINE_VERT_THRESHOLD)
                            label.angle += 180.0;
                        else
                            position = MS_LC;
                    }

                    p = get_metrics(&(cachePtr->point), position, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);

                    if(draw_marker)
                        msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

                    if(!label.partials) { // check against image first
                        if(labelInImage(map->width, map->height, cachePtr->poly, label.buffer) == MS_FALSE) {
                            cachePtr->status = MS_FALSE;
                            continue; // next angle
                        }
                    }

                    for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
                        if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
                            if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                                cachePtr->status = MS_FALSE;
                                break;
                            }
                        }
                    }

                    if(!cachePtr->status)
                        continue; // next angle

                    for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
                        if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

                            if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
                                cachePtr->status = MS_FALSE;
                                break;
                            }

                            if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                                cachePtr->status = MS_FALSE;
                                break;
                            }
                        }
                    }

                    if(cachePtr->status) // found a suitable place for this label
                        break;

                } // next angle

            } else {
                for(j=0; j<=7; j++) { /* loop through the outer label positions */

                    msFreeShape(cachePtr->poly);
                    cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

                    p = get_metrics(&(cachePtr->point), j, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);

                    if(draw_marker)
                        msRectToPolygon(marker_rect, cachePtr->poly); // save marker bounding polygon

                    if(!label.partials) { // check against image first
                        if(labelInImage(map->width, map->height, cachePtr->poly, label.buffer) == MS_FALSE) {
                            cachePtr->status = MS_FALSE;
                            continue; // next position
                        }
                    }

                    for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
                        if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
                            if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                                cachePtr->status = MS_FALSE;
                                break;
                            }
                        }
                    }

                    if(!cachePtr->status)
                        continue; // next position

                    for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered labels
                        if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */

                            if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string,map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
                                cachePtr->status = MS_FALSE;
                                break;
                            }

                            if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                                cachePtr->status = MS_FALSE;
                                break;
                            }
                        }
                    }

                    if(cachePtr->status) // found a suitable place for this label
                        break;
                } // next position
            }

            if(label.force) cachePtr->status = MS_TRUE; /* draw in spite of collisions based on last position, need a *best* position */

        } else {

            cachePtr->status = MS_TRUE; /* assume label *can* be drawn */

            if(label.position == MS_CC) // don't need the marker_offset
                p = get_metrics(&(cachePtr->point), label.position, r, label.offsetx, label.offsety, label.angle, label.buffer, cachePtr->poly);
            else
                p = get_metrics(&(cachePtr->point), label.position, r, (marker_offset_x + label.offsetx), (marker_offset_y + label.offsety), label.angle, label.buffer, cachePtr->poly);

            if(draw_marker)
                msRectToPolygon(marker_rect, cachePtr->poly); /* save marker bounding polygon, part of overlap tests */

            if(!label.force) { // no need to check anything else

                if(!label.partials) {
                    if(labelInImage(map->width, map->height, cachePtr->poly, label.buffer) == MS_FALSE)
                        cachePtr->status = MS_FALSE;
                }

                if(!cachePtr->status)
                    continue; // next label

                for(i=0; i<map->labelcache.nummarkers; i++) { // compare against points already drawn
                    if(l != map->labelcache.markers[i].id) { // labels can overlap their own marker
                        if(intersectLabelPolygons(map->labelcache.markers[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                            cachePtr->status = MS_FALSE;
                            break;
                        }
                    }
                }

                if(!cachePtr->status)
                    continue; // next label

                for(i=l+1; i<map->labelcache.numlabels; i++) { // compare against rendered label
                    if(map->labelcache.labels[i].status == MS_TRUE) { /* compare bounding polygons and check for duplicates */
                        if((label.mindistance != -1) && (cachePtr->classidx == map->labelcache.labels[i].classidx) && (strcmp(cachePtr->string, map->labelcache.labels[i].string) == 0) && (dist(cachePtr->point, map->labelcache.labels[i].point) <= label.mindistance)) { /* label is a duplicate */
                            cachePtr->status = MS_FALSE;
                            break;
                        }

                        if(intersectLabelPolygons(map->labelcache.labels[i].poly, cachePtr->poly) == MS_TRUE) { /* polys intersect */
                            cachePtr->status = MS_FALSE;
                            break;
                        }
                    }
                }
            }
        } /* end position if-then-else */

            /* msImagePolyline(img, cachePtr->poly, 1); */

        if(!cachePtr->status)
            continue; /* next label */

        if(draw_marker) { /* need to draw a marker */
            colorObj *fcp, *bcp, *ocp;
            colorObj *fcop,*bcop,*ocop;
            colorObj  fc,   bc,   oc;
            colorObj  fco,   bco,   oco;

/// Do color initialization stuff
            fcp = bcp = ocp = fcop = bcop = ocop = NULL;
            getRgbColor(map, classPtr->color,           &fc.red, &fc.green, &fc.blue);
            getRgbColor(map, classPtr->backgroundcolor, &bc.red, &bc.green, &bc.blue);
            getRgbColor(map, classPtr->outlinecolor,    &oc.red, &oc.green, &oc.blue);
            getRgbColor(map, classPtr->overlaycolor,           &fco.red, &fco.green, &fco.blue);
            getRgbColor(map, classPtr->overlaybackgroundcolor, &bco.red, &bco.green, &bco.blue);
            getRgbColor(map, classPtr->overlayoutlinecolor,    &oco.red, &oco.green, &oco.blue);

            if (classPtr->color!=-1)                  fcp = &fc;
            if (classPtr->backgroundcolor!=-1)        bcp = &bc;
            if (classPtr->outlinecolor!=-1)            ocp = &oc;
            if (classPtr->overlaycolor!=-1)          fcop = &fco;
            if (classPtr->overlaybackgroundcolor!=-1)bcop = &bco;
            if (classPtr->overlayoutlinecolor!=-1)    ocop = &oco;
/// end of color initialization
            msDrawMarkerSymbolPDF(&map->symbolset, pdf, &(cachePtr->point), classPtr->symbol, fcp, bcp, ocp, classPtr->sizescaled, fontHash);
            if(classPtr->overlaysymbol >= 0)
                msDrawMarkerSymbolPDF(&map->symbolset, pdf, &(cachePtr->point), classPtr->overlaysymbol, fcop, bcop, ocop, classPtr->overlaysizescaled, fontHash);
        }

//        if(label.backgroundcolor >= 0)
//            billboard(img, cachePtr->poly, &label);

        draw_text_PDF(pdf, p, cachePtr->string, &label, &(map->fontset), map, fontHash); /* actually draw the label */

    } /* next in cache */

  return(0);
}


PDF *
initializeDocument()
{
        /* First we need to make the PDF */
    PDF *pdf = PDF_new();
    PDF_open_file (pdf,"-");/*This creates the file in-memory, not on disk */
    PDF_set_info  (pdf, "Creator", "pdfmaprequest");
    PDF_set_info  (pdf, "Author", "Market Insite Group");
    PDF_set_info  (pdf, "Title", "Market Insite PDF Map");
    return pdf;
}

void msPolylinePDF(PDF *pdf, shapeObj *p, colorObj  *c, double width)
{
    int i, j;

    if (width){
        PDF_setlinejoin(pdf,1);
        PDF_setlinewidth(pdf,(float)width);
    }

    if (c){
        PDF_setrgbcolor(pdf,(float)c->red/255,(float)c->green/255,(float)c->blue/255);
    }
    for (i = 0; i < p->numlines; i++){
        if (p->line[i].numpoints){
            PDF_moveto(pdf,p->line[i].point[0].x, p->line[i].point[0].y);
        }
        for(j=1; j<p->line[i].numpoints; j++){
            PDF_lineto(pdf,p->line[i].point[j].x, p->line[i].point[j].y);
        }
    }
    PDF_stroke(pdf);
//    PDF_setlinejoin(pdf,0);
    PDF_setlinewidth(pdf,1);

}

void msFilledPolygonPDF(PDF *pdf, shapeObj *p, colorObj  *c)
{
    int i, j;
    if (c){
        PDF_setrgbcolor(pdf,(float)c->red/255,(float)c->green/255,(float)c->blue/255);
    }
    for (i = 0; i < p->numlines; i++){
        if (p->line[i].numpoints){
            PDF_moveto(pdf,p->line[i].point[0].x, p->line[i].point[0].y);
        }
        for(j=1; j<p->line[i].numpoints; j++){
            PDF_lineto(pdf,p->line[i].point[j].x, p->line[i].point[j].y);
        }
    }
    PDF_closepath_fill_stroke(pdf);

}

/* ------------------------------------------------------------------------------- */
/*       Draw a line symbol of the specified size and color                        */
/* ------------------------------------------------------------------------------- */
void msDrawLineSymbolPDF(symbolSetObj *symbolset, PDF *pdf, shapeObj *p, int sy, colorObj *fc, colorObj *bc, colorObj *oc, double sz)
{
    if(p->numlines <= 0)
        return;

    if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
        return;

    if(!fc) /* invalid color */
        return;

    if(sz < 1) /* size too small */
        return;

    msPolylinePDF(pdf, p, fc, sz);
    return;
}

int msDrawLabelPDF(PDF *pdf, pointObj labelPnt, char *string, labelObj *label, fontSetObj *fontset, mapObj *map, hashTableObj fontHash)
{

    if(!string)
        return(0); /* not an error, just don't want to do anything */

    if(strlen(string) == 0)
        return(0); /* not an error, just don't want to do anything */

    if(label->position != MS_XY) {
        pointObj p;
        rectObj r;

        if(msGetLabelSize(string, label, &r, fontset) == -1) return(-1);
        p = get_metrics(&labelPnt, label->position, r, label->offsetx, label->offsety, label->angle, 0, NULL);
        draw_text_PDF(pdf, p, string, label, fontset, map, fontHash); /* actually draw the label */
    } else {
        labelPnt.x += label->offsetx;
        labelPnt.y += label->offsety;
        draw_text_PDF(pdf, labelPnt, string, label, fontset, map, fontHash); /* actually draw the label */
    }

    return(0);
}

/* ------------------------------------------------------------------------------- */
/*       Draw a shade symbol of the specified size and color                       */
/* ------------------------------------------------------------------------------- */
void msDrawShadeSymbolPDF(symbolSetObj *symbolset, PDF *pdf, shapeObj *p, int sy, colorObj *fc, colorObj *bc, colorObj *oc, double sz)
{

    if(p->numlines <= 0)
        return;

    if(sy > symbolset->numsymbols || sy < 0) /* no such symbol, 0 is OK */
        return;

    if(sz < 1) /* size too small */
        return;

    if(fc)
        msFilledPolygonPDF(pdf, p, fc);

    if(oc)
        msPolylinePDF(pdf, p, oc, sz);
    return;
}

int msDrawShapePDF(mapObj *map, layerObj *layer, shapeObj *shape, PDF *pdf, int overlay, hashTableObj fontHash)
{
    int i,j,c;
    rectObj cliprect;
    pointObj annopnt, *point;
    double angle, length, scalefactor=1.0;
    colorObj *fcp, *bcp, *ocp;
    colorObj *fcop,*bcop,*ocop;
    colorObj  fc,   bc,   oc;
    colorObj  fco,   bco,   oco;

    cliprect.minx = map->extent.minx - 2*map->cellsize; // set clipping rectangle just a bit larger than the map extent
    cliprect.miny = map->extent.miny - 2*map->cellsize;
    cliprect.maxx = map->extent.maxx + 2*map->cellsize;
    cliprect.maxy = map->extent.maxy + 2*map->cellsize;

    c = shape->classindex;

/// Do color initialization stuff
    fcp = bcp = ocp = fcop = bcop = ocop = NULL;
    getRgbColor(map, layer->class[c].color,           &fc.red, &fc.green, &fc.blue);
    getRgbColor(map, layer->class[c].backgroundcolor, &bc.red, &bc.green, &bc.blue);
    getRgbColor(map, layer->class[c].outlinecolor,    &oc.red, &oc.green, &oc.blue);
    getRgbColor(map, layer->class[c].overlaycolor,           &fco.red, &fco.green, &fco.blue);
    getRgbColor(map, layer->class[c].overlaybackgroundcolor, &bco.red, &bco.green, &bco.blue);
    getRgbColor(map, layer->class[c].overlayoutlinecolor,    &oco.red, &oco.green, &oco.blue);

    if (layer->class[c].color!=-1)                  fcp = &fc;
    if (layer->class[c].backgroundcolor!=-1)        bcp = &bc;
    if (layer->class[c].outlinecolor!=-1)            ocp = &oc;
    if (layer->class[c].overlaycolor!=-1)          fcop = &fco;
    if (layer->class[c].overlaybackgroundcolor!=-1)bcop = &bco;
    if (layer->class[c].overlayoutlinecolor!=-1)    ocop = &oco;
/// end of color initialization

    if(layer->symbolscale > 0 && map->scale > 0) scalefactor = layer->symbolscale/map->scale;

    if(layer->sizeunits != MS_PIXELS) {
        layer->class[c].sizescaled = layer->class[c].size * (inchesPerUnitPDF[layer->sizeunits]/inchesPerUnitPDF[map->units]);
        layer->class[c].overlaysizescaled = layer->class[c].overlaysize * (inchesPerUnitPDF[layer->sizeunits]/inchesPerUnitPDF[map->units]);
    } else {
        layer->class[c].sizescaled = MS_NINT(layer->class[c].size * scalefactor);
        layer->class[c].sizescaled = MS_MAX(layer->class[c].sizescaled, layer->class[c].minsize);
        layer->class[c].sizescaled = MS_MIN(layer->class[c].sizescaled, layer->class[c].maxsize);
        layer->class[c].overlaysizescaled = layer->class[c].sizescaled - (layer->class[c].size - layer->class[c].overlaysize);
            // layer->class[c].overlaysizescaled = MS_NINT(layer->class[c].overlaysize * scalefactor);
        layer->class[c].overlaysizescaled = MS_MAX(layer->class[c].overlaysizescaled, layer->class[c].overlayminsize);
        layer->class[c].overlaysizescaled = MS_MIN(layer->class[c].overlaysizescaled, layer->class[c].overlaymaxsize);
    }

    if(layer->class[c].sizescaled == 0) return(MS_SUCCESS);

#if defined (USE_GD_FT) || defined (USE_GD_TTF)
    if(layer->class[c].label.type == MS_TRUETYPE) {
        layer->class[c].label.sizescaled = MS_NINT(layer->class[c].label.size * scalefactor);
        layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
        layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
    }
#endif

#ifdef USE_PROJ
    if((layer->projection.numargs > 0) && (map->projection.numargs > 0))
        msProjectShape(&layer->projection, &map->projection, shape);
#endif

    switch(layer->type) {
        case MS_LAYER_ANNOTATION:
            if(!shape->text) return(MS_SUCCESS); // nothing to draw

            switch(shape->type) {
                case(MS_SHAPE_LINE):

                    if(layer->transform) {
                        msClipPolylineRect(shape, cliprect);
                        if(shape->numlines == 0) return(MS_SUCCESS);
                        msTransformShape(shape, map->extent, map->cellsize);
                    }

                    if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {

                        if(layer->labelangleitemindex != -1)
                            layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                        if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                            layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? (layer->symbolscale/map->scale):1);
                            layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                            layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                        }

                        if(layer->class[c].label.autoangle)
                            layer->class[c].label.angle = angle;

                        if(layer->labelcache)
                            msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
                        else {
                            if(layer->class[c].color != -1) {
                                 msDrawMarkerSymbolPDF(&map->symbolset, pdf, &annopnt, layer->class[c].symbol, fcp,bcp,ocp, layer->class[c].sizescaled, fontHash);
                            }
                            else {
                                if(layer->class[c].overlaysymbol >= 0){
                                    msDrawMarkerSymbolPDF(&map->symbolset, pdf, &annopnt, layer->class[c].overlaysymbol,fcop,bcop,ocop, layer->class[c].overlaysizescaled, fontHash);
                                }
                            }
                            msDrawLabelPDF(pdf, annopnt, shape->text, &(layer->class[c].label), &map->fontset, map, fontHash);
                        }
                    }
                    break;
                case(MS_SHAPE_POLYGON):
                    if(layer->transform) {
                        msClipPolygonRect(shape, cliprect);
                        if(shape->numlines == 0) return(MS_SUCCESS);
                        msTransformShape(shape, map->extent, map->cellsize);
                    }

                    if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {

                        if(layer->labelangleitemindex != -1)
                            layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                        if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                            layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                            layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                            layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                        }

                        if(layer->labelcache){
                            msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
                        }
                        else {
                            if(layer->class[c].color != -1) {
                                msDrawMarkerSymbolPDF(&map->symbolset, pdf, &annopnt, layer->class[c].symbol,fcp,bcp,ocp, layer->class[c].sizescaled, fontHash);
                                if(layer->class[c].overlaysymbol >= 0){
                                    msDrawMarkerSymbolPDF(&map->symbolset, pdf, &annopnt, layer->class[c].overlaysymbol, fcop,bcop,ocop, layer->class[c].overlaysizescaled, fontHash);
                                }
                            }
                            msDrawLabelPDF(pdf, annopnt, shape->text, &(layer->class[c].label), &map->fontset, map, fontHash);
                        }
                    }
                    break;
                default: // points and anything with out a proper type
                    for(j=0; j<shape->numlines;j++) {
                        for(i=0; i<shape->line[j].numpoints;i++) {

                            point = &(shape->line[j].point[i]);

                            if(layer->transform) {
                                if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
                                point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
                                point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
                            }

                            if(layer->labelangleitemindex != -1)
                                layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                            if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                                layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                                layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                                layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                            }

                            if(shape->text) {
                                if(layer->labelcache)
                                    msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
                                else {
                                    if(layer->class[c].color != -1) {
                                        msDrawMarkerSymbolPDF(&map->symbolset, pdf, point, layer->class[c].symbol,fcp,bcp,ocp, layer->class[c].sizescaled, fontHash);
                                        if(layer->class[c].overlaysymbol >= 0){
                                            msDrawMarkerSymbolPDF(&map->symbolset, pdf, point, layer->class[c].overlaysymbol, fcop,bcop,ocop, layer->class[c].overlaysizescaled, fontHash);
                                        }
                                    }
                                    msDrawLabelPDF(pdf, *point, shape->text, &layer->class[c].label, &map->fontset, map, fontHash);
                                }
                            }
                        }
                    }
            }
            break;

        case MS_LAYER_POINT:

            for(j=0; j<shape->numlines;j++) {
                for(i=0; i<shape->line[j].numpoints;i++) {

                    point = &(shape->line[j].point[i]);

                    if(layer->transform) {
                        if(!msPointInRect(point, &map->extent)) return(MS_SUCCESS);
                        point->x = MS_MAP2IMAGE_X(point->x, map->extent.minx, map->cellsize);
                        point->y = MS_MAP2IMAGE_Y(point->y, map->extent.maxy, map->cellsize);
                    }

                    msDrawMarkerSymbolPDF(&map->symbolset, pdf, point, layer->class[c].symbol, fcp,bcp,ocp, layer->class[c].sizescaled, fontHash);

                    if(overlay && layer->class[c].overlaysymbol >= 0) {
                        msDrawMarkerSymbolPDF(&map->symbolset, pdf, point, layer->class[c].overlaysymbol, fcop,bcop,ocop, layer->class[c].overlaysizescaled, fontHash);
                    }

                    if(shape->text) {
                        if(layer->labelangleitemindex != -1)
                            layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                        if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                            layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                            layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                            layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                        }

                        if(layer->labelcache)
                            msAddLabel(map, layer->index, c, shape->tileindex, shape->index, *point, shape->text, -1);
                        else
                            msDrawLabelPDF(pdf, *point, shape->text, &layer->class[c].label, &map->fontset, map, fontHash);
                    }
                }
            }
            break;

        case MS_LAYER_LINE:
            if(shape->type != MS_SHAPE_POLYGON && shape->type != MS_SHAPE_LINE){
                msSetError(MS_MISCERR, "Only polygon or line shapes can be drawn using a line layer definition.", "msDrawShape()");
                return(MS_FAILURE);
            }

            if(layer->transform) {
                msClipPolylineRect(shape, cliprect);
                if(shape->numlines == 0) return(MS_SUCCESS);
                msTransformShape(shape, map->extent, map->cellsize);
            }

            msDrawLineSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].symbol, fcp,bcp,ocp, layer->class[c].sizescaled);
            if(overlay && layer->class[c].overlaysymbol >= 0)
                msDrawLineSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].overlaysymbol, fcop,bcop,ocop, layer->class[c].overlaysizescaled);

            if(shape->text) {
                if(msPolylineLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize, &angle, &length) == MS_SUCCESS) {
                    if(layer->labelangleitemindex != -1)
                        layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                    if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                        layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                        layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                        layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                    }

                    if(layer->class[c].label.autoangle)
                        layer->class[c].label.angle = angle;

                    if(layer->labelcache)
                        msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, length);
                    else
                        msDrawLabelPDF(pdf, annopnt, shape->text, &layer->class[c].label, &map->fontset, map, fontHash);
                }
            }
            break;

        case MS_LAYER_POLYLINE:
            if(shape->type != MS_SHAPE_POLYGON){
                msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a polyline layer definition.", "msDrawShape()");
                return(MS_FAILURE);
            }

            if(layer->transform) {
                msClipPolygonRect(shape, cliprect);
                if(shape->numlines == 0) return(MS_SUCCESS);
                msTransformShape(shape, map->extent, map->cellsize);
            }

            msDrawLineSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].symbol, fcp,bcp,ocp, layer->class[c].sizescaled);
            if(overlay && layer->class[c].overlaysymbol >= 0)
                msDrawLineSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].overlaysymbol,fcop,bcop,ocop, layer->class[c].overlaysizescaled);

            if(shape->text) {
                if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {
                    if(layer->labelangleitemindex != -1)
                        layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                    if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                        layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                        layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                        layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                    }

                    if(layer->labelcache)
                        msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, -1);
                    else
                        msDrawLabelPDF(pdf, annopnt, shape->text, &layer->class[c].label, &map->fontset, map, fontHash);
                }
            }
            break;
        case MS_LAYER_POLYGON:
            if(shape->type != MS_SHAPE_POLYGON){
                msSetError(MS_MISCERR, "Only polygon shapes can be drawn using a polygon layer definition.", "msDrawShape()");
                return(MS_FAILURE);
            }

            if(layer->transform) {
                msClipPolygonRect(shape, cliprect);
                if(shape->numlines == 0) return(MS_SUCCESS);
                msTransformShape(shape, map->extent, map->cellsize);
            }

            msDrawShadeSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].symbol, fcp,bcp,ocp, layer->class[c].sizescaled);
            if(overlay && layer->class[c].overlaysymbol >= 0)
                msDrawShadeSymbolPDF(&map->symbolset, pdf, shape, layer->class[c].overlaysymbol, fcop,bcop,ocop, layer->class[c].overlaysizescaled);

            if(shape->text) {
                if(msPolygonLabelPoint(shape, &annopnt, layer->class[c].label.minfeaturesize) == MS_SUCCESS) {
                    if(layer->labelangleitemindex != -1)
                        layer->class[c].label.angle = atof(shape->values[layer->labelangleitemindex]);

                    if((layer->labelsizeitemindex != -1) && (layer->class[c].label.type == MS_TRUETYPE)) {
                        layer->class[c].label.sizescaled = atoi(shape->values[layer->labelsizeitemindex])*((layer->symbolscale > 0) ? scalefactor:1);
                        layer->class[c].label.sizescaled = MS_MAX(layer->class[c].label.sizescaled, layer->class[c].label.minsize);
                        layer->class[c].label.sizescaled = MS_MIN(layer->class[c].label.sizescaled, layer->class[c].label.maxsize);
                    }

                    if(layer->labelcache)
                        msAddLabel(map, layer->index, c, shape->tileindex, shape->index, annopnt, shape->text, -1);
                    else
                        msDrawLabelPDF(pdf, annopnt, shape->text, &layer->class[c].label, &map->fontset, map, fontHash);
                }
            }
            break;
        default:
            msSetError(MS_MISCERR, "Unknown layer type.", "msDrawShape()");
            return(MS_FAILURE);
    }

    return(MS_SUCCESS);
}

int msDrawLayerPDF(mapObj *map, layerObj *layer, PDF *pdf, hashTableObj fontHash)
{
    int status;
    char annotate=MS_TRUE, cache=MS_FALSE;
    shapeObj shape;
    rectObj searchrect;

    featureListNodeObjPtr shpcache=NULL, current=NULL;

    if(layer->connectiontype == MS_WMS) return(MS_SUCCESS); // return(msDrawWMSLayer(map, layer, img));
    if(layer->type == MS_LAYER_RASTER)  return(MS_SUCCESS); // return(msDrawRasterLayer(map, layer, img));
    if(layer->type == MS_LAYER_QUERY)   return(MS_SUCCESS);
    if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) return(MS_SUCCESS);

    if(msEvalContext(map, layer->requires) == MS_FALSE) return(MS_SUCCESS);
    annotate = msEvalContext(map, layer->labelrequires);

    if(map->scale > 0) {
        if((layer->maxscale > 0) && (map->scale > layer->maxscale))
            return(MS_SUCCESS);
        if((layer->minscale > 0) && (map->scale <= layer->minscale))
            return(MS_SUCCESS);
        if((layer->labelmaxscale != -1) && (map->scale >= layer->labelmaxscale))
            annotate = MS_FALSE;
        if((layer->labelminscale != -1) && (map->scale < layer->labelminscale))
            annotate = MS_FALSE;
    }

        // open this layer
    status = msLayerOpen(layer, map->shapepath);
    if(status != MS_SUCCESS) return(MS_FAILURE);

        // build item list
    status = msLayerWhichItems(layer, MS_TRUE, annotate);
    if(status != MS_SUCCESS) return(MS_FAILURE);

        // identify target shapes
    searchrect = map->extent;
#ifdef USE_PROJ
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
        msProjectRect(&map->projection, &layer->projection, &searchrect); // project the searchrect to source coords
#endif
    status = msLayerWhichShapes(layer, searchrect);
    if(status == MS_DONE) { // no overlap
        msLayerClose(layer);
        return(MS_SUCCESS);
    } else if(status != MS_SUCCESS)
        return(MS_FAILURE);

        // step through the target shapes
    msInitShape(&shape);

    if(layer->type == MS_LAYER_LINE || layer->type == MS_LAYER_POLYLINE) cache = MS_TRUE;
// only line/polyline layers need to (potentially) be cached with overlayed symbols
    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {

        shape.classindex = msShapeGetClass(layer, &shape);
        if((shape.classindex == -1) || (layer->class[shape.classindex].status == MS_OFF)) {
            msFreeShape(&shape);
            continue;
        }

            // With 'STYLEITEM AUTO', we will have the datasource fill the class'
            // style parameters for this shape.
        if (layer->styleitem && strcasecmp(layer->styleitem, "AUTO") == 0)
        {
            if (msLayerGetAutoStyle(map, layer, &(layer->class[shape.classindex]),
                                    shape.tileindex, shape.index) != MS_SUCCESS)
            {
                return MS_FAILURE;
            }

                // Dynamic class update may have extended the color palette...
// For the PDF there are no palette issues
//        if (!msUpdatePalette(img, &(map->palette)))
//            return MS_FAILURE;

                // __TODO__ For now, we can't cache features with 'AUTO'
                // style
            cache = MS_FALSE;
        }

        if(annotate && (layer->class[shape.classindex].text.string || layer->labelitem) && layer->class[shape.classindex].label.size != -1)
            shape.text = msShapeGetAnnotation(layer, &shape);

        status = msDrawShapePDF(map, layer, &shape, pdf, !cache, fontHash); // if caching we DON'T want to do overlays at this time
        if(status != MS_SUCCESS) {
            msLayerClose(layer);
            return(MS_FAILURE);
        }

        if(shape.numlines == 0) { // once clipped the shape didn't need to be drawn
            msFreeShape(&shape);
            continue;
        }

        if(cache && layer->class[shape.classindex].overlaysymbol >= 0)
            if(insertFeatureList(&shpcache, &shape) == NULL) return(MS_FAILURE); // problem adding to the cache

        msFreeShape(&shape);
    }

    if(status != MS_DONE) return(MS_FAILURE);

    if(shpcache) {
        int c;
        colorObj overlaycolor, overlaybackgroundcolor, overlayoutlinecolor;

        for(current=shpcache; current; current=current->next) {

            c = current->shape.classindex;
            getRgbColor(map, layer->class[c].overlaycolor,
                        &overlaycolor.red,
                        &overlaycolor.green,
                        &overlaycolor.blue);
            getRgbColor(map, layer->class[c].overlaybackgroundcolor,
                        &overlaybackgroundcolor.red,
                        &overlaybackgroundcolor.green,
                        &overlaybackgroundcolor.blue);
            getRgbColor(map, layer->class[c].overlayoutlinecolor,
                        &overlayoutlinecolor.red,
                        &overlayoutlinecolor.green,
                        &overlayoutlinecolor.blue);

            msDrawLineSymbolPDF(&map->symbolset, pdf, &current->shape, layer->class[c].overlaysymbol,
                                &overlaycolor, &overlaybackgroundcolor, &overlayoutlinecolor,
                                layer->class[c].overlaysizescaled);
        }

        freeFeatureList(shpcache);
        shpcache = NULL;
    }

    msLayerClose(layer);
    return(MS_SUCCESS);
}


int msLoadFontSetPDF(fontSetObj *fontset, PDF *pdf)
{

    FILE *stream;
    char buffer[MS_BUFFER_LENGTH];
    char alias[64], file1[MS_PATH_LENGTH], file2[MS_PATH_LENGTH];
    char *path, *fullPath;
    int i;

    path = getPath(fontset->filename);

    stream = fopen(fontset->filename, "r");
    if(!stream) {
        sprintf(ms_error.message, "Error opening fontset %s.", fontset->filename);
        msSetError(MS_IOERR, ms_error.message, "msLoadFontsetPDF()");
        return(-1);
    }

    i = 0;
    while(fgets(buffer, MS_BUFFER_LENGTH, stream)) { /* while there's something to load */
        char *fontParam;

        if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == ' ')
            continue; /* skip comments and blank lines */

        fullPath = NULL;
        sscanf(buffer,"%s %s", alias,  file1);

        fullPath = file1;
        if(file1[0] != '/') { /* already full path */
            sprintf(file2, "%s%s", path, file1);
            fullPath = file2;
        }

            // ok we have the alias and the full name
        fontParam = (char *)malloc(sizeof(char)*(strlen(fullPath)+strlen(alias)+3));
        sprintf(fontParam,"%s==%s",alias,fullPath);
        PDF_set_parameter(pdf, "FontOutline", fontParam);
        free(fontParam);
        i++;
    }

    fclose(stream); /* close the file */
    free(path);

    return(0);
}



PDF *msDrawMapPDF(mapObj *map, PDF *pdf)
{
    int i;
    layerObj *lp=NULL;
    int status;
    hashTableObj fontHash;


    fontHash = msCreateHashTable();
    if(map->width == -1 && map->height == -1) {
        msSetError(MS_MISCERR, "Image dimensions not specified.", "msDrawMap()");
        return(NULL);
    }

    if(map->width == -1 ||  map->height == -1)
        if(msAdjustImage(map->extent, &map->width, &map->height) == -1)
            return(NULL);

    if (!pdf){
        pdf = initializeDocument();

        PDF_begin_page(pdf, map->width, map->height);
        PDF_translate(pdf,0,map->height);
        PDF_scale(pdf, 1,-1);
        PDF_set_value(pdf,"textrendering",2);
    }

    msLoadFontSetPDF((&(map->fontset)), pdf);
    if (map->imagecolor.red != -1){
        PDF_setrgbcolor(pdf,(float)map->imagecolor.red/255,(float)map->imagecolor.green/255,(float)map->imagecolor.blue/255);
        PDF_rect(pdf,0,0,map->width,map->height);
        PDF_fill_stroke(pdf);
    }

    if(!pdf) {
        msSetError(MS_GDERR, "Unable to initialize pdfPage.", "msDrawMap()");
        return(NULL);
    }


// ok we don't need to do this.. pdf has its one color palette
//  if(msLoadPalette(img, &(map->palette), map->imagecolor) == -1)
//    return(NULL);

    map->cellsize = msAdjustExtent(&(map->extent), map->width, map->height);
    status = msCalculateScale(map->extent, map->units, map->width, map->height, map->resolution, &map->scale);
    if(status != MS_SUCCESS) return(NULL);

    for(i=0; i<map->numlayers; i++) {

        lp = &(map->layers[i]);

        if(lp->postlabelcache) // wait to draw
            continue;

        status = msDrawLayerPDF(map, lp, pdf,fontHash);
        if(status != MS_SUCCESS) return(NULL);
    }

// ignore these for now
//  if(map->scalebar.status == MS_EMBED && !map->scalebar.postlabelcache)
//    msEmbedScalebar(map, img);

//  if(map->legend.status == MS_EMBED && !map->legend.postlabelcache)
//    msEmbedLegend(map, img);

  if(msDrawLabelCachePDF(pdf, map, fontHash) == -1)
    return(NULL);

    for(i=0; i<map->numlayers; i++) { // for each layer, check for postlabelcache layers

        lp = &(map->layers[i]);

        if(!lp->postlabelcache)
            continue;

        status = msDrawLayerPDF(map, lp, pdf, fontHash);
        if(status != MS_SUCCESS) return(NULL);
    }

//  if(map->scalebar.status == MS_EMBED && map->scalebar.postlabelcache)
//    msEmbedScalebar(map, img);

//  if(map->legend.status == MS_EMBED && map->legend.postlabelcache)
//    msEmbedLegend(map, img);

    msFreeHashTable(fontHash);

    return(pdf);
}
