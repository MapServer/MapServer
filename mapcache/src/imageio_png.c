/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "geocache.h"
#include <png.h>
#include <apr_strings.h>


/**\addtogroup imageio_png */
/** @{ */
typedef struct _geocache_buffer_closure _geocache_buffer_closure;
struct _geocache_buffer_closure{
   geocache_buffer *buffer;
   unsigned char *ptr;
};

void _geocache_imageio_png_write_func(png_structp png_ptr, png_bytep data, png_size_t length) {
   geocache_buffer_append((geocache_buffer*)png_get_io_ptr(png_ptr),length,data);
}

void _geocache_imageio_png_read_func(png_structp png_ptr, png_bytep data, png_size_t length) {
   _geocache_buffer_closure *b = (_geocache_buffer_closure*)png_get_io_ptr(png_ptr);
   memcpy(data,b->ptr,length);
   b->ptr += length;
}

void _geocache_imageio_png_flush_func(png_structp png_ptr) {
   // do nothing
}

geocache_image* _geocache_imageio_png_decode(geocache_context *ctx, geocache_buffer *buffer) {
   geocache_image *img;
   int bit_depth,color_type,i;
   unsigned char **row_pointers;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   _geocache_buffer_closure b;
   b.buffer = buffer;
   b.ptr = buffer->buf;
   png_uint_32 width, height;


   /* could pass pointers to user-defined error handlers instead of NULLs: */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (!png_ptr) {
      ctx->set_error(ctx, 500, "failed to allocate png_struct structure");
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      ctx->set_error(ctx, 500, "failed to allocate png_info structure");
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      ctx->set_error(ctx, 500, "failed to setjmp(png_jmpbuf(png_ptr))");
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      return NULL;
   }
   png_set_read_fn(png_ptr,&b,_geocache_imageio_png_read_func);

   png_read_info(png_ptr,info_ptr);
   img = geocache_image_create(ctx);
   if(!png_get_IHDR(png_ptr, info_ptr, &width, &height,&bit_depth, &color_type,NULL,NULL,NULL)) {
      ctx->set_error(ctx, 500, "failed to read png header");
      return NULL;
   }

   img->w = width;
   img->h = height;
   img->data = apr_pcalloc(ctx->pool,img->w*img->h*4*sizeof(unsigned char));
   img->stride = img->w * 4;
   row_pointers = apr_pcalloc(ctx->pool,img->h * sizeof(unsigned char*));

   unsigned char *rowptr = img->data;
   for(i=0;i<img->h;i++) {
      row_pointers[i] = rowptr;
      rowptr += img->stride;
   }


   png_set_expand(png_ptr);
   png_set_strip_16(png_ptr);
   png_set_gray_to_rgb(png_ptr);
   png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
   
   png_read_update_info(png_ptr, info_ptr);
#ifdef DEBUG
   if(img->stride != png_get_rowbytes(png_ptr, info_ptr)) {
      ctx->set_error(ctx, 500, "### BUG ### failed to decompress PNG to RGBA");
      return NULL;
   }
#endif

   png_read_image(png_ptr, row_pointers);
   
   png_read_end(png_ptr,NULL);
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

   return img;
}

/**
 * \brief encode an image to RGB(A) PNG format
 * \private \memberof geocache_image_format_png
 * \sa geocache_image_format::write()
 */
