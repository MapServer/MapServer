/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  PNG related functions for use with RGBA imagetypes
 * Author:   Thomas Bonfort
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */
/* 
 * derivations from pngquant and ppmquant
 * 
 ** pngquant.c - quantize the colors in an alphamap down to a specified number
 **
 ** Copyright (C) 1989, 1991 by Jef Poskanzer.
 ** Copyright (C) 1997, 2000, 2002 by Greg Roelofs; based on an idea by
 **                                Stefan Schneider.
 **
 ** Permission to use, copy, modify, and distribute this software and its
 ** documentation for any purpose and without fee is hereby granted, provided
 ** that the above copyright notice appear in all copies and that both that
 ** copyright notice and this permission notice appear in supporting
 ** documentation.  This software is provided "as is" without express or
 ** implied warranty.
 */

#ifdef USE_RGBA_PNG
#include "mapserver.h"
#include <assert.h>
#include <stdlib.h>

#define PNG_SETJMP_NOT_SUPPORTED 1
#include "png.h"
#include <setjmp.h>




typedef struct {
    unsigned char b,g,r,a;
} apixel;

typedef struct {
    unsigned char r,g,b;
} apalettepixel;


static void
ctxWriteFunc (png_structp png_ptr, png_bytep data, png_size_t length)
{
    gdIOCtx *ctx=(gdIOCtx *) png_get_io_ptr (png_ptr);
    ctx->putBuf(ctx,data,length);
}

static void
ctxFlushFunc (png_structp png_ptr)
{
}

typedef struct _ms_png_info {
    int width;
    int height;
    void *png_ptr;
    void *info_ptr;
    apalettepixel palette[256];
    unsigned char trans[256];
    unsigned char *indexed_data;
    unsigned char **row_pointers;
    jmp_buf jmpbuf;
    int interlaced;
    int sample_depth;
    int num_palette;
    int num_trans;
} ms_png_info;

