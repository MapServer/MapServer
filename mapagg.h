/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  AGG template library types.
 * Author:   John Novak (jnovak@novacell.com) 
 *
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

#include "agg_array.h"
#include "agg_rendering_buffer.h"


template<class T> class mapserv_row_ptr_cache
{
public:
	typedef agg::const_row_info<T> row_data;

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

        //--------------------------------------------------------------------
        AGG_INLINE       T* buf()          { return m_buf;    }
        AGG_INLINE const T* buf()    const { return m_buf;    }
        AGG_INLINE unsigned width()  const { return m_width;  }
        AGG_INLINE unsigned height() const { return m_height; }
        AGG_INLINE int      stride() const { return m_stride; }
        AGG_INLINE unsigned stride_abs() const 
        {
            return (m_stride < 0) ? unsigned(-m_stride) : unsigned(m_stride); 
        }

        //--------------------------------------------------------------------
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

        //--------------------------------------------------------------------
        T const* const* rows() const { return &m_rows[0]; }

        //--------------------------------------------------------------------
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

        //--------------------------------------------------------------------
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
        //--------------------------------------------------------------------
        T*            m_buf;        // Pointer to rendering buffer
        agg::pod_array<T*> m_rows;       // Pointers to each row of the buffer
        unsigned      m_width;      // Width in pixels
        unsigned      m_height;     // Height in pixels
        int           m_stride;     // Number of bytes per row. Can be < 0
};

