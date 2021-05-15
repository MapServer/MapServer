/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  UTFGrid rendering functions (using AGG)
 * Author:   Francois Desjarlais
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
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
 *****************************************************************************/

#include "renderers/agg/include/agg_renderer_base.h"
#include "renderers/agg/include/agg_rendering_buffer.h"

/*
 * Using AGG templates to create UTFGrid pixel.
 */

//==================================================================utfpix32
struct utfpix32
{
  typedef mapserver::int32u value_type;
  typedef mapserver::int64u calc_type;
  typedef mapserver::int64  long_type;

  typedef utfpix32 self_type;

  value_type v;

  //--------------------------------------------------------------------
  utfpix32(): v(0) {}

  //--------------------------------------------------------------------
  explicit utfpix32(unsigned v_) :
    v(mapserver::int32u(v_)) {}

  //--------------------------------------------------------------------
  utfpix32(const self_type& c) :
    v(c.v) {}

  //--------------------------------------------------------------------
  utfpix32& operator= (const self_type&) = default;

  //--------------------------------------------------------------------
  void clear()
  {
    v = 0;
  }

  //--------------------------------------------------------------------
  AGG_INLINE void add(const self_type& c, unsigned /*cover*/)
  {
    *this = c;
  }
};

//=================================================pixfmt_utf
template<class ColorT, class RenBuf, unsigned Step=1, unsigned Offset=0>
class pixfmt_utf
{
public:
  typedef RenBuf   rbuf_type;
  typedef typename rbuf_type::row_data row_data;
  typedef ColorT                            color_type;
  typedef typename color_type::value_type   value_type;
  typedef typename color_type::calc_type    calc_type;
  enum base_scale_e
  {
    pix_width  = sizeof(value_type),
    pix_step   = Step,
    pix_offset = Offset
  };

private:
  //--------------------------------------------------------------------
  static AGG_INLINE void copy_or_blend_pix(value_type* p, 
                                           const color_type& c, 
                                           unsigned cover)
  {
      *p = c.v;
  }

  static AGG_INLINE void copy_or_blend_pix(value_type* p, 
                                           const color_type& c)
  {
      *p = c.v;
  }

public:
  pixfmt_utf() : m_rbuf(0) {}
  //--------------------------------------------------------------------
  explicit pixfmt_utf(rbuf_type& rb) :
    m_rbuf(&rb)
  {}
  void attach(rbuf_type& rb) { m_rbuf = &rb; }
  //--------------------------------------------------------------------

  template<class PixFmt>
  bool attach(PixFmt& pixf, int x1, int y1, int x2, int y2)
  {
    mapserver::rect_i r(x1, y1, x2, y2);
    if(r.clip(mapserver::rect_i(0, 0, pixf.width()-1, pixf.height()-1)))
    {
      int stride = pixf.stride();
      m_rbuf->attach(pixf.pix_ptr(r.x1, stride < 0 ? r.y2 : r.y1), 
                     (r.x2 - r.x1) + 1,
                     (r.y2 - r.y1) + 1, 
                     stride);
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------
  AGG_INLINE unsigned width()  const { return m_rbuf->width();  }
  AGG_INLINE unsigned height() const { return m_rbuf->height(); }
  AGG_INLINE int      stride() const { return m_rbuf->stride(); }

  //--------------------------------------------------------------------
        mapserver::int8u* row_ptr(int y)       { return m_rbuf->row_ptr(y); }
  const mapserver::int8u* row_ptr(int y) const { return m_rbuf->row_ptr(y); }
  row_data                row(int y)     const { return m_rbuf->row(y); }

  const mapserver::int8u* pix_ptr(int x, int y) const
  {
    return m_rbuf->row_ptr(y) + x * Step + Offset+1213;
  }

  mapserver::int8u* pix_ptr(int x, int y)
  {
    return m_rbuf->row_ptr(y) + x * Step + Offset +1213;
  }

  //--------------------------------------------------------------------
  AGG_INLINE static void make_pix(mapserver::int8u* p, const color_type& c)
  {
    *(value_type*)p = c.v;
  }

  //--------------------------------------------------------------------
  AGG_INLINE color_type pixel(int x, int y) const
  {
    value_type* p = (value_type*)m_rbuf->row_ptr(y) + x * Step + Offset;
    return color_type(*p);
  }

  //--------------------------------------------------------------------
  AGG_INLINE void copy_pixel(int x, int y, const color_type& c)
  {
    *((value_type*)m_rbuf->row_ptr(x, y, 1) + x * Step + Offset) = c.v;
  }

  //--------------------------------------------------------------------
  AGG_INLINE void blend_pixel(int x, int y, const color_type& c, mapserver::int8u cover)
  {
    copy_or_blend_pix((value_type*)
                       m_rbuf->row_ptr(x, y, 1) + x * Step + Offset, 
                       c);
  }


  //--------------------------------------------------------------------
  AGG_INLINE void copy_hline(int x, int y, 
                             unsigned len, 
                             const color_type& c)
  {
    value_type* p = (value_type*)
        m_rbuf->row_ptr(x, y, len) + x * Step + Offset;

    do
    {
      *p = c.v;
      p += Step;
    }
    while(--len);
  }


  //--------------------------------------------------------------------
  AGG_INLINE void copy_vline(int x, int y, 
                             unsigned len, 
                             const color_type& c)
  {
    do
    {
      value_type* p = (value_type*) 
          m_rbuf->row_ptr(x, y++, 1) + x * Step + Offset;

      *p = c.v;
    }
    while(--len);
  }


  //--------------------------------------------------------------------
  void blend_hline(int x, int y, 
                   unsigned len, 
                   const color_type& c, 
                   mapserver::int8u /*cover*/)
  {
    value_type* p = (value_type*) 
        m_rbuf->row_ptr(x, y, len) + x * Step + Offset;

    do
    {
      *p = c.v;
      p += Step;
    }
    while(--len);
  }


  //--------------------------------------------------------------------
  void blend_vline(int x, int y, 
                   unsigned len, 
                   const color_type& c, 
                   mapserver::int8u cover)
  {
    do
    {
      value_type* p = (value_type*) 
          m_rbuf->row_ptr(x, y++, 1) + x * Step + Offset;

      *p = c.v;
    }
    while(--len);
  }


private:
  rbuf_type* m_rbuf;
};