geocache_buffer* _geocache_imageio_png_encode(geocache_context *ctx, geocache_image *img, geocache_image_format *format) {
   png_infop info_ptr;
   int color_type;
   size_t row;
   geocache_buffer *buffer = NULL;
   int compression = ((geocache_image_format_png*)format)->compression_level;
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
   if (!png_ptr) {
      ctx->set_error(ctx, 500, "failed to allocate png_struct structure");
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_write_struct(&png_ptr,
            (png_infopp)NULL);
      ctx->set_error(ctx, 500, "failed to allocate png_info structure");
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      ctx->set_error(ctx, 500, "failed to setjmp(png_jmpbuf(png_ptr))");
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return NULL;
   }

   buffer = geocache_buffer_create(5000,ctx->pool);

   png_set_write_fn(png_ptr, buffer, _geocache_imageio_png_write_func, _geocache_imageio_png_flush_func);

   if(geocache_imageio_image_has_alpha(img))
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
   else
      color_type = PNG_COLOR_TYPE_RGB;
      
   png_set_IHDR(png_ptr, info_ptr, img->w, img->h,
         8, color_type, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
   if(compression == GEOCACHE_COMPRESSION_BEST)
      png_set_compression_level (png_ptr, Z_BEST_COMPRESSION);
   else if(compression == GEOCACHE_COMPRESSION_FAST)
      png_set_compression_level (png_ptr, Z_BEST_SPEED);

   png_write_info(png_ptr, info_ptr);
   if(color_type == PNG_COLOR_TYPE_RGB)
       png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);

   png_bytep rowptr = img->data;
   for(row=0;row<img->h;row++) {
      png_write_row(png_ptr,rowptr);
      rowptr += img->stride;
   }
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);
   return buffer;
}

/** \cond DONOTDOCUMENT */

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

typedef struct {
   unsigned char r,g,b,a;
} rgbaPixel;

