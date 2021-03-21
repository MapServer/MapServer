/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  AGG template library types.
 * Author:   John Novak (jnovak@novacell.com)
 * Author:   Thomas Bonfort (tbonfort@terriscope.fr)
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
 ******************************************************************************/

#include "renderers/agg/include/agg_path_storage.h"

/*
 * interface to a shapeObj representing lines, providing the functions
 * needed by the agg rasterizer. treats shapeObjs with multiple linestrings.
 */
class line_adaptor
{
public:
  explicit line_adaptor(shapeObj *shape):s(shape) {
    m_line=s->line; /*first line*/
    m_point=m_line->point; /*current vertex is first vertex of first line*/
    m_lend=&(s->line[s->numlines]); /*pointer to after last line*/
    m_pend=&(m_line->point[m_line->numpoints]); /*pointer to after last vertex of first line*/
  }

  /* a class with virtual functions should also provide a virtual destructor */
  virtual ~line_adaptor() {}

  void rewind(unsigned) {
    m_line=s->line; /*first line*/
    m_point=m_line->point; /*current vertex is first vertex of first line*/
    m_pend=&(m_line->point[m_line->numpoints]); /*pointer to after last vertex of first line*/
  }

  virtual unsigned vertex(double* x, double* y) {
    if(m_point < m_pend) {
      /*here we treat the case where a real vertex is returned*/
      bool first = m_point == m_line->point; /*is this the first vertex of a line*/
      *x = m_point->x;
      *y = m_point->y;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    /*if here, we're at the end of a line*/
    m_line++;
    *x = *y = 0.0;
    if(m_line>=m_lend) /*is this the last line of the shapObj. normally,
        (m_line==m_lend) should be a sufficient test, as the caller should not call
        this function if a previous call returned path_cmd_stop.*/
      return mapserver::path_cmd_stop; /*no more points to process*/

    /*if here, there are more lines in the shapeObj, continue with next one*/
    m_point=m_line->point; /*pointer to first point of next line*/
    m_pend=&(m_line->point[m_line->numpoints]); /*pointer to after last point of next line*/

    return vertex(x,y); /*this will return the first point of the next line*/
  }
protected:
  shapeObj *s;
  lineObj *m_line, /*current line pointer*/
          *m_lend; /*points to after the last line*/
  pointObj *m_point, /*current point*/
           *m_pend; /*points to after last point of current line*/
};


class polygon_adaptor
{
public:
  explicit polygon_adaptor(shapeObj *shape):s(shape) {
    m_line=s->line; /*first lines*/
    m_point=m_line->point; /*first vertex of first line*/
    m_lend=&(s->line[s->numlines]); /*pointer to after last line*/
    m_pend=&(m_line->point[m_line->numpoints]); /*pointer to after last vertex of first line*/
  }

  /* a class with virtual functions should also provide a virtual destructor */
  virtual ~polygon_adaptor() {}

  void rewind(unsigned) {
    /*reset pointers*/
    m_stop=false;
    m_line=s->line;
    m_point=m_line->point;
    m_pend=&(m_line->point[m_line->numpoints]);
  }

  virtual unsigned vertex(double* x, double* y) {
    if(m_point < m_pend) {
      /*if here, we have a real vertex*/
      bool first = m_point == m_line->point;
      *x = m_point->x;
      *y = m_point->y;
      m_point++;
      return first ? mapserver::path_cmd_move_to : mapserver::path_cmd_line_to;
    }
    *x = *y = 0.0;
    if(!m_stop) {
      /*if here, we're after the last vertex of the current line
       * we return the command to close the current polygon*/
      m_line++;
      if(m_line>=m_lend) {
        /*if here, we've finished all the vertexes of the shape.
         * we still return the command to close the current polygon,
         * but set m_stop so the subsequent call to vertex() will return
         * the stop command*/
        m_stop=true;
        return mapserver::path_cmd_end_poly;
      }
      /*if here, there's another line in the shape, so we set the pointers accordingly
       * and return the command to close the current polygon*/
      m_point=m_line->point; /*first vertex of next line*/
      m_pend=&(m_line->point[m_line->numpoints]); /*pointer to after last vertex of next line*/
      return mapserver::path_cmd_end_poly;
    }
    /*if here, a previous call to vertex informed us that we'd consumed all the vertexes
     * of the shape. return the command to stop processing this shape*/
    return mapserver::path_cmd_stop;
  }
protected:
  shapeObj *s;
  double ox = 0.0;
  double oy = 0.0;
  lineObj *m_line, /*pointer to current line*/
          *m_lend; /*pointer to after last line of the shape*/
  pointObj *m_point, /*pointer to current vertex*/
           *m_pend; /*pointer to after last vertex of current line*/
  bool m_stop = false; /*should next call return stop command*/
};

mapserver::path_storage imageVectorSymbol(symbolObj *);
