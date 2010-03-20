/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  AGG template library types.
 * Author:   John Novak (jnovak@novacell.com) 
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

#include "renderers/agg/include/agg_array.h"
#include "renderers/agg/include/agg_rendering_buffer.h"


template<class T> class mapserv_row_ptr_cache
{
public:
	typedef mapserver::const_row_info<T> row_data;

	mapserv_row_ptr_cache() :
            m_buf(0),
            m_rows(),
            m_width(0),
            m_height(0),
            m_stride(0)
	{
	}
	
	mapserv_row_ptr_cache( const gdImagePtr pImg ) :
            m_buf(0),
            m_rows(),
            m_width(0),
            m_height(0),
            m_stride(0)
	{
		attach( pImg->tpixels, pImg->sx, pImg->sy, pImg->sx * sizeof( T ) );
	}
	
	void attach( T **ppRows, unsigned width, unsigned height, int stride )
	{
		m_width	= width;
		m_height = height;
		m_stride = stride;
		
		if( height > m_rows.size() )
			m_rows.resize( height );
			
		T** rows	= &m_rows[0];
		int iRowIndex = 0;

		while( height--)
		{
			*rows = ppRows[iRowIndex];
			rows++;
			iRowIndex++;
		}
	}

        /*-------------------------------------------------------------------*/
        AGG_INLINE       T* buf()          { return m_buf;    }
        AGG_INLINE const T* buf()    const { return m_buf;    }
        AGG_INLINE unsigned width()  const { return m_width;  }
        AGG_INLINE unsigned height() const { return m_height; }
        AGG_INLINE int      stride() const { return m_stride; }
        AGG_INLINE unsigned stride_abs() const 
        {
            return (m_stride < 0) ? unsigned(-m_stride) : unsigned(m_stride); 
        }

        /*-------------------------------------------------------------------*/
        AGG_INLINE       T* row_ptr(int, int y, unsigned) 
        { 
            return m_rows[y]; 
        }
        AGG_INLINE       T* row_ptr(int y)       { return m_rows[y]; }
        AGG_INLINE const T* row_ptr(int y) const { return m_rows[y]; }
        AGG_INLINE row_data row    (int y) const 
        { 
            return row_data(0, m_width-1, m_rows[y]); 
        }

        /*-------------------------------------------------------------------*/
        T const* const* rows() const { return &m_rows[0]; }

        /*-------------------------------------------------------------------*/
        template<class RenBuf>
        void copy_from(const RenBuf& src)
        {
            unsigned h = height();
            if(src.height() < h) h = src.height();
        
            unsigned l = stride_abs();
            if(src.stride_abs() < l) l = src.stride_abs();
        
            l *= sizeof(T);

            unsigned y;
            unsigned w = width();
            for (y = 0; y < h; y++)
            {
                memcpy(row_ptr(0, y, w), src.row_ptr(y), l);
            }
        }

        /*-------------------------------------------------------------------*/
        void clear(T value)
        {
            unsigned y;
            unsigned w = width();
            unsigned stride = stride_abs();
            for(y = 0; y < height(); y++)
            {
                T* p = row_ptr(0, y, w);
                unsigned x;
                for(x = 0; x < stride; x++)
                {
                    *p++ = value;
                }
            }
        }

    private:
        /*-------------------------------------------------------------------*/
        T*            m_buf;        /* Pointer to rendering buffer */
        mapserver::pod_array<T*> m_rows;  /* Pointers to each row of the buffer */
        unsigned      m_width;      /* Width in pixels */
        unsigned      m_height;     /* Height in pixels */
        int           m_stride;     /* Number of bytes per row. Can be < 0 */
};

/*
 * interface to a shapeObj representing lines, providing the functions
 * needed by the agg rasterizer. treats shapeObjs with multiple linestrings.
 */
class line_adaptor {
public:
    line_adaptor(shapeObj *shape):s(shape)
    {
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
    
    virtual unsigned vertex(double* x, double* y)
    {
        if(m_point < m_pend)
        {
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
private:
    shapeObj *s;
    lineObj *m_line, /*current line pointer*/
    *m_lend; /*points to after the last line*/
    pointObj *m_point, /*current point*/
    *m_pend; /*points to after last point of current line*/
};

class offset_line_adaptor: public line_adaptor  {
public:
    offset_line_adaptor(shapeObj *shape, double ox, double oy):line_adaptor(shape),ox(ox),oy(oy)
    {
    }
    
    unsigned vertex(double* x, double* y)
    {
        unsigned ret = line_adaptor::vertex(x,y);
        *x+=ox;
        *y+=oy;
        return ret;
    }
private:
    double ox,oy;
};



class polygon_adaptor {
public:
    polygon_adaptor(shapeObj *shape):s(shape),m_stop(false)
    {
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
    
    virtual unsigned vertex(double* x, double* y)
    {
        if(m_point < m_pend)
        {
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
private:
    shapeObj *s;
    double ox,oy;
    lineObj *m_line, /*pointer to current line*/
    *m_lend; /*pointer to after last line of the shape*/
    pointObj *m_point, /*pointer to current vertex*/
    *m_pend; /*pointer to after last vertex of current line*/
    bool m_stop; /*should next call return stop command*/ 
};

class offset_polygon_adaptor: public polygon_adaptor  {
public:
    offset_polygon_adaptor(shapeObj *shape, double ox, double oy):polygon_adaptor(shape),ox(ox),oy(oy)
    {
    }
    
    unsigned vertex(double* x, double* y)
    {
        unsigned ret = polygon_adaptor::vertex(x,y);
        *x+=ox;
        *y+=oy;
        return ret;
    }
private:
    double ox,oy;
};