typedef struct {
   unsigned char r,g,b;
} rgbPixel;

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
   rgbaPixel acolor;
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
(rgbaPixel **apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_computeacolorhash
(rgbaPixel** apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_allocacolorhash (void);
static int pam_addtoacolorhash
(acolorhash_table acht, rgbaPixel *acolorP, int value);
static int pam_lookupacolor (acolorhash_table acht, rgbaPixel* acolorP);
static void pam_freeacolorhist (acolorhist_vector achv);
static void pam_freeacolorhash (acolorhash_table acht);


/**
 * Compute a palette for the given RGBA rasterBuffer using a median cut quantization.
 * - rb: the rasterBuffer to quantize
 * - reqcolors: the desired number of colors the palette should contain. will be set
 *   with the actual number of entries in the computed palette
 * - forced_palette: entries that should appear in the computed palette
 * - num_forced_palette_entries: number of entries contained in "force_palette". if 0,
 *   "force_palette" can be NULL
 */
int _geocache_imageio_quantize_image(geocache_image *rb,
      unsigned int *reqcolors, rgbaPixel *palette,
      rgbaPixel *forced_palette, int num_forced_palette_entries) {

   rgbaPixel **apixels=NULL; /* pointer to the start rows of truecolor pixels */
   register rgbaPixel *pP;
   register int col;

   unsigned char maxval, newmaxval;
   acolorhist_vector achv, acolormap=NULL;

   int row;
   int colors;
   int newcolors = 0;

   int x;
   /*  int channels;  */


   maxval = 255;

   apixels=(rgbaPixel**)malloc(rb->h*sizeof(rgbaPixel**));
   if(!apixels) return GEOCACHE_FAILURE;

   for(row=0;row<rb->h;row++) {
      apixels[row]=(rgbaPixel*)(&(rb->data[row * rb->stride]));
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
            apixels, rb->w, rb->h, MAXCOLORS, &colors );
      if ( achv != (acolorhist_vector) 0 )
         break;
      newmaxval = maxval / 2;
      for ( row = 0; row < rb->h; ++row )
         for ( col = 0, pP = apixels[row]; col < rb->w; ++col, ++pP )
            PAM_DEPTH( *pP, *pP, maxval, newmaxval );
      maxval = newmaxval;
   }
   newcolors = GEOCACHE_MIN(colors, *reqcolors);
   acolormap = mediancut(achv, colors, rb->w*rb->h, maxval, newcolors);
   pam_freeacolorhist(achv);


   *reqcolors = newcolors;


   /*
    ** rescale the palette colors to a maxval of 255
    */

   if (maxval < 255) {
      for (x = 0; x < newcolors; ++x) {
         /* the rescaling part of this is really just PAM_DEPTH() broken out
          *  for the PNG palette; the trans-remapping just puts the values
          *  in different slots in the PNG palette */
         palette[x].r = (acolormap[x].acolor.r * 255 + (maxval >> 1)) / maxval;
         palette[x].g = (acolormap[x].acolor.g * 255 + (maxval >> 1)) / maxval;
         palette[x].b = (acolormap[x].acolor.b * 255 + (maxval >> 1)) / maxval;
         palette[x].a = (acolormap[x].acolor.a * 255 + (maxval >> 1)) / maxval;
      }
   } else {
      for (x = 0; x < newcolors; ++x) {
         palette[x].r = acolormap[x].acolor.r;
         palette[x].g = acolormap[x].acolor.g;
         palette[x].b = acolormap[x].acolor.b;
         palette[x].a = acolormap[x].acolor.a;
      }
   }

   free(acolormap);
   free(apixels);
   return GEOCACHE_SUCCESS;
}


int _geocache_imageio_classify(geocache_image *rb, unsigned char *pixels, rgbaPixel *palette, int numPaletteEntries) {
   register int ind;
   unsigned char *outrow,*pQ;
   register rgbaPixel *pP;
   acolorhash_table acht;
   int usehash, row, col;
   /*
    ** Step 4: map the colors in the image to their closest match in the
    ** new colormap, and write 'em out.
    */
   acht = pam_allocacolorhash( );
   usehash = 1;

   for ( row = 0; row < rb->h; ++row ) {
      outrow = &(pixels[row*rb->w]);
      col = 0;
      pP = (rgbaPixel*)(&(rb->data[row * rb->stride]));;
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
            for ( i = 0; i < numPaletteEntries; ++i ) {
               r2 = PAM_GETR( palette[i] );
               g2 = PAM_GETG( palette[i] );
               b2 = PAM_GETB( palette[i] );
               a2 = PAM_GETA( palette[i] );
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
         *pQ = (unsigned char)ind;

         ++col;
         ++pP;
         ++pQ;

      }
      while ( col != rb->w );
   }
   pam_freeacolorhash(acht);

   return GEOCACHE_SUCCESS;
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
rgbaPixel** apixels;
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
rgbaPixel** apixels;
int cols, rows, maxacolors;
int* acolorsP;
{
   acolorhash_table acht;
   register rgbaPixel* pP;
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
rgbaPixel* acolorP;
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
rgbaPixel* acolorP;
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

/** \endcond DONOTDOCUMENT */

int _geocache_imageio_remap_palette(unsigned char *pixels, int npixels, rgbaPixel *palette, int numPaletteEntries, rgbPixel *rgb, unsigned char *a, int *num_a) {
   int bot_idx, top_idx, x;
   int remap[256];
   /*
    ** remap the palette colors so that all entries with
    ** the maximal alpha value (i.e., fully opaque) are at the end and can
    ** therefore be omitted from the tRNS chunk.  Note that the ordering of
    ** opaque entries is reversed from how Step 3 arranged them--not that
    ** this should matter to anyone.
    */

   for (top_idx = numPaletteEntries-1, bot_idx = x = 0;  x < numPaletteEntries;  ++x) {
      if (palette[x].a == 255)
         remap[x] = top_idx--;
      else
         remap[x] = bot_idx++;
   }
   /* sanity check:  top and bottom indices should have just crossed paths */
   if (bot_idx != top_idx + 1) {
      return GEOCACHE_FAILURE;
   }

   *num_a = bot_idx;

   for(x=0;x<npixels;x++)
      pixels[x] = remap[pixels[x]];

   for (x = 0; x < numPaletteEntries; ++x) {
      a[remap[x]] = palette[x].a;
      rgb[remap[x]].r = palette[x].r;
      rgb[remap[x]].g = palette[x].g;
      rgb[remap[x]].b = palette[x].b;
   }

   return GEOCACHE_SUCCESS;
}

/**
 * \brief encode an image to quantized PNG format
 * \private \memberof geocache_image_format_png_q
 * \sa geocache_image_format::write()
 */
geocache_buffer* _geocache_imageio_png_q_encode( geocache_context *ctx, geocache_image *image, geocache_image_format *format) {
   int ret;
   geocache_buffer *buffer = geocache_buffer_create(3000,ctx->pool);
   geocache_image_format_png_q *f = (geocache_image_format_png_q*)format;
   int compression = f->format.compression_level;
   unsigned int numPaletteEntries = f->ncolors;
   unsigned char *pixels = (unsigned char*)apr_pcalloc(ctx->pool,image->w*image->h*sizeof(unsigned char));
   rgbaPixel palette[256];
   ret = _geocache_imageio_quantize_image(image,&numPaletteEntries,palette, NULL, 0);
   ret = _geocache_imageio_classify(image,pixels,palette,numPaletteEntries);
   png_infop info_ptr;
   rgbPixel rgb[256];
   unsigned char a[256];
   int num_a;
   int row,sample_depth;
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);

   if (!png_ptr)
      return (NULL);

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_write_struct(&png_ptr,(png_infopp)NULL);
      return (NULL);
   }

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (NULL);
   }

   png_set_write_fn(png_ptr,buffer, _geocache_imageio_png_write_func, _geocache_imageio_png_flush_func);


   if (numPaletteEntries <= 2)
      sample_depth = 1;
   else if (numPaletteEntries <= 4)
      sample_depth = 2;
   else if (numPaletteEntries <= 16)
      sample_depth = 4;
   else
      sample_depth = 8;

   png_set_IHDR(png_ptr, info_ptr, image->w , image->h,
         sample_depth, PNG_COLOR_TYPE_PALETTE,
         0, PNG_COMPRESSION_TYPE_DEFAULT,
         PNG_FILTER_TYPE_DEFAULT);

   if(compression == GEOCACHE_COMPRESSION_BEST)
      png_set_compression_level (png_ptr, Z_BEST_COMPRESSION);
   else if(compression == GEOCACHE_COMPRESSION_FAST)
      png_set_compression_level (png_ptr, Z_BEST_SPEED);
   
   _geocache_imageio_remap_palette(pixels, image->w * image->h, palette, numPaletteEntries,rgb,a,&num_a);
   
   png_set_PLTE(png_ptr, info_ptr, (png_colorp)(rgb),numPaletteEntries);
   if(num_a)
      png_set_tRNS(png_ptr, info_ptr, a,num_a, NULL);

   png_write_info(png_ptr, info_ptr);
   png_set_packing(png_ptr);

   for(row=0;row<image->h;row++) {
      unsigned char *rowptr = &(pixels[row*image->w]);
      png_write_row(png_ptr, rowptr);
   }
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);

   return buffer;
}