static void ms_png_error_handler(png_structp png_ptr, png_const_charp msg)
{
    ms_png_info  *ms_ptr = png_get_error_ptr(png_ptr);
    msSetError(MS_IOERR, "libpng error (%s)", msg);
    if (ms_ptr == NULL) {/* we are completely hosed now */
        fprintf(stderr, "png severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }
    longjmp(ms_ptr->jmpbuf, 1);
}


/* newer versions of libpng do not include zlib.h, so we
 * define the compression value here
 */
#ifndef Z_BEST_COMPRESSION
#define Z_BEST_COMPRESSION       9
#endif

int ms_png_write_image_init(gdIOCtx *ctx, ms_png_info *ms_ptr)
{
    png_structp png_ptr;       /* note:  temporary variables! */
    png_infop info_ptr;
    png_text text[1];


    /* TODO: warning function to replace last NULL */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,ms_ptr,ms_png_error_handler, NULL);

    if (!png_ptr) {
        msSetError(MS_MEMERR,"could not create png write struct","ms_png_write_image_init()");
        return MS_FAILURE;
    }
    ms_ptr->png_ptr = png_ptr;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        msSetError(MS_MEMERR,"could not create png info struct","ms_png_write_image_init()");
        return MS_FAILURE;
    }

    /*
     * first call of this function intializes jmpbuf.
     * any subsequent error will call our custom error handler (ms_png_error_handler)
     * wich will longjmp back here as if setjmp had returned a non-zero value
     */
    if (setjmp(ms_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        msSetError(MS_MISCERR,"error writing png header (via longjmp)","ms_png_write_image_init()");
        return MS_FAILURE;
    }

    /*use the gdIOCtx writing functions*/
    png_set_write_fn (png_ptr, (void*)ctx, ctxWriteFunc, ctxFlushFunc);

    /* set max compression (writing a palette image is to gain size anyways */
    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

    /* set the image parameters appropriately */
    png_set_IHDR(png_ptr, info_ptr, ms_ptr->width, ms_ptr->height,
            ms_ptr->sample_depth, PNG_COLOR_TYPE_PALETTE,
            ms_ptr->interlaced, PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

    /* GRR WARNING:  cast of rwpng_colorp to png_colorp could fail in future
     * major revisions of libpng (but png_ptr/info_ptr will fail, regardless) */
    png_set_PLTE(png_ptr, info_ptr, (png_colorp)ms_ptr->palette,ms_ptr->num_palette);

    if (ms_ptr->num_trans > 0)
        png_set_tRNS(png_ptr, info_ptr, ms_ptr->trans,ms_ptr->num_trans, NULL);

    
    text[0].key="Software";
    text[0].compression=PNG_TEXT_COMPRESSION_NONE;
    text[0].text="UMN Mapserver";
    png_set_text(png_ptr, info_ptr, text, 1);


    /* write all chunks up to (but not including) first IDAT */
    png_write_info(png_ptr, info_ptr);

    /* set up the transformations:  for now, just pack low-bit-depth pixels
     * into bytes (one, two or four pixels per byte) */

    png_set_packing(png_ptr);


    /* make sure we save our pointers for use in other writing functions */
    ms_ptr->png_ptr = png_ptr;
    ms_ptr->info_ptr = info_ptr;
    return MS_SUCCESS;
}


/* this routine is called only for interlaced images */

int ms_png_write_image_whole(ms_png_info *ms_ptr)
{
    png_structp png_ptr = (png_structp)ms_ptr->png_ptr;
    png_infop info_ptr = (png_infop)ms_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */
    if (setjmp(ms_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        ms_ptr->png_ptr = NULL;
        ms_ptr->info_ptr = NULL;
        msSetError(MS_MISCERR,"error writing png data (via longjmp)","ms_png_write_image_whole()");
        return MS_FAILURE;
    }


    /* and now we just write the whole image; libpng takes care of interlacing
     * for us */
    png_write_image(png_ptr, ms_ptr->row_pointers);

    return MS_SUCCESS;
}





/* this routine is called only for non-interlaced images */
int ms_png_write_image_row(ms_png_info *ms_ptr)
{
    png_structp png_ptr = (png_structp)ms_ptr->png_ptr;
    png_infop info_ptr = (png_infop)ms_ptr->info_ptr;

    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */

    if (setjmp(ms_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        ms_ptr->png_ptr = NULL;
        ms_ptr->info_ptr = NULL;
        msSetError(MS_MISCERR,"error writing png row (via longjmp)","ms_png_write_image_row()");
        return MS_FAILURE;
    }

    /* indexed_data points at our one row of indexed data */
    png_write_row(png_ptr, ms_ptr->indexed_data);
    return MS_SUCCESS;
}





/* this routine is called only for non-interlaced images */
int ms_png_write_image_finish(ms_png_info *ms_ptr)
{
    png_structp png_ptr = (png_structp)ms_ptr->png_ptr;
    png_infop info_ptr = (png_infop)ms_ptr->info_ptr;


    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */

    if (setjmp(ms_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        ms_ptr->png_ptr = NULL;
        ms_ptr->info_ptr = NULL;
        msSetError(MS_MISCERR,"error writing png footer (via longjmp)","ms_png_write_image_finish()");
        return MS_FAILURE;
    }


    /* close out PNG file; if we had any text or time info to write after
     * the IDATs, second argument would be info_ptr */

    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    ms_ptr->png_ptr = NULL;
    ms_ptr->info_ptr = NULL;

    return MS_SUCCESS;
}


#define PAM_GETR(p) ((p).r)
#define PAM_GETG(p) ((p).g)
#define PAM_GETB(p) ((p).b)
#define PAM_GETA(p) ((p).a)
#define PAM_ASSIGN(p,red,grn,blu,alf) \
    do { (p).r = (red); (p).g = (grn); (p).b = (blu); (p).a = (alf); } while (0)
#define PAM_EQUAL(p,q) \
    ((p).r == (q).r && (p).g == (q).g && (p).b == (q).b && (p).a == (q).a)
#define PAM_DEPTH(newp,p,oldmaxval,newmaxval) \
    PAM_ASSIGN( (newp), \
            ( (int) PAM_GETR(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETG(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETB(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETA(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval) )


/* from pamcmap.h */

typedef struct acolorhist_item *acolorhist_vector;
struct acolorhist_item {
    apixel acolor;
    int value;
};

typedef struct acolorhist_list_item *acolorhist_list;
struct acolorhist_list_item {
    struct acolorhist_item ch;
    acolorhist_list next;
};

typedef acolorhist_list *acolorhash_table;

#define MAXCOLORS  32767

#define LARGE_NORM
#define REP_AVERAGE_PIXELS

typedef struct box *box_vector;
struct box {
    int ind;
    int colors;
    int sum;
};

static acolorhist_vector mediancut
(acolorhist_vector achv, int colors, int sum, unsigned char maxval, int newcolors);
static int redcompare (const void *ch1, const void *ch2);
static int greencompare (const void *ch1, const void *ch2);
static int bluecompare (const void *ch1, const void *ch2);
static int alphacompare (const void *ch1, const void *ch2);
static int sumcompare (const void *b1, const void *b2);

static acolorhist_vector pam_acolorhashtoacolorhist
(acolorhash_table acht, int maxacolors);
static acolorhist_vector pam_computeacolorhist
(apixel **apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_computeacolorhash
(apixel** apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_allocacolorhash (void);
static int pam_addtoacolorhash
(acolorhash_table acht, apixel *acolorP, int value);
static int pam_lookupacolor (acolorhash_table acht, apixel* acolorP);
static void pam_freeacolorhist (acolorhist_vector achv);
static void pam_freeacolorhash (acolorhash_table acht);




int msSaveImageRGBAQuantized(gdImagePtr img, gdIOCtx *ctx, outputFormatObj *format)
{
    apixel **apixels=NULL,*datapixels=NULL; /* pointer to the gd truecolor pixels */
    register apixel *pP;
    register int col;
    register int ind;
    int retval=MS_SUCCESS;
    unsigned char *outrow,*pQ;
    unsigned char maxval, newmaxval;
    acolorhist_vector achv, acolormap=NULL;
    acolorhash_table acht;
    int row;
    int colors;
    int newcolors = 0;
    int usehash;
    int x;
    /*  int channels;  */
    int bot_idx, top_idx;
    int remap[256];
    int reqcolors = atoi(msGetOutputFormatOption( format, "QUANTIZE_COLORS", "256"));
    const char *interlace;
    ms_png_info info;
    info.width = gdImageSX(img);
    info.height = gdImageSY(img);
    interlace = msGetOutputFormatOption( format, "INTERLACE", "OFF" );
    if( strcasecmp("ON", interlace) == 0 || strcasecmp("YES", interlace) == 0
            || strcasecmp("1", interlace) == 0)
        info.interlaced=1;
    else
        info.interlaced=0;
    info.row_pointers=NULL;
    info.indexed_data=NULL;
    maxval = 255;

    /*switch our gd alpha to something coherent*/
    apixels=(apixel**)malloc(info.height*sizeof(apixel**));
    datapixels=(apixel*)malloc(info.height*info.width*sizeof(apixel));
    for(row=0;row<info.height;row++) {
        apixels[row]=&(datapixels[row*info.width]);
        for(col=0;col<info.width;col++) {
            int c=gdImageTrueColorPixel(img,col,row);
            apixels[row][col].r=gdTrueColorGetRed(c);
            apixels[row][col].g=gdTrueColorGetGreen(c);
            apixels[row][col].b=gdTrueColorGetBlue(c);
            switch gdTrueColorGetAlpha(c) {
            case 0:
                apixels[row][col].a=255;
                break;
            case 127:
                apixels[row][col].a=0;
                break;
            default:
                apixels[row][col].a=(127-gdTrueColorGetAlpha(c))*2;
            }
        }
    }

    /*
     ** Step 2: attempt to make a histogram of the colors, unclustered.
     ** If at first we don't succeed, lower maxval to increase color
     ** coherence and try again.  This will eventually terminate, with
     ** maxval at worst 15, since 32^3 is approximately MAXCOLORS.
                  [GRR POSSIBLE BUG:  what about 32^4 ?]
     */
    for ( ; ; ) {
        achv = pam_computeacolorhist(
                apixels, info.width, info.height, MAXCOLORS, &colors );
        if ( achv != (acolorhist_vector) 0 )
            break;
        newmaxval = maxval / 2;
        for ( row = 0; row < info.height; ++row )
            for ( col = 0, pP = apixels[row]; col < info.width; ++col, ++pP )
                PAM_DEPTH( *pP, *pP, maxval, newmaxval );
        maxval = newmaxval;
    }
    newcolors = MS_MIN(colors, reqcolors);
    acolormap = mediancut(achv, colors, info.width*info.height, maxval, newcolors);
    pam_freeacolorhist(achv);

    /*
     ** Step 3.4 [GRR]: set the bit-depth appropriately, given the actual
     ** number of colors that will be used in the output image.
     */

    if (newcolors <= 2)
        info.sample_depth = 1;
    else if (newcolors <= 4)
        info.sample_depth = 2;
    else if (newcolors <= 16)
        info.sample_depth = 4;
    else
        info.sample_depth = 8;

    /*
     ** Step 3.5 [GRR]: remap the palette colors so that all entries with
     ** the maximal alpha value (i.e., fully opaque) are at the end and can
     ** therefore be omitted from the tRNS chunk.  Note that the ordering of
     ** opaque entries is reversed from how Step 3 arranged them--not that
     ** this should matter to anyone.
     */

    for (top_idx = newcolors-1, bot_idx = x = 0;  x < newcolors;  ++x) {
        if (PAM_GETA(acolormap[x].acolor) == maxval)
            remap[x] = top_idx--;
        else
            remap[x] = bot_idx++;
    }


    /* sanity check:  top and bottom indices should have just crossed paths */
    if (bot_idx != top_idx + 1) {
        msSetError(MS_MISCERR,"quantization sanity check failed","msSaveImageRGBAQuantized()");
        retval = MS_FAILURE;
        goto failure;
    }

    info.num_palette = newcolors;
    info.num_trans = bot_idx;
    /* GRR TO DO:  if bot_idx == 0, check whether all RGB samples are gray
                   and if so, whether grayscale sample_depth would be same
                   => skip following palette section and go grayscale */


    /*
     ** Step 3.6 [GRR]: rescale the palette colors to a maxval of 255, as
     ** required by the PNG spec.  (Technically, the actual remapping happens
     ** in here, too.)
     */

    if (maxval < 255) {
        for (x = 0; x < newcolors; ++x) {
            /* the rescaling part of this is really just PAM_DEPTH() broken out
             *  for the PNG palette; the trans-remapping just puts the values
             *  in different slots in the PNG palette */
            info.palette[remap[x]].r = (acolormap[x].acolor.r * 255 + (maxval >> 1)) / maxval;
            info.palette[remap[x]].g = (acolormap[x].acolor.g * 255 + (maxval >> 1)) / maxval;
            info.palette[remap[x]].b = (acolormap[x].acolor.b * 255 + (maxval >> 1)) / maxval;
            info.trans[remap[x]]     = (acolormap[x].acolor.a * 255 + (maxval >> 1)) / maxval;
        }
    } else {
        for (x = 0; x < newcolors; ++x) {
            info.palette[remap[x]].r = acolormap[x].acolor.r;
            info.palette[remap[x]].g = acolormap[x].acolor.g;
            info.palette[remap[x]].b = acolormap[x].acolor.b;
            info.trans[remap[x]]     = acolormap[x].acolor.a;
        }
    }


    /*
     ** Step 3.7 [GRR]: allocate memory for either a single row (non-
     ** interlaced -> progressive write) or the entire indexed image
     ** (interlaced -> all at once); note that rwpng_info.row_pointers
     ** is still in use via apixels (INPUT data).
     */

    if (info.interlaced) {
        if ((info.indexed_data = (unsigned char *)malloc(info.width * info.height)) != NULL) {
            if ((info.row_pointers = (unsigned char **)malloc(info.height * sizeof(unsigned char *))) != NULL) {
                for (row = 0;  row < info.height;  ++row)
                    info.row_pointers[row] = info.indexed_data + row*info.width;
            }
        }
    } else
        info.indexed_data = (unsigned char *)malloc(info.width);

    if (info.indexed_data == NULL ||
            (info.interlaced && info.row_pointers == NULL))
    {
        msSetError(MS_MEMERR,"error allocating png structs","msSaveImageRGBAQuantized()");
        retval = MS_FAILURE;
        goto failure;
    }



    /*
     ** Step 4: map the colors in the image to their closest match in the
     ** new colormap, and write 'em out.
     */
    acht = pam_allocacolorhash( );
    usehash = 1;

    if(ms_png_write_image_init(ctx,&info)==MS_FAILURE) {
        msSetError(MS_MISCERR,"error writing png header","msSaveImageRGBAQuantized()");
        retval = MS_FAILURE;
        goto failure;
    }

    for ( row = 0; row < info.height; ++row ) {
        outrow = info.interlaced? info.row_pointers[row] : info.indexed_data;
        col = 0;
        pP = apixels[row];
        pQ = outrow;
        do {
            /* Check hash table to see if we have already matched this color. */
            ind = pam_lookupacolor( acht, pP );
            if ( ind == -1 ) {
                /* No; search acolormap for closest match. */
                register int i, r1, g1, b1, a1, r2, g2, b2, a2;
                register long dist, newdist;

                r1 = PAM_GETR( *pP );
                g1 = PAM_GETG( *pP );
                b1 = PAM_GETB( *pP );
                a1 = PAM_GETA( *pP );
                dist = 2000000000;
                for ( i = 0; i < newcolors; ++i ) {
                    r2 = PAM_GETR( acolormap[i].acolor );
                    g2 = PAM_GETG( acolormap[i].acolor );
                    b2 = PAM_GETB( acolormap[i].acolor );
                    a2 = PAM_GETA( acolormap[i].acolor );
                    /* GRR POSSIBLE BUG */
                    newdist = ( r1 - r2 ) * ( r1 - r2 ) +  /* may overflow? */
                    ( g1 - g2 ) * ( g1 - g2 ) +
                    ( b1 - b2 ) * ( b1 - b2 ) +
                    ( a1 - a2 ) * ( a1 - a2 );
                    if ( newdist < dist ) {
                        ind = i;
                        dist = newdist;
                    }
                }
                if ( usehash ) {
                    if ( pam_addtoacolorhash( acht, pP, ind ) < 0 ) {
                        usehash = 0;
                    }
                }
            }

            /*          *pP = acolormap[ind].acolor;  */
            *pQ = (unsigned char)remap[ind];

            ++col;
            ++pP;
            ++pQ;

        }
        while ( col != info.width );

        /* if non-interlaced PNG, write row now */
        if (!info.interlaced) {
            if(ms_png_write_image_row(&info)==MS_FAILURE) {
                msSetError(MS_MISCERR,"Error writing png row","msSaveImageRGBAQuantized()");
                retval = MS_FAILURE;
                goto failure;
            }
        }
    }

    /* write entire interlaced palette PNG, or finish/flush noninterlaced one */
    if (info.interlaced) {
        if(ms_png_write_image_whole(&info)==MS_FAILURE) {
            msSetError(MS_MISCERR,"Error writing interlaced png data","msSaveImageRGBAQuantized()");
            retval = MS_FAILURE;
            goto failure;
        }
    }
    pam_freeacolorhash(acht);
    ms_png_write_image_finish(&info);


    failure:
    free(info.indexed_data);
    free(info.row_pointers);
    free(acolormap);
    free(apixels);
    free(datapixels);
    return retval;
}



/*
 ** Here is the fun part, the median-cut colormap generator.  This is based
 ** on Paul Heckbert's paper, "Color Image Quantization for Frame Buffer
 ** Display," SIGGRAPH 1982 Proceedings, page 297.
 */

static acolorhist_vector
mediancut( achv, colors, sum, maxval, newcolors )
acolorhist_vector achv;
int colors, sum, newcolors;
unsigned char maxval;
{
    acolorhist_vector acolormap;
    box_vector bv;
    register int bi, i;
    int boxes;

    bv = (box_vector) malloc( sizeof(struct box) * newcolors );
    acolormap =
        (acolorhist_vector) malloc( sizeof(struct acolorhist_item) * newcolors);
    if ( bv == (box_vector) 0 || acolormap == (acolorhist_vector) 0 ) {
        fprintf( stderr, "  out of memory allocating box vector\n" );
        fflush(stderr);
        exit(6);
    }
    for ( i = 0; i < newcolors; ++i )
        PAM_ASSIGN( acolormap[i].acolor, 0, 0, 0, 0 );

    /*
     ** Set up the initial box.
     */
    bv[0].ind = 0;
    bv[0].colors = colors;
    bv[0].sum = sum;
    boxes = 1;

    /*
     ** Main loop: split boxes until we have enough.
     */
    while ( boxes < newcolors ) {
        register int indx, clrs;
        int sm;
        register int minr, maxr, ming, mina, maxg, minb, maxb, maxa, v;
        int halfsum, lowersum;

        /*
         ** Find the first splittable box.
         */
        for ( bi = 0; bi < boxes; ++bi )
            if ( bv[bi].colors >= 2 )
                break;
        if ( bi == boxes )
            break;        /* ran out of colors! */
        indx = bv[bi].ind;
        clrs = bv[bi].colors;
        sm = bv[bi].sum;

        /*
         ** Go through the box finding the minimum and maximum of each
         ** component - the boundaries of the box.
         */
        minr = maxr = PAM_GETR( achv[indx].acolor );
        ming = maxg = PAM_GETG( achv[indx].acolor );
        minb = maxb = PAM_GETB( achv[indx].acolor );
        mina = maxa = PAM_GETA( achv[indx].acolor );
        for ( i = 1; i < clrs; ++i )
        {
            v = PAM_GETR( achv[indx + i].acolor );
            if ( v < minr ) minr = v;
            if ( v > maxr ) maxr = v;
            v = PAM_GETG( achv[indx + i].acolor );
            if ( v < ming ) ming = v;
            if ( v > maxg ) maxg = v;
            v = PAM_GETB( achv[indx + i].acolor );
            if ( v < minb ) minb = v;
            if ( v > maxb ) maxb = v;
            v = PAM_GETA( achv[indx + i].acolor );
            if ( v < mina ) mina = v;
            if ( v > maxa ) maxa = v;
        }

        /*
         ** Find the largest dimension, and sort by that component.  I have
         ** included two methods for determining the "largest" dimension;
         ** first by simply comparing the range in RGB space, and second
         ** by transforming into luminosities before the comparison.  You
         ** can switch which method is used by switching the commenting on
         ** the LARGE_ defines at the beginning of this source file.
         */
#ifdef LARGE_NORM
        if ( maxa - mina >= maxr - minr && maxa - mina >= maxg - ming && maxa - mina >= maxb - minb )
            qsort(
                    (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                    alphacompare );
        else if ( maxr - minr >= maxg - ming && maxr - minr >= maxb - minb )
            qsort(
                    (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                    redcompare );
        else if ( maxg - ming >= maxb - minb )
            qsort(
                    (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                    greencompare );
        else
            qsort(
                    (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                    bluecompare );
#endif /*LARGE_NORM*/
#ifdef LARGE_LUM
        {
            apixel p;
            float rl, gl, bl, al;

            PAM_ASSIGN(p, maxr - minr, 0, 0, 0);
            rl = PPM_LUMIN(p);
            PAM_ASSIGN(p, 0, maxg - ming, 0, 0);
            gl = PPM_LUMIN(p);
            PAM_ASSIGN(p, 0, 0, maxb - minb, 0);
            bl = PPM_LUMIN(p);

            /*
GRR: treat alpha as grayscale and assign (maxa - mina) to each of R, G, B?
     assign (maxa - mina)/3 to each?
     use alpha-fractional luminosity?  (normalized_alpha * lum(r,g,b))
        al = dunno ...
     [probably should read Heckbert's paper to decide]
             */

            if ( al >= rl && al >= gl && al >= bl )
                qsort(
                        (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                        alphacompare );
            else if ( rl >= gl && rl >= bl )
                qsort(
                        (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                        redcompare );
            else if ( gl >= bl )
                qsort(
                        (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                        greencompare );
            else
                qsort(
                        (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                        bluecompare );
        }
#endif /*LARGE_LUM*/

        /*
         ** Now find the median based on the counts, so that about half the
         ** pixels (not colors, pixels) are in each subdivision.
         */
        lowersum = achv[indx].value;
        halfsum = sm / 2;
        for ( i = 1; i < clrs - 1; ++i )
        {
            if ( lowersum >= halfsum )
                break;
            lowersum += achv[indx + i].value;
        }

        /*
         ** Split the box, and sort to bring the biggest boxes to the top.
         */
        bv[bi].colors = i;
        bv[bi].sum = lowersum;
        bv[boxes].ind = indx + i;
        bv[boxes].colors = clrs - i;
        bv[boxes].sum = sm - lowersum;
        ++boxes;
        qsort( (char*) bv, boxes, sizeof(struct box), sumcompare );
    }

    /*
     ** Ok, we've got enough boxes.  Now choose a representative color for
     ** each box.  There are a number of possible ways to make this choice.
     ** One would be to choose the center of the box; this ignores any structure
     ** within the boxes.  Another method would be to average all the colors in
     ** the box - this is the method specified in Heckbert's paper.  A third
     ** method is to average all the pixels in the box.  You can switch which
     ** method is used by switching the commenting on the REP_ defines at
     ** the beginning of this source file.
     */
    for ( bi = 0; bi < boxes; ++bi ) {
#ifdef REP_CENTER_BOX
        register int indx = bv[bi].ind;
        register int clrs = bv[bi].colors;
        register int minr, maxr, ming, maxg, minb, maxb, mina, maxa, v;

        minr = maxr = PAM_GETR( achv[indx].acolor );
        ming = maxg = PAM_GETG( achv[indx].acolor );
        minb = maxb = PAM_GETB( achv[indx].acolor );
        mina = maxa = PAM_GETA( achv[indx].acolor );
        for ( i = 1; i < clrs; ++i )
        {
            v = PAM_GETR( achv[indx + i].acolor );
            minr = min( minr, v );
            maxr = max( maxr, v );
            v = PAM_GETG( achv[indx + i].acolor );
            ming = min( ming, v );
            maxg = max( maxg, v );
            v = PAM_GETB( achv[indx + i].acolor );
            minb = min( minb, v );
            maxb = max( maxb, v );
            v = PAM_GETA( achv[indx + i].acolor );
            mina = min( mina, v );
            maxa = max( maxa, v );
        }
        PAM_ASSIGN(
                acolormap[bi].acolor, ( minr + maxr ) / 2, ( ming + maxg ) / 2,
                ( minb + maxb ) / 2, ( mina + maxa ) / 2 );
#endif /*REP_CENTER_BOX*/
#ifdef REP_AVERAGE_COLORS
        register int indx = bv[bi].ind;
        register int clrs = bv[bi].colors;
        register long r = 0, g = 0, b = 0, a = 0;

        for ( i = 0; i < clrs; ++i )
        {
            r += PAM_GETR( achv[indx + i].acolor );
            g += PAM_GETG( achv[indx + i].acolor );
            b += PAM_GETB( achv[indx + i].acolor );
            a += PAM_GETA( achv[indx + i].acolor );
        }
        r = r / clrs;
        g = g / clrs;
        b = b / clrs;
        a = a / clrs;
        PAM_ASSIGN( acolormap[bi].acolor, r, g, b, a );
#endif /*REP_AVERAGE_COLORS*/
#ifdef REP_AVERAGE_PIXELS
        register int indx = bv[bi].ind;
        register int clrs = bv[bi].colors;
        register long r = 0, g = 0, b = 0, a = 0, sum = 0;

        for ( i = 0; i < clrs; ++i )
        {
            r += PAM_GETR( achv[indx + i].acolor ) * achv[indx + i].value;
            g += PAM_GETG( achv[indx + i].acolor ) * achv[indx + i].value;
            b += PAM_GETB( achv[indx + i].acolor ) * achv[indx + i].value;
            a += PAM_GETA( achv[indx + i].acolor ) * achv[indx + i].value;
            sum += achv[indx + i].value;
        }
        r = r / sum;
        if ( r > maxval ) r = maxval;        /* avoid math errors */
        g = g / sum;
        if ( g > maxval ) g = maxval;
        b = b / sum;
        if ( b > maxval ) b = maxval;
        a = a / sum;
        if ( a > maxval ) a = maxval;
        /* GRR 20001228:  added casts to quiet warnings; 255 DEPENDENCY */
        PAM_ASSIGN( acolormap[bi].acolor, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a );
#endif /*REP_AVERAGE_PIXELS*/
    }

    /*
     ** All done.
     */
    free(bv);
    return acolormap;
}

static int
redcompare( const void *ch1, const void *ch2 )
{
    return (int) PAM_GETR( ((acolorhist_vector)ch1)->acolor ) -
    (int) PAM_GETR( ((acolorhist_vector)ch2)->acolor );
}

static int
greencompare( const void *ch1, const void *ch2 )
{
    return (int) PAM_GETG( ((acolorhist_vector)ch1)->acolor ) -
    (int) PAM_GETG( ((acolorhist_vector)ch2)->acolor );
}

static int
bluecompare( const void *ch1, const void *ch2 )
{
    return (int) PAM_GETB( ((acolorhist_vector)ch1)->acolor ) -
    (int) PAM_GETB( ((acolorhist_vector)ch2)->acolor );
}

static int
alphacompare( const void *ch1, const void *ch2 )
{
    return (int) PAM_GETA( ((acolorhist_vector)ch1)->acolor ) -
    (int) PAM_GETA( ((acolorhist_vector)ch2)->acolor );
}

static int
sumcompare( const void *b1, const void *b2 )
{
    return ((box_vector)b2)->sum -
    ((box_vector)b1)->sum;
}


/*===========================================================================*/


/* libpam3.c - pam (portable alpha map) utility library part 3
 **
 ** Colormap routines.
 **
 ** Copyright (C) 1989, 1991 by Jef Poskanzer.
 ** Copyright (C) 1997 by Greg Roelofs.
 **
 ** Permission to use, copy, modify, and distribute this software and its
 ** documentation for any purpose and without fee is hereby granted, provided
 ** that the above copyright notice appear in all copies and that both that
 ** copyright notice and this permission notice appear in supporting
 ** documentation.  This software is provided "as is" without express or
 ** implied warranty.
 */

/*
#include "pam.h"
#include "pamcmap.h"
 */

#define HASH_SIZE 20023

#define pam_hashapixel(p) ( ( ( (long) PAM_GETR(p) * 33023 + \
    (long) PAM_GETG(p) * 30013 + \
    (long) PAM_GETB(p) * 27011 + \
    (long) PAM_GETA(p) * 24007 ) \
    & 0x7fffffff ) % HASH_SIZE )

static acolorhist_vector
pam_computeacolorhist( apixels, cols, rows, maxacolors, acolorsP )
apixel** apixels;
int cols, rows, maxacolors;
int* acolorsP;
{
    acolorhash_table acht;
    acolorhist_vector achv;

    acht = pam_computeacolorhash( apixels, cols, rows, maxacolors, acolorsP );
    if ( acht == (acolorhash_table) 0 )
        return (acolorhist_vector) 0;
    achv = pam_acolorhashtoacolorhist( acht, maxacolors );
    pam_freeacolorhash( acht );
    return achv;
}



static acolorhash_table
pam_computeacolorhash( apixels, cols, rows, maxacolors, acolorsP )
apixel** apixels;
int cols, rows, maxacolors;
int* acolorsP;
{
    acolorhash_table acht;
    register apixel* pP;
    acolorhist_list achl;
    int col, row, hash;

    acht = pam_allocacolorhash( );
    *acolorsP = 0;

    /* Go through the entire image, building a hash table of colors. */
    for ( row = 0; row < rows; ++row )
        for ( col = 0, pP = apixels[row]; col < cols; ++col, ++pP )
        {
            hash = pam_hashapixel( *pP );
            for ( achl = acht[hash]; achl != (acolorhist_list) 0; achl = achl->next )
                if ( PAM_EQUAL( achl->ch.acolor, *pP ) )
                    break;
            if ( achl != (acolorhist_list) 0 )
                ++(achl->ch.value);
            else
            {
                if ( ++(*acolorsP) > maxacolors )
                {
                    pam_freeacolorhash( acht );
                    return (acolorhash_table) 0;
                }
                achl = (acolorhist_list) malloc( sizeof(struct acolorhist_list_item) );
                if ( achl == 0 ) {
                    fprintf( stderr, "  out of memory computing hash table\n" );
                    exit(7);
                }
                achl->ch.acolor = *pP;
                achl->ch.value = 1;
                achl->next = acht[hash];
                acht[hash] = achl;
            }
        }

    return acht;
}



static acolorhash_table
pam_allocacolorhash( )
{
    acolorhash_table acht;
    int i;

    acht = (acolorhash_table) malloc( HASH_SIZE * sizeof(acolorhist_list) );
    if ( acht == 0 ) {
        fprintf( stderr, "  out of memory allocating hash table\n" );
        exit(8);
    }

    for ( i = 0; i < HASH_SIZE; ++i )
        acht[i] = (acolorhist_list) 0;

    return acht;
}



static int
pam_addtoacolorhash( acht, acolorP, value )
acolorhash_table acht;
apixel* acolorP;
int value;
{
    register int hash;
    register acolorhist_list achl;

    achl = (acolorhist_list) malloc( sizeof(struct acolorhist_list_item) );
    if ( achl == 0 )
        return -1;
    hash = pam_hashapixel( *acolorP );
    achl->ch.acolor = *acolorP;
    achl->ch.value = value;
    achl->next = acht[hash];
    acht[hash] = achl;
    return 0;
}



static acolorhist_vector
pam_acolorhashtoacolorhist( acht, maxacolors )
acolorhash_table acht;
int maxacolors;
{
    acolorhist_vector achv;
    acolorhist_list achl;
    int i, j;

    /* Now collate the hash table into a simple acolorhist array. */
    achv = (acolorhist_vector) malloc( maxacolors * sizeof(struct acolorhist_item) );
    /* (Leave room for expansion by caller.) */
    if ( achv == (acolorhist_vector) 0 ) {
        fprintf( stderr, "  out of memory generating histogram\n" );
        exit(9);
    }

    /* Loop through the hash table. */
    j = 0;
    for ( i = 0; i < HASH_SIZE; ++i )
        for ( achl = acht[i]; achl != (acolorhist_list) 0; achl = achl->next )
        {
            /* Add the new entry. */
            achv[j] = achl->ch;
            ++j;
        }

    /* All done. */
    return achv;
}



static int
pam_lookupacolor( acht, acolorP )
acolorhash_table acht;
apixel* acolorP;
{
    int hash;
    acolorhist_list achl;

    hash = pam_hashapixel( *acolorP );
    for ( achl = acht[hash]; achl != (acolorhist_list) 0; achl = achl->next )
        if ( PAM_EQUAL( achl->ch.acolor, *acolorP ) )
            return achl->ch.value;

    return -1;
}



static void
pam_freeacolorhist( achv )
acolorhist_vector achv;
{
    free( (char*) achv );
}



static void
pam_freeacolorhash( acht )
acolorhash_table acht;
{
    int i;
    acolorhist_list achl, achlnext;

    for ( i = 0; i < HASH_SIZE; ++i )
        for ( achl = acht[i]; achl != (acolorhist_list) 0; achl = achlnext )
        {
            achlnext = achl->next;
            free( (char*) achl );
        }
    free( (char*) acht );
}

int find_closest_color(ms_png_info *mpi, int r, int g, int b, int a) {
    int i,dr,dg,db,da;
    int idx=-1,dst,mindst=0xFFFFF;
    for(i=0;i<mpi->num_palette;i++) {
        dr=r-mpi->palette[i].r;
        dg=g-mpi->palette[i].g;
        db=b-mpi->palette[i].b;
        da=(i<mpi->num_trans)?a-mpi->trans[i]:a-255;
        dst=dr*dr+dg*dg+db*db+da*da;
        if(dst<mindst) {
            mindst=dst;
            idx=i;
        }
    }
    return idx;
}


int msSaveImageRGBAPalette(gdImagePtr img, gdIOCtx *ctx ,outputFormatObj *format) {
    const char *pfile = msGetOutputFormatOption( format, "PALETTE", "palette.txt");
    FILE *stream=NULL;
    char buffer[MS_BUFFER_LENGTH];
    int ncolors = 0;
    int r,g,b,a,c,x,y;
    ms_png_info info;
    unsigned short ****cache;
    info.width=gdImageSX(img);
    info.height=gdImageSY(img);
    info.sample_depth=8;
    info.interlaced=0;
    if(info.width < 1 || info.height < 1) return MS_FAILURE;

    stream = fopen(pfile, "r");
    if(!stream) {
        msSetError(MS_IOERR, "Error opening palette file %s.", "msSaveImageRGBAPalette()", pfile);
        return MS_FAILURE;
    }

    while(fgets(buffer, MS_BUFFER_LENGTH, stream) && ncolors<256) 
    { 
        if(sscanf(buffer,"%d,%d,%d,%d\n",&r,&g,&b,&a)==4) {
            info.palette[ncolors].r=r;
            info.palette[ncolors].g=g;
            info.palette[ncolors].b=b;
            info.trans[ncolors]=a;
            ncolors++;
        }
    }
    fclose(stream);
    info.num_palette=info.num_trans=ncolors;

    cache=(unsigned short****)calloc(256,sizeof(unsigned short***));
    if(!cache) {
        msSetError(MS_MEMERR,"error allocating color index lookup cache","msSaveImageRGBAPalette()");
        return MS_FAILURE;
    }
    info.indexed_data=(unsigned char*)malloc(info.width);
    if(!info.indexed_data) {
        msSetError(MS_MEMERR,"error allocating png row cache","msSaveImageRGBAPalette()");
        free(cache);
        return MS_FAILURE;
    }

    if(ms_png_write_image_init(ctx,&info)==MS_FAILURE) {
        msSetError(MS_MISCERR,"error in png header writing","msSaveImageRGBAPalette()");
        free(cache);
        free(info.indexed_data);
        return MS_FAILURE;
    }

    for(y=0;y<info.height;y++) {
        for(x=0;x<info.width;x++) {
            int index;
            c = gdImageTrueColorPixel(img,x,y);
            r=gdTrueColorGetRed(c);
            b=gdTrueColorGetBlue(c);
            g=gdTrueColorGetGreen(c);
            a=(127-gdTrueColorGetAlpha(c))*2;
            if(!cache[r])
                cache[r]=(unsigned short***)calloc(256,sizeof(unsigned short**));
            if(!cache[r][g])
                cache[r][g]=(unsigned short**)calloc(256,sizeof(unsigned short*));
            if(!cache[r][g][b]){
                cache[r][g][b]=(unsigned short*)calloc(256,sizeof(unsigned short));
            }
            if(!cache[r][g][b][a]) {
                index=find_closest_color(&info,r,g,b,a);
                cache[r][g][b][a]=index+1;
                info.indexed_data[x]=index;
            } else {
                info.indexed_data[x]=cache[r][g][b][a]-1;
            }
        }
        if(ms_png_write_image_row(&info)==MS_FAILURE) {
            msSetError(MS_MISCERR,"error in png row writing","msSaveImageRGBAPalette()");
            free(cache);
            free(info.indexed_data);
            return MS_FAILURE;
        }
    }
    if(ms_png_write_image_finish(&info)==MS_FAILURE) {
        msSetError(MS_MISCERR,"error in png row writing","msSaveImageRGBAPalette()");
        free(cache);
        free(info.indexed_data);
        return MS_FAILURE;
    }
    for(r=0;r<256;r++) {
        if(cache[r]) {
            for (g=0;g<256;g++){
                if(cache[r][g]) {
                    for(b=0;b<256;b++) {
                        if(cache[r][g][b]) {
                            free(cache[r][g][b]);
                        }
                    }
                    free(cache[r][g]);
                }
            }
            free(cache[r]);
        }
    }
    free(cache);
    free(info.indexed_data);
    return MS_SUCCESS;
}
#endif /*USE_RGBA_PNG*/



