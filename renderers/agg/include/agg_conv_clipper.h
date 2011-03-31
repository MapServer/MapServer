//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
// mcseemagg@yahoo.com
// http://www.antigrain.com
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// agg_conv_clipper.h
// Author    :  Angus Johnson                                                   
// Version   :  1.0a                                                            
// Date      :  19 June 2010                                                    
//----------------------------------------------------------------------------

#ifndef AGG_CONV_CLIPPER_INCLUDED
#define AGG_CONV_CLIPPER_INCLUDED

#include <cmath>
#include "agg_basics.h"
#include "agg_array.h"
#include "clipper.hpp"

namespace mapserver
{
  enum clipper_op_e { clipper_or, clipper_and, clipper_xor, clipper_a_minus_b, clipper_b_minus_a };

  template<class VSA, class VSB> class conv_clipper 
  {
    enum status { status_move_to, status_line_to, status_stop };
    typedef VSA source_a_type;
    typedef VSB source_b_type;
    typedef conv_clipper<source_a_type, source_b_type> self_type;
	//typedef   vertex_array_type;
    
  private:
    source_a_type*							m_src_a;
    source_b_type*							m_src_b;
    status									m_status;
    int										m_vertex;
    int										m_contour;
    clipper_op_e							m_operation;
    pod_bvector<clipper::TDoublePoint, 8>	m_vertex_accumulator;
    clipper::TPolyPolygon					m_poly_a;
    clipper::TPolyPolygon					m_poly_b;
    clipper::TPolyPolygon					m_result;
    clipper::Clipper						m_clipper;
    
  public:
    conv_clipper(source_a_type &a, source_b_type &b, clipper_op_e op = clipper_or) :
        m_src_a(&a),
        m_src_b(&b),
        m_status(status_move_to),
        m_vertex(-1),
        m_contour(-1),
        m_operation(op)
    {
      m_clipper.ForceAlternateOrientation(true);
    }

    ~conv_clipper()
    {
    }

    void attach1(VSA &source) { m_src_a = &source; }
    void attach2(VSB &source) { m_src_b = &source; }

    void operation(clipper_op_e v) { m_operation = v; }

    void rewind(unsigned path_id);
    unsigned vertex(double* x, double* y);
  
    bool next_contour();
    bool next_vertex(double* x, double* y);
    void start_extracting();
    void add_vertex_(double &x, double &y);
    void end_contour(clipper::TPolyPolygon &p);

	template<class VS> void add(VS &src, clipper::TPolyPolygon &p){
		unsigned cmd;
		double x; double y; double start_x; double start_y;
		bool starting_first_line;

		start_x = 0.0;
		start_y = 0.0;
		starting_first_line = true;
		p.resize(0);

		cmd = src->vertex( &x , &y );
		while(!is_stop(cmd))
		{
		  if(is_vertex(cmd))
		  {
			if(is_move_to(cmd))
			{
			  if(!starting_first_line ) end_contour(p);
			  start_x = x;
			  start_y = y;
			}
			add_vertex_( x, y );
			starting_first_line = false;
		  }
		  else if(is_end_poly(cmd))
		  {
			if(!starting_first_line && is_closed(cmd))
			  add_vertex_( start_x, start_y );
		  }
		  cmd = src->vertex( &x, &y );
		}
		end_contour(p);
	}
  };

  //------------------------------------------------------------------------

  template<class VSA, class VSB> 
  void conv_clipper<VSA, VSB>::start_extracting()
  {
    m_status = status_move_to;
    m_contour = -1;
    m_vertex = -1;
  }
  //------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  void conv_clipper<VSA, VSB>::rewind(unsigned path_id)
  {
    m_src_a->rewind( path_id );
    m_src_b->rewind( path_id );

    add( m_src_a , m_poly_a );
    add( m_src_b , m_poly_b );
    m_result.resize(0);

    m_clipper.Clear();
    switch( m_operation ) {
      case clipper_or: 
        {
        m_clipper.AddPolyPolygon( m_poly_a , clipper::ptSubject );
        m_clipper.AddPolyPolygon( m_poly_b , clipper::ptClip );
        m_clipper.Execute( clipper::ctUnion , m_result );
		break;
        }
      case clipper_and: 
        {
        m_clipper.AddPolyPolygon( m_poly_a , clipper::ptSubject );
        m_clipper.AddPolyPolygon( m_poly_b , clipper::ptClip );
        m_clipper.Execute( clipper::ctIntersection , m_result );
		break;
        }
      case clipper_xor: 
        {
        m_clipper.AddPolyPolygon( m_poly_a , clipper::ptSubject );
        m_clipper.AddPolyPolygon( m_poly_b , clipper::ptClip );
        m_clipper.Execute( clipper::ctXor , m_result );
		break;
        }
      case clipper_a_minus_b: 
        {
        m_clipper.AddPolyPolygon( m_poly_a , clipper::ptSubject );
        m_clipper.AddPolyPolygon( m_poly_b , clipper::ptClip );
        m_clipper.Execute( clipper::ctDifference , m_result );
		break;
        }
      case clipper_b_minus_a: 
        {
        m_clipper.AddPolyPolygon( m_poly_b , clipper::ptSubject );
        m_clipper.AddPolyPolygon( m_poly_a , clipper::ptClip );
        m_clipper.Execute( clipper::ctDifference , m_result );
		break;
        }
    }
    start_extracting();
  }
  //------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  void conv_clipper<VSA, VSB>::end_contour( clipper::TPolyPolygon &p)
  {
  unsigned i, len;

  if( m_vertex_accumulator.size() < 3 ) return;
  len = p.size();
  p.resize(len+1);
  p[len].resize(m_vertex_accumulator.size());
  for( i = 0 ; i < m_vertex_accumulator.size() ; i++ )
    p[len][i] = m_vertex_accumulator[i];
  m_vertex_accumulator.remove_all();
  }
  //------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  void conv_clipper<VSA, VSB>::add_vertex_(double &x, double &y)
  {
	 clipper::TDoublePoint v;

	  v.X = x;
	  v.Y = y;
	  m_vertex_accumulator.add( v );
  }
  //------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  bool conv_clipper<VSA, VSB>::next_contour()
  {   
	m_contour++;
	if(m_contour >= (int)m_result.size()) return false;
	m_vertex =-1;
	return true;
}
//------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  bool conv_clipper<VSA, VSB>::next_vertex(double *x, double *y)
  {
    m_vertex++;
    if(m_vertex >= (int)m_result[m_contour].size()) return false;
    *x = m_result[ m_contour ][ m_vertex ].X;
    *y = m_result[ m_contour ][ m_vertex ].Y;
    return true;
  }
  //------------------------------------------------------------------------------

  template<class VSA, class VSB> 
  unsigned conv_clipper<VSA, VSB>::vertex(double *x, double *y)
{ 
  if(  m_status == status_move_to )
  {
    if( next_contour() )
    {
      if(  next_vertex( x, y ) )
      {
        m_status =status_line_to;
        return path_cmd_move_to;
      } 
	  else 
	  {
        m_status = status_stop;
        return path_cmd_end_poly || path_flags_close;
      }
    } 
	else
      return path_cmd_stop;
  } 
  else
  {
    if(  next_vertex( x, y ) )
    {
      return path_cmd_line_to;
    } 
	else
    {
      m_status = status_move_to;
      return path_cmd_end_poly || path_flags_close;
    }
  }
}
//------------------------------------------------------------------------------


} //namespace agg
#endif //AGG_CONV_CLIPPER_INCLUDED
