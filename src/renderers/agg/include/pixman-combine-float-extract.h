/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2010, 2012 Soren Sandmann Pedersen
 * Copyright © 2010, 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Soren Sandmann Pedersen (sandmann@cs.au.dk)
 */

#include <cfloat>

#define force_inline AGG_INLINE
#define FLOAT_IS_ZERO(f)     (-FLT_MIN < (f) && (f) < FLT_MIN)

/****************************************************************************/
/* Below code is extracted from pixman-0.43.4 pixman/pixman-combine-float.c */
/****************************************************************************/

/*
 * PDF nonseperable blend modes are implemented using the following functions
 * to operate in Hsl space, with Cmax, Cmid, Cmin referring to the max, mid
 * and min value of the red, green and blue components.
 *
 * LUM (C) = 0.3 × Cred + 0.59 × Cgreen + 0.11 × Cblue
 *
 * clip_color (C):
 *     l = LUM (C)
 *     min = Cmin
 *     max = Cmax
 *     if n < 0.0
 *         C = l + (((C – l) × l) ⁄ (l – min))
 *     if x > 1.0
 *         C = l + (((C – l) × (1 – l) ) ⁄ (max – l))
 *     return C
 *
 * set_lum (C, l):
 *     d = l – LUM (C)
 *     C += d
 *     return clip_color (C)
 *
 * SAT (C) = CH_MAX (C) - CH_MIN (C)
 *
 * set_sat (C, s):
 *     if Cmax > Cmin
 *         Cmid = ( ( ( Cmid – Cmin ) × s ) ⁄ ( Cmax – Cmin ) )
 *         Cmax = s
 *     else
 *         Cmid = Cmax = 0.0
 *         Cmin = 0.0
 *     return C
 */

/* For premultiplied colors, we need to know what happens when C is
 * multiplied by a real number. LUM and SAT are linear:
 *
 *     LUM (r × C) = r × LUM (C)	SAT (r * C) = r * SAT (C)
 *
 * If we extend clip_color with an extra argument a and change
 *
 *     if x >= 1.0
 *
 * into
 *
 *     if x >= a
 *
 * then clip_color is also linear:
 *
 *     r * clip_color (C, a) = clip_color (r * C, r * a);
 *
 * for positive r.
 *
 * Similarly, we can extend set_lum with an extra argument that is just passed
 * on to clip_color:
 *
 *       r * set_lum (C, l, a)
 *
 *     = r × clip_color (C + l - LUM (C), a)
 *
 *     = clip_color (r * C + r × l - r * LUM (C), r * a)
 *
 *     = set_lum (r * C, r * l, r * a)
 *
 * Finally, set_sat:
 *
 *       r * set_sat (C, s) = set_sat (x * C, r * s)
 *
 * The above holds for all non-zero x, because the x'es in the fraction for
 * C_mid cancel out. Specifically, it holds for x = r:
 *
 *       r * set_sat (C, s) = set_sat (r * C, r * s)
 *
 */
typedef struct
{
    float	r;
    float	g;
    float	b;
} rgb_t;

static force_inline float
minf (float a, float b)
{
    return a < b? a : b;
}

static force_inline float
maxf (float a, float b)
{
    return a > b? a : b;
}

static force_inline float
channel_min (const rgb_t *c)
{
    return minf (minf (c->r, c->g), c->b);
}

static force_inline float
channel_max (const rgb_t *c)
{
    return maxf (maxf (c->r, c->g), c->b);
}

static force_inline float
get_lum (const rgb_t *c)
{
    return c->r * 0.3f + c->g * 0.59f + c->b * 0.11f;
}

static force_inline float
get_sat (const rgb_t *c)
{
    return channel_max (c) - channel_min (c);
}