geocache_image_format* geocache_imageio_create_png_format(apr_pool_t *pool, char *name, geocache_compression_type compression) {
   geocache_image_format_png *format = apr_pcalloc(pool, sizeof(geocache_image_format_png));
   format->format.name = name;
   format->format.extension = apr_pstrdup(pool,"png");
   format->format.mime_type = apr_pstrdup(pool,"image/png");
   format->compression_level = compression;
   format->format.metadata = apr_table_make(pool,3);
   format->format.write = _geocache_imageio_png_encode;
   return (geocache_image_format*)format;
}

geocache_image_format* geocache_imageio_create_png_q_format(apr_pool_t *pool, char *name, geocache_compression_type compression, int ncolors) {
   geocache_image_format_png_q *format = apr_pcalloc(pool, sizeof(geocache_image_format_png_q));
   format->format.format.name = name;
   format->format.format.extension = apr_pstrdup(pool,"png");
   format->format.format.mime_type = apr_pstrdup(pool,"image/png");
   format->format.compression_level = compression;
   format->format.format.write = _geocache_imageio_png_q_encode;
   format->format.format.metadata = apr_table_make(pool,3);
   format->ncolors = ncolors;
   return (geocache_image_format*)format;
}

/** @} */

/* vim: ai ts=3 sts=3 et sw=3
*/