static void
clip_color (rgb_t *color, float a)
{
    float l = get_lum (color);
    float n = channel_min (color);
    float x = channel_max (color);
    float t;

    if (n < 0.0f)
    {
	t = l - n;
	if (FLOAT_IS_ZERO (t))
	{
	    color->r = 0.0f;
	    color->g = 0.0f;
	    color->b = 0.0f;
	}
	else
	{
	    color->r = l + (((color->r - l) * l) / t);
	    color->g = l + (((color->g - l) * l) / t);
	    color->b = l + (((color->b - l) * l) / t);
	}
    }
    if (x > a)
    {
	t = x - l;
	if (FLOAT_IS_ZERO (t))
	{
	    color->r = a;
	    color->g = a;
	    color->b = a;
	}
	else
	{
	    color->r = l + (((color->r - l) * (a - l) / t));
	    color->g = l + (((color->g - l) * (a - l) / t));
	    color->b = l + (((color->b - l) * (a - l) / t));
	}
    }
}

static void
set_lum (rgb_t *color, float sa, float l)
{
    float d = l - get_lum (color);

    color->r = color->r + d;
    color->g = color->g + d;
    color->b = color->b + d;

    clip_color (color, sa);
}

static void
set_sat (rgb_t *src, float sat)
{
    float *max, *mid, *min;
    float t;

    if (src->r > src->g)
    {
	if (src->r > src->b)
	{
	    max = &(src->r);

	    if (src->g > src->b)
	    {
		mid = &(src->g);
		min = &(src->b);
	    }
	    else
	    {
		mid = &(src->b);
		min = &(src->g);
	    }
	}
	else
	{
	    max = &(src->b);
	    mid = &(src->r);
	    min = &(src->g);
	}
    }
    else
    {
	if (src->r > src->b)
	{
	    max = &(src->g);
	    mid = &(src->r);
	    min = &(src->b);
	}
	else
	{
	    min = &(src->r);

	    if (src->g > src->b)
	    {
		max = &(src->g);
		mid = &(src->b);
	    }
	    else
	    {
		max = &(src->b);
		mid = &(src->g);
	    }
	}
    }

    t = *max - *min;

    if (FLOAT_IS_ZERO (t))
    {
	*mid = *max = 0.0f;
    }
    else
    {
	*mid = ((*mid - *min) * sat) / t;
	*max = sat;
    }

    *min = 0.0f;
}

/* Hue:
 *
 *       as * ad * B(s/as, d/as)
 *     = as * ad * set_lum (set_sat (s/as, SAT (d/ad)), LUM (d/ad), 1)
 *     = set_lum (set_sat (ad * s, as * SAT (d)), as * LUM (d), as * ad)
 *
 */
static force_inline void
blend_hsl_hue (rgb_t *res,
	       const rgb_t *dest, float da,
	       const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_sat (res, get_sat (dest) * sa);
    set_lum (res, sa * da, get_lum (dest) * sa);
}

/* 
 * Saturation
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (set_sat (d/ad, SAT (s/as)), LUM (d/ad), 1)
 *   = set_lum (as * ad * set_sat (d/ad, SAT (s/as)),
 *                                       as * LUM (d), as * ad)
 *   = set_lum (set_sat (as * d, ad * SAT (s), as * LUM (d), as * ad))
 */
static force_inline void
blend_hsl_saturation (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_sat (res, get_sat (src) * da);
    set_lum (res, sa * da, get_lum (dest) * sa);
}

/* 
 * Color
 *
 *     as * ad * B(s/as, d/as)
 *   = as * ad * set_lum (s/as, LUM (d/ad), 1)
 *   = set_lum (s * ad, as * LUM (d), as * ad)
 */
static force_inline void
blend_hsl_color (rgb_t *res,
		 const rgb_t *dest, float da,
		 const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_lum (res, sa * da, get_lum (dest) * sa);
}

/*
 * Luminosity
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (d/ad, LUM (s/as), 1)
 *   = set_lum (as * d, ad * LUM (s), as * ad)
 */
static force_inline void
blend_hsl_luminosity (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_lum (res, sa * da, get_lum (src) * da);
}
