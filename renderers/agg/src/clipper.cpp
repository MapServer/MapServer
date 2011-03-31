
/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  1.4i                                                            *
* Date      :  18 June 2010                                                    *
* Copyright :  Angus Johnson                                                   *
*                                                                              *
* The code in this library is an extension of Bala Vatti's clipping algorithm: *
* "A generic solution to polygon clipping"                                     *
* Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.             *
* http://portal.acm.org/citation.cfm?id=129906                                 *
*                                                                              *
* See also:                                                                    *
* Computer graphics and geometric modeling: implementation and algorithms      *
* By Max K. Agoston                                                            *
* Springer; 1 edition (January 4, 2005)                                        *
* http://books.google.com/books?q=vatti+clipping+agoston                       *
*                                                                              *
*******************************************************************************/

/****** BEGIN LICENSE BLOCK ****************************************************
*                                                                              *
* Version: MPL 1.1 or LGPL 2.1 with linking exception                          *
*                                                                              *
* The contents of this file are subject to the Mozilla Public License Version  *
* 1.1 (the "License"); you may not use this file except in compliance with     *
* the License. You may obtain a copy of the License at                         *
* http://www.mozilla.org/MPL/                                                  *
*                                                                              *
* Software distributed under the License is distributed on an "AS IS" basis,   *
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License     *
* for the specific language governing rights and limitations under the         *
* License.                                                                     *
*                                                                              *
* Alternatively, the contents of this file may be used under the terms of the  *
* Free Pascal modified version of the GNU Lesser General Public License        *
* Version 2.1 (the "FPC modified LGPL License"), in which case the provisions  *
* of this license are applicable instead of those above.                       *
* Please see the file LICENSE.txt for additional information concerning this   *
* license.                                                                     *
*                                                                              *
* The Original Code is clipper.hpp & clipper.cpp                               *
*                                                                              *
* The Initial Developer of the Original Code is                                *
* Angus Johnson (http://www.angusj.com)                                        *
*                                                                              *
******* END LICENSE BLOCK *****************************************************/

/*******************************************************************************
*                                                                              *
*  This is a translation of my Delphi clipper code and is the very first stuff *
*  I've written in C++ (or C). My apologies if the coding style is unorthodox. *
*  Please see the accompanying Delphi Clipper library (clipper.pas) for a more *
*  detailed explanation of the code logic.                                     *
*                                                                              *
*******************************************************************************/

#include "clipper.hpp"
#include <cmath>
#include <vector>
#include <cstring>
#include <algorithm>

namespace clipper {

static double const infinite = -3.4E+38;
static double const tolerance = 1.0E-10;

//same_point_tolerance ...
//Can be customized to individual needs to indicate the point at which adjacent
//vertices will be merged. Necessary because edges must have a slope.
//Must be significantly greater than 'tolerance' ...
static double const same_point_tolerance = 1.0E-4;

static const unsigned ipLeft = 1;
static const unsigned ipRight = 2;
typedef enum { dRightToLeft, dLeftToRight } TDirection;
typedef enum { itIgnore, itMax, itMin, itEdge1, itEdge2 } TIntersectType;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

TDoublePoint DoublePoint(double const &X, double const &Y)
{
	TDoublePoint p;

	p.X = X;
	p.Y = Y;
	return p;
}
//------------------------------------------------------------------------------

bool PointsEqual( TDoublePoint const &pt1, TDoublePoint const &pt2)
{
	return ( std::fabs( pt1.X - pt2.X ) < same_point_tolerance ) &&
	( std::fabs( pt1.Y - pt2.Y ) < same_point_tolerance );
}
//------------------------------------------------------------------------------

bool PointsEqual(TEdge const &edge)
{
	return ( std::fabs( edge.xbot - edge.xtop ) < same_point_tolerance ) &&
				( std::fabs( edge.ybot - edge.ytop ) < same_point_tolerance );
}
//------------------------------------------------------------------------------

void DisposePolyPts(TPolyPt *&pp)
{
	if (pp == 0) return;
	TPolyPt *tmpPp;
	pp->prev->next = 0;
	while( pp )
	{
	tmpPp = pp;
	pp = pp->next;
	delete tmpPp ;
	}
}
//------------------------------------------------------------------------------

void Clipper::DisposeAllPolyPts(){
	for (unsigned i = 0; i < m_PolyPts.size(); i++)
		DisposePolyPts(m_PolyPts[i]);
	m_PolyPts.clear();
}
//------------------------------------------------------------------------------

void ReversePolyPtLinks(TPolyPt &pp)
{
	TPolyPt *pp1, *pp2;

	pp1 = &pp;
	do {
	pp2 = pp1->next;
	pp1->next = pp1->prev;
	pp1->prev = pp2;
	pp1 = pp2;
	} while( pp1 != &pp );
}
//------------------------------------------------------------------------------

double Slope(TDoublePoint const &pt1, TDoublePoint const &pt2)
{
	if(  std::fabs( pt1.Y - pt2.Y ) < tolerance ) return infinite;
	return ( pt1.X - pt2.X )/( pt1.Y - pt2.Y );
}
//------------------------------------------------------------------------------

bool IsHorizontal(TEdge const &e)
{
	return &e  && ( e.dx < -3.39e+38 );
}
//------------------------------------------------------------------------------

void SwapSides(TEdge &edge1, TEdge &edge2)
{
	TEdgeSide side;

	side =  edge1.side;
	edge1.side = edge2.side;
	edge2.side = side;
}
//------------------------------------------------------------------------------

void SwapPolyIndexes(TEdge &edge1, TEdge &edge2)
{
  int polyIdx;

  polyIdx =  edge1.polyIdx;
	edge1.polyIdx = edge2.polyIdx;
	edge2.polyIdx = polyIdx;
}
//------------------------------------------------------------------------------

double TopX(TEdge *edge,  double const &currentY)
{
	if(  currentY == edge->ytop ) return edge->xtop;
	return edge->savedBot.X + edge->dx *( currentY - edge->savedBot.Y );
}
//------------------------------------------------------------------------------

bool EdgesShareSamePoly(TEdge &e1, TEdge &e2)
{
	return &e1  && &e2  && ( e1.polyIdx == e2.polyIdx );
}
//------------------------------------------------------------------------------

bool SlopesEqual(TEdge &e1, TEdge &e2)
{
	if(IsHorizontal(e1)) return IsHorizontal(e2);
	return ! IsHorizontal(e2) && (std::fabs(e1.dx - e2.dx)*1000 <= std::fabs(e1.dx));
}
//------------------------------------------------------------------------------

bool IntersectPoint(TEdge &edge1, TEdge &edge2, TDoublePoint &ip)
{
	double m1; double m2; double b1; double b2;

	m1 = 0; m2 = 0; b1 = 0; b2 = 0;
	if( edge1.dx != 0 ){
		m1 = 1.0/edge1.dx;
		b1 = edge1.savedBot.Y - edge1.savedBot.X/edge1.dx;
	}
	if( edge2.dx != 0 )
	{
		m2 = 1.0/edge2.dx;
		b2 = edge2.savedBot.Y - edge2.savedBot.X/edge2.dx;
	}
	if(  std::fabs( m1 - m2 ) < tolerance ) return false;
	if(  edge1.dx == 0 )
	{
		ip.X = edge1.savedBot.X;
		ip.Y = m2*ip.X + b2;
	}
	else if(  edge2.dx == 0 )
	{
		ip.X = edge2.savedBot.X;
		ip.Y = m1*ip.X + b1;
	} else
	{
		ip.X = ( b2 - b1 )*1.0/( m1 - m2 );
		ip.Y = ( m1 * ip.X + b1 );
	}
	return ( ip.Y > edge1.ytop + tolerance ) && ( ip.Y > edge2.ytop + tolerance );
}
//------------------------------------------------------------------------------

TDoublePoint GetUnitNormal( TDoublePoint const &pt1, TDoublePoint const &pt2)
{
  double dx; double dy; double f;

  dx = ( pt2.X - pt1.X );
  dy = ( pt2.Y - pt1.Y );
  if(  ( dx == 0 ) && ( dy == 0 ) ) return DoublePoint( 0, 0 );

	f = 1 *1.0/ hypot( dx , dy );
	dx = dx * f;
  dy = dy * f;
  return DoublePoint(dy, -dx);
}
//------------------------------------------------------------------------------

bool ValidateOrientation(TPolyPt *pt)
{
  TPolyPt *ptStart, *bottomPt, *ptPrev, *ptNext;
	TDoublePoint N1; TDoublePoint N2;
  bool IsClockwise;

  //compares the orientation (clockwise vs counter-clockwise) of a *simple*
  //polygon with its hole status (ie test whether an inner or outer polygon).
	//nb: complex polygons have indeterminate orientations.
	bottomPt = pt;
	ptStart = pt;
	pt = pt->next;
	while(  ( pt != ptStart ) )
	{
	if(  ( pt->pt.Y > bottomPt->pt.Y ) ||
		( ( pt->pt.Y == bottomPt->pt.Y ) && ( pt->pt.X > bottomPt->pt.X ) ) )
		bottomPt = pt;
	pt = pt->next;
	}

	ptPrev = bottomPt->prev;
	ptNext = bottomPt->next;
	N1 = GetUnitNormal( ptPrev->pt , bottomPt->pt );
	N2 = GetUnitNormal( bottomPt->pt , ptNext->pt );
	//(N1.X * N2.Y - N2.X * N1.Y) == unit normal "cross product" == sin(angle)
	IsClockwise = ( N1.X * N2.Y - N2.X * N1.Y ) > 0; //ie angle > 180deg.
	return (IsClockwise != bottomPt->isHole);
}
//------------------------------------------------------------------------------

void InitEdge(TEdge *e, TDoublePoint const &pt1,
	TDoublePoint const &pt2, TPolyType polyType)
{
	//nb: round the vertices to 6 decimal places (roundToVal == -6)
	//to further minimize floating point errors ...
	std::memset( e, 0, sizeof( TEdge ));
	if(  ( pt1.Y > pt2.Y ) )
	{
		e->xbot = pt1.X;
		e->ybot = pt1.Y;
		e->xtop = pt2.X;
		e->ytop = pt2.Y;
	} else
	{
		e->xbot = pt2.X;
		e->ybot = pt2.Y;
		e->xtop = pt1.X;
		e->ytop = pt1.Y;
	}
	e->savedBot.X = pt1.X; //temporary (just until duplicates removed)
	e->savedBot.Y = pt1.Y; //temporary (just until duplicates removed)
	e->polyType = polyType;
	e->polyIdx = -1;
	e->dx = Slope( pt1, pt2 );
}
//------------------------------------------------------------------------------

void ReInitEdge(TEdge *e, TDoublePoint
	const &pt1, TDoublePoint const &pt2)
{
	//nb: preserves linkages
	if(  ( pt1.Y > pt2.Y ) )
	{
		e->xbot = pt1.X;
		e->ybot = pt1.Y;
		e->xtop = pt2.X;
		e->ytop = pt2.Y;
	} else
	{
		e->xbot = pt2.X;
		e->ybot = pt2.Y;
		e->xtop = pt1.X;
		e->ytop = pt1.Y;
	}
	e->savedBot = pt1; //temporary (until duplicates etc removed)
	e->dx = Slope( pt1, pt2 );
}
//------------------------------------------------------------------------------

bool FixupIfDupOrColinear( TEdge *&e, TEdge *edges)
{
	if(  ( e->next == e->prev ) ) return false;

	if(  PointsEqual( *e ) ||
		PointsEqual( e->savedBot , e->prev->savedBot ) ||
		SlopesEqual( *e->prev , *e ) )
	{
		//fixup the previous edge because 'e' is about to come out of the loop ...
		ReInitEdge( e->prev , e->prev->savedBot , e->next->savedBot );
		//now remove 'e' from the DLL loop by changing the links ...
		e->next->prev = e->prev;
		e->prev->next = e->next;
		if(  ( e == edges ) )
		{
			std::memcpy(e, e->next, sizeof(TEdge));
			e->prev->next = e;
			e->next->prev = e;
		} else
			e = e->prev;
		return true;
	} else
	{
		e = e->next;
		return false;
	}
}
//------------------------------------------------------------------------------

void SwapX(TEdge &e)
{
	e.xbot = e.xtop;
	e.xtop = e.savedBot.X;
	e.savedBot.X = e.xbot;
}
//------------------------------------------------------------------------------

TEdge *BuildBound(TEdge *e,  TEdgeSide s,  bool buildForward)
{
	TEdge *eNext; TEdge *eNextNext;

	do {
		e->side = s;
		if(  buildForward ) eNext = e->next;
		else eNext = e->prev;
		if(  IsHorizontal( *eNext ) )
		{
			if(  buildForward ) eNextNext = eNext->next;
			else eNextNext = eNext->prev;
			if(  ( eNextNext->ytop < eNext->ytop ) )
			{
				//eNext is an intermediate horizontal.
				//All horizontals have their xbot aligned with the adjoining lower edge
				if(  eNext->xbot != e->xtop ) SwapX( *eNext );
			} else if(  buildForward )
			{
				//to avoid duplicating top bounds, stop if this is a
				//horizontal edge at the top of a going forward bound ...
				e->nextInLML = 0;
				return eNext;
			}
		}
		else if(  e->ytop == eNext->ytop )
		{
			e->nextInLML = 0;
			return eNext;
		}
		e->nextInLML = eNext;
		e = eNext;
	} while( true );
}

//------------------------------------------------------------------------------
// ClipperBase methods ...
//------------------------------------------------------------------------------

ClipperBase::ClipperBase() //constructor
{
	m_localMinimaList = 0;
	m_recycledLocMin = 0;
	m_recycledLocMinEnd = 0;
	m_edges.reserve(32);
}
//------------------------------------------------------------------------------

ClipperBase::~ClipperBase() //destructor
{
	Clear();
}
//------------------------------------------------------------------------------

void ClipperBase::InsertLocalMinima(TLocalMinima *newLm)
{
	TLocalMinima *tmpLm;

	if( ! m_localMinimaList )
	{
		m_localMinimaList = newLm;
	}
	else if( newLm->Y >= m_localMinimaList->Y )
	{
		newLm->nextLm = m_localMinimaList;
		m_localMinimaList = newLm;
	} else
	{
		tmpLm = m_localMinimaList;
		while( tmpLm->nextLm  && ( newLm->Y < tmpLm->nextLm->Y ) )
		tmpLm = tmpLm->nextLm;
		newLm->nextLm = tmpLm->nextLm;
		tmpLm->nextLm = newLm;
	}
}
//------------------------------------------------------------------------------

TEdge *ClipperBase::AddLML(TEdge *e)
{
	TLocalMinima *newLm;
	TEdge *e2;

	newLm = new TLocalMinima;
	newLm->nextLm = 0;
	newLm->Y = e->ybot;
	e2 = e;
	do {
		e2 = e2->next;
	} while( e2->ytop == e->ybot );
	if(  IsHorizontal( *e ) ) e = e->prev;

	if(  ( ( e->next != e2 ) && ( e->xbot < e2->xbot ) )
		|| ( ( e->next == e2 ) && ( e->dx > e2->dx ) ) )
	{
		newLm->leftBound = e;
		newLm->rightBound = e->next;
		BuildBound( newLm->leftBound , esLeft , false );
		if(  IsHorizontal( *newLm->rightBound ) &&
			( newLm->rightBound->xbot != newLm->leftBound->xbot ) )
				SwapX( *newLm->rightBound );
		InsertLocalMinima( newLm);
		return BuildBound( newLm->rightBound , esRight , true );
	}
	else
	{
		newLm->leftBound = e2;
		newLm->rightBound = e2->prev;
		if(  IsHorizontal( *newLm->rightBound ) &&
			( newLm->rightBound->xbot != newLm->leftBound->xbot ) )
				SwapX( *newLm->rightBound );
		BuildBound( newLm->rightBound , esRight , false );
		InsertLocalMinima( newLm);
		return BuildBound( newLm->leftBound , esLeft , true );
	}
}
//------------------------------------------------------------------------------

TEdge *NextMin(TEdge *e)
{
	while(  e->next->ytop >= e->ybot ) e = e->next;
	while(  e->ytop == e->ybot ) e = e->prev;
	return e;
}
//------------------------------------------------------------------------------

double RoundToTolerance(double const number){
	return std::floor( number/same_point_tolerance + 0.5 )*same_point_tolerance;
}
//------------------------------------------------------------------------------

void ClipperBase::AddPolygon( TPolygon &pg, TPolyType polyType)
{
	int i; int highI;
	TEdge *e, *e2;

	highI = pg.size() -1;
	for (i = 0; i <= highI; i++) {
		pg[i].X = RoundToTolerance(pg[i].X);
		pg[i].Y = RoundToTolerance(pg[i].Y);
	}

	while(  ( highI > 1 ) && PointsEqual(pg[0] , pg[highI] ) ) --highI;
	if(  highI < 2 ) return;

	//make sure this is a sensible polygon (ie with at least one minima) ...
	i = 1;
	while(  ( i <= highI ) && ( pg[i].Y == pg[0].Y ) ) ++i;
	if(  i > highI ) return;

	//create a new edge array ...
	TEdge *edges = new TEdge [highI +1];
	m_edges.push_back(edges);
	//fill the edge array ...
	InitEdge( &edges[highI], pg[highI], pg[0], polyType);

	edges[highI].prev = &edges[highI-1];
	edges[highI].next = &edges[0];
	InitEdge( &edges[0] , pg[0] , pg[1], polyType );
	edges[0].prev = &edges[highI];
	edges[0].next = &edges[1];
	for( i = 1 ; i < highI ; ++i )
	{
		InitEdge( &edges[i] , pg[i] , pg[i+1], polyType );
		edges[i].prev = &edges[i-1];
		edges[i].next = &edges[i+1];
	}

	//fixup any co-linear edges or duplicate points ...
	e = &edges[0];
	do {
		while( FixupIfDupOrColinear(e, &edges[0]) ) ;
	} while((e != &edges[0]) && ( e->next != e->prev ));
	if( e->next != e->prev)
		do {
			e = &edges[0];
		} while(FixupIfDupOrColinear( e, &edges[0] ));

	//make sure we still have a valid polygon ...
	if( e->next == e->prev )
	{
		m_edges.pop_back();
		delete [] edges;
		return; //oops!!
	}

	//once duplicates etc have been removed, we can set edge savedBot to their
	//proper values and also get the starting minima (e2) ...
	e = &edges[0];
	e2 = e;
	do {
		e->savedBot.X = e->xbot;
		e->savedBot.Y = e->ybot;
		if(  e->ybot > e2->ybot ) e2 = e;
		e = e->next;
	} while( e != &edges[0] );

	//to avoid endless loops, make sure e2 will line up with subsequ. NextMin.
	if( (e2->prev->ybot == e2->ybot) && ( (e2->prev->xbot == e2->xbot) ||
		(IsHorizontal(*e2) && (e2->prev->xbot == e2->xtop)) ) )
	{
		e2 = e2->prev;
		if( IsHorizontal(*e2) ) e2 = e2->prev;
	}

	//finally insert each local minima ...
	e = e2;
	do {
		e2 = AddLML( e2 );
		e2 = NextMin( e2 );
	} while( e2 != e );
}
//------------------------------------------------------------------------------

void ClipperBase::AddPolyPolygon( TPolyPolygon &ppg, TPolyType polyType)
{
	for (unsigned i = 0; i < ppg.size(); i++)
	AddPolygon(ppg[i], polyType);
}
//------------------------------------------------------------------------------

void ClipperBase::Clear()
{
	DisposeLocalMinimaList();
	for (unsigned i = 0; i < m_edges.size(); i++) delete [] m_edges[i];
	m_edges.clear();
}
//------------------------------------------------------------------------------

bool ClipperBase::Reset()
{
	TEdge *e;
	TLocalMinima *lm;

	while( m_localMinimaList ) PopLocalMinima();
	if( m_recycledLocMin ) m_localMinimaList = m_recycledLocMin;
	m_recycledLocMin = 0;
	m_recycledLocMinEnd = 0;
	if( !m_localMinimaList ) return false; //ie nothing to process

	//reset all edges ...
	lm = m_localMinimaList;
	while( lm )
	{
		e = lm->leftBound;
		while( e )
		{
			e->xbot = e->savedBot.X;
			e->ybot = e->savedBot.Y;
			e->side = esLeft;
			e->polyIdx = -1;
			e = e->nextInLML;
		}
		e = lm->rightBound;
		while( e )
		{
			e->xbot = e->savedBot.X;
			e->ybot = e->savedBot.Y;
			e->side = esRight;
			e->polyIdx = -1;
			e = e->nextInLML;
		}
		lm = lm->nextLm;
	}
	return true;
}
//------------------------------------------------------------------------------

void ClipperBase::PopLocalMinima()
{
	TLocalMinima *tmpLm;

	if( ! m_localMinimaList ) return;
	else if( ! m_recycledLocMin )
	{
		m_recycledLocMinEnd = m_localMinimaList;
		m_recycledLocMin = m_localMinimaList;
		m_localMinimaList = m_localMinimaList->nextLm;
		m_recycledLocMin->nextLm = 0;
	} else
	{
		tmpLm = m_localMinimaList->nextLm;
		m_localMinimaList->nextLm = 0;
		m_recycledLocMinEnd->nextLm = m_localMinimaList;
		m_recycledLocMinEnd = m_localMinimaList;
		m_localMinimaList = tmpLm;
	}
}
//------------------------------------------------------------------------------

void ClipperBase::DisposeLocalMinimaList()
{
	TLocalMinima *tmpLm;

	while( m_localMinimaList )
	{
		tmpLm = m_localMinimaList->nextLm;
		delete m_localMinimaList;
		m_localMinimaList = tmpLm;
	}
	while( m_recycledLocMin )
	{
		tmpLm = m_recycledLocMin->nextLm;
		delete m_recycledLocMin;
		m_recycledLocMin = tmpLm;
	}
	m_recycledLocMinEnd = 0;
}

//------------------------------------------------------------------------------
// Clipper methods ...
//------------------------------------------------------------------------------

Clipper::Clipper() : ClipperBase() //constructor
{
	m_Scanbeam = 0;
	m_ActiveEdges = 0;
	m_SortedEdges = 0;
	m_IntersectNodes = 0;
	m_ExecuteLocked = false;
	m_ForceAlternateOrientation = true;
	m_PolyPts.reserve(32);
};
//------------------------------------------------------------------------------

Clipper::~Clipper() //destructor
{
  DisposeScanbeamList();
	DisposeAllPolyPts();
};
//------------------------------------------------------------------------------

bool Clipper::InitializeScanbeam()
{
	TLocalMinima *lm;

	DisposeScanbeamList();
	if(  !Reset() ) return false;
	//add all the local minima into a fresh fScanbeam list ...
	lm = m_localMinimaList;
	while( lm )
	{
	InsertScanbeam( lm->Y );
	InsertScanbeam(lm->leftBound->ytop); //this is necessary too!
	lm = lm->nextLm;
	}
	return true;
}
//------------------------------------------------------------------------------

void Clipper::InsertScanbeam( double const &Y)
{
	TScanbeam *newSb; TScanbeam *sb2;

	if( !m_Scanbeam )
	{
	m_Scanbeam = new TScanbeam;
	m_Scanbeam->nextSb = 0;
	m_Scanbeam->Y = Y;
	}
	else if(  Y > m_Scanbeam->Y )
	{
	newSb = new TScanbeam;
	newSb->Y = Y;
	newSb->nextSb = m_Scanbeam;
	m_Scanbeam = newSb;
	} else
	{
	sb2 = m_Scanbeam;
	while( sb2->nextSb  && ( Y <= sb2->nextSb->Y ) ) sb2 = sb2->nextSb;
	if(  Y == sb2->Y ) return; //ie ignores duplicates
	newSb = new TScanbeam;
	newSb->Y = Y;
	newSb->nextSb = sb2->nextSb;
	sb2->nextSb = newSb;
  }
}
//------------------------------------------------------------------------------

double Clipper::PopScanbeam()
{
  double Y;
  TScanbeam *sb2;
  Y = m_Scanbeam->Y;
	sb2 = m_Scanbeam;
  m_Scanbeam = m_Scanbeam->nextSb;
	delete sb2;
  return Y;
}
//------------------------------------------------------------------------------

void Clipper::DisposeScanbeamList()
{
  TScanbeam *sb2;
  while ( m_Scanbeam ) {
	sb2 = m_Scanbeam->nextSb;
	delete m_Scanbeam;
	m_Scanbeam = sb2;
  }
}
//------------------------------------------------------------------------------

bool Edge2InsertsBeforeEdge1(TEdge &e1, TEdge &e2)
{
	if( e2.xbot - tolerance > e1.xbot ) return false;
	if( e2.xbot + tolerance < e1.xbot ) return true;
	if( IsHorizontal(e2) || SlopesEqual(e1,e2) ) return false;
  return (e2.dx > e1.dx);
}
//------------------------------------------------------------------------------

void Clipper::InsertEdgeIntoAEL(TEdge *edge)
{
	TEdge *e;

	edge->prevInAEL = 0;
	edge->nextInAEL = 0;
	if(  !m_ActiveEdges )
	{
		m_ActiveEdges = edge;
	}
	else if( Edge2InsertsBeforeEdge1(*m_ActiveEdges, *edge) )
	{
		edge->nextInAEL = m_ActiveEdges;
		m_ActiveEdges->prevInAEL = edge;
		m_ActiveEdges = edge;
	} else
	{
		e = m_ActiveEdges;
		while( e->nextInAEL  && !Edge2InsertsBeforeEdge1(*e->nextInAEL , *edge) )
			e = e->nextInAEL;
		edge->nextInAEL = e->nextInAEL;
		if( e->nextInAEL ) e->nextInAEL->prevInAEL = edge;
		edge->prevInAEL = e;
		e->nextInAEL = edge;
	}
}
//----------------------------------------------------------------------

void Clipper::AddHorzEdgeToSEL(TEdge *edge)
{
	//SEL pointers in PEdge are reused to build a list of horizontal edges.
	//Also, we don't need to worry about order with horizontal edge processing ...
	if( !m_SortedEdges )
	{
		m_SortedEdges = edge;
		edge->prevInSEL = 0;
		edge->nextInSEL = 0;
	}
	else
	{
		edge->nextInSEL = m_SortedEdges;
		edge->prevInSEL = 0;
		m_SortedEdges->prevInSEL = edge;
		m_SortedEdges = edge;
	}
}
//------------------------------------------------------------------------------

void AdjustLocalMinSides(TEdge *edge1, TEdge *edge2)
{
	if( !IsHorizontal( *edge2 ) && ( edge1->dx > edge2->dx ) )
	{
		edge1->side = esLeft;
		edge2->side = esRight;
	} else
	{
		edge1->side = esRight;
		edge2->side = esLeft;
	}
}
//----------------------------------------------------------------------

void Clipper::DoEdge1(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt)
{
	AddPolyPt(edge1->polyIdx , pt , edge1->side == esLeft);
	SwapSides(*edge1, *edge2);
	SwapPolyIndexes(*edge1, *edge2);
}
//----------------------------------------------------------------------

void Clipper::DoEdge2(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt)
{
	AddPolyPt(edge2->polyIdx, pt, edge2->side == esLeft);
	SwapSides(*edge1, *edge2);
	SwapPolyIndexes(*edge1, *edge2);
}
//----------------------------------------------------------------------

void Clipper::DoBothEdges(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt)
{
	AddPolyPt( edge1->polyIdx , pt , edge1->side == esLeft );
	AddPolyPt( edge2->polyIdx , pt , edge2->side == esLeft );
	SwapSides( *edge1 , *edge2 );
	SwapPolyIndexes( *edge1 , *edge2 );
}
//----------------------------------------------------------------------

void Clipper::ProcessIntersections( double const& topY)
{
	TIntersectNode *iNode;

	if( !m_ActiveEdges ) return;
	try {
		BuildIntersectList( topY );
		ProcessIntersectList();
	}
	catch(...) {
		while( m_IntersectNodes ){
			iNode = m_IntersectNodes->next;
			delete m_IntersectNodes;
			m_IntersectNodes = iNode;
		}
		m_SortedEdges = 0;
		throw "ProcessIntersections error";
	}
}
//------------------------------------------------------------------------------

bool E1PrecedesE2inAEL(TEdge *e1, TEdge *e2)
{
	while( e1 ){
		if(  e1 == e2 ) return true;
		else e1 = e1->nextInAEL;
	}
	return false;
}
//------------------------------------------------------------------------------

bool Process1Before2(TIntersectNode *Node1, TIntersectNode *Node2)
{
	if( std::fabs(Node1->pt.Y - Node2->pt.Y) < tolerance ){
		if( SlopesEqual(*Node1->edge1, *Node2->edge1) ){
			if( SlopesEqual(*Node1->edge2, *Node2->edge2) ){
				if(Node1->edge2 == Node2->edge2)
					return E1PrecedesE2inAEL(Node2->edge1, Node1->edge1); else
					return E1PrecedesE2inAEL(Node1->edge2, Node2->edge2);
			} else return ( Node1->edge2->dx < Node2->edge2->dx );
		} else return ( Node1->edge1->dx < Node2->edge1->dx );
	} else return ( Node1->pt.Y > Node2->pt.Y );
}
//------------------------------------------------------------------------------

void Clipper::AddIntersectNode(TEdge *e1, TEdge *e2, TDoublePoint const& pt)
{
	TIntersectNode *IntersectNode, *iNode;

	IntersectNode = new TIntersectNode;
	IntersectNode->edge1 = e1;
	IntersectNode->edge2 = e2;
	IntersectNode->pt = pt;
	IntersectNode->next = 0;
	IntersectNode->prev = 0;
	if( !m_IntersectNodes )
		m_IntersectNodes = IntersectNode;
	else if(  Process1Before2(IntersectNode , m_IntersectNodes) )
	{
		IntersectNode->next = m_IntersectNodes;
		m_IntersectNodes->prev = IntersectNode;
		m_IntersectNodes = IntersectNode;
	}
	else
	{
		iNode = m_IntersectNodes;
		while( iNode->next  && Process1Before2(iNode->next, IntersectNode) )
			iNode = iNode->next;
		if( iNode->next ) iNode->next->prev = IntersectNode;
		IntersectNode->next = iNode->next;
		IntersectNode->prev = iNode;
		iNode->next = IntersectNode;
	}
}
//------------------------------------------------------------------------------

void Clipper::BuildIntersectList( double const& topY)
{
	TEdge *e, *eNext;
	TDoublePoint pt;

	//prepare for sorting ...
	e = m_ActiveEdges;
	e->tmpX = TopX( e, topY );
	m_SortedEdges = e;
	m_SortedEdges->prevInSEL = 0;
	e = e->nextInAEL;
	while( e )
	{
		e->prevInSEL = e->prevInAEL;
		e->prevInSEL->nextInSEL = e;
		e->nextInSEL = 0;
		e->tmpX = TopX( e, topY );
		e = e->nextInAEL;
	}

		//bubblesort ...
	while( m_SortedEdges )
	{
		e = m_SortedEdges;
		while( e->nextInSEL )
		{
			eNext = e->nextInSEL;
			if((e->tmpX > eNext->tmpX + tolerance) && IntersectPoint(*e, *eNext, pt))
			{
				AddIntersectNode( e, eNext, pt );
				SwapWithNextInSEL(e);
			}
			else
				e = eNext;
		}
		if( e->prevInSEL ) e->prevInSEL->nextInSEL = 0;
		else break;
	}

	m_SortedEdges = 0;
}
//------------------------------------------------------------------------------

void Clipper::ProcessIntersectList()
{
	TIntersectNode *iNode;

	while( m_IntersectNodes )
	{
		iNode = m_IntersectNodes->next;
		{
			IntersectEdges( m_IntersectNodes->edge1 ,
				m_IntersectNodes->edge2 , m_IntersectNodes->pt, 0 );
			SwapPositionsInAEL( m_IntersectNodes->edge1 , m_IntersectNodes->edge2 );
		}
		delete m_IntersectNodes;
		m_IntersectNodes = iNode;
	}
}
//------------------------------------------------------------------------------

void Clipper::IntersectEdges(TEdge *e1, TEdge *e2,
		 TDoublePoint const &pt, TIntersectProtects protects)
{
	bool e1stops, e2stops, e1Contributing, e2contributing;

	e1stops = !(ipLeft & protects) &&  !e1->nextInLML &&
		( std::fabs( e1->xtop - pt.X ) < tolerance ) &&
		( std::fabs( e1->ytop - pt.Y ) < tolerance );
	e2stops = !(ipRight & protects) &&  !e2->nextInLML &&
		( std::fabs( e2->xtop - pt.X ) < tolerance ) &&
		( std::fabs( e2->ytop - pt.Y ) < tolerance );
	e1Contributing = ( e1->polyIdx >= 0 );
	e2contributing = ( e2->polyIdx >= 0 );

	switch( m_ClipType ) {
		case ctXor:
			//edges can be momentarily non-contributing at complex intersections ...
			if(  e1Contributing && e2contributing )
			{
				if(  e1stops || e2stops ) AddLocalMaxPoly( e1 , e2 , pt );
				else DoBothEdges( e1, e2, pt );
			}
			else if(  e1Contributing )
				AddPolyPt( e1->polyIdx , pt , e1->side == esLeft );
			else if(  e2contributing )
				AddPolyPt( e2->polyIdx , pt , e2->side == esLeft );
			break;
		default:
			if(  e1Contributing && e2contributing )
			{
				if(  e1stops || e2stops || ( e1->polyType != e2->polyType ) )
						AddLocalMaxPoly( e1 , e2 , pt );
				else
						DoBothEdges( e1, e2, pt );
			}
			else if(  e1Contributing )
				DoEdge1( e1, e2, pt );
			else if(  e2contributing )
				DoEdge2( e1, e2, pt );
			else
			{
				//neither edge contributing
				if(  ( e1->polyType != e2->polyType ) &&  !e1stops &&  !e2stops )
				{
					AddLocalMinPoly( e1, e2, pt );
					AdjustLocalMinSides( e1, e2 );
				}
					else SwapSides( *e1, *e2 );
			}
			break;
	} //end switch

	if(  (e1stops != e2stops) &&
		( (e1stops && (e1->polyIdx >= 0)) || (e2stops && (e2->polyIdx >= 0)) ) )
	{
		SwapSides( *e1, *e2 );
		SwapPolyIndexes( *e1, *e2 );
	}

	//finally, delete any non-contrib. maxima edges  ...
	if( e1stops ) DeleteFromAEL( e1 );
	if( e2stops ) DeleteFromAEL( e2 );
}
//------------------------------------------------------------------------------

void Clipper::DeleteFromAEL(TEdge *e)
{
	TEdge *AelPrev, *AelNext;

	AelPrev = e->prevInAEL;
	AelNext = e->nextInAEL;
	if(  !AelPrev &&  !AelNext && (e != m_ActiveEdges) ) return; //already deleted
	if( AelPrev ) AelPrev->nextInAEL = AelNext;
	else m_ActiveEdges = AelNext;
	if( AelNext ) AelNext->prevInAEL = AelPrev;
	e->nextInAEL = 0;
	e->prevInAEL = 0;
}
//------------------------------------------------------------------------------

void Clipper::DeleteFromSEL(TEdge *e)
{
	TEdge *SelPrev, *SelNext;

	SelPrev = e->prevInSEL;
	SelNext = e->nextInSEL;
	if(  !SelPrev &&  !SelNext && (e != m_SortedEdges) ) return; //already deleted
	if( SelPrev ) SelPrev->nextInSEL = SelNext;
	else m_SortedEdges = SelNext;
	if( SelNext ) SelNext->prevInSEL = SelPrev;
	e->nextInSEL = 0;
	e->prevInSEL = 0;
}
//------------------------------------------------------------------------------

void Clipper::UpdateEdgeIntoAEL(TEdge *&e)
{
	TEdge *AelPrev, *AelNext;

	if( !e->nextInLML ) throw "UpdateEdgeIntoAEL: invalid call";
	AelPrev = e->prevInAEL;
	AelNext = e->nextInAEL;
	e->nextInLML->polyIdx = e->polyIdx;
	if( AelPrev ) AelPrev->nextInAEL = e->nextInLML;
	else m_ActiveEdges = e->nextInLML;
	if( AelNext ) AelNext->prevInAEL = e->nextInLML;
	e->nextInLML->side = e->side;
	e = e->nextInLML;
	e->prevInAEL = AelPrev;
	e->nextInAEL = AelNext;
	if( !IsHorizontal(*e) ) InsertScanbeam( e->ytop );
}
//------------------------------------------------------------------------------

void Clipper::SwapWithNextInSEL(TEdge *edge)
{
	TEdge *prev, *next, *nextNext;

	prev = edge->prevInSEL;
	next = edge->nextInSEL;
	nextNext = next->nextInSEL;
	if( prev ) prev->nextInSEL = next;
	if( nextNext ) nextNext->prevInSEL = edge;
	edge->nextInSEL = nextNext;
	edge->prevInSEL = next;
	next->nextInSEL = edge;
	next->prevInSEL = prev;
	if( edge == m_SortedEdges ) m_SortedEdges = next;
}
//------------------------------------------------------------------------------

bool Clipper::IsContributing(TEdge *edge, bool &reverseSides)
{
	bool isContrib;
	TPolyType polyType = edge->polyType;
	reverseSides = false;
	 switch( m_ClipType ) {
		case ctIntersection:
			{
				isContrib = false;
				while( edge->prevInAEL )
				{
					if( edge->prevInAEL->polyType == polyType )
						reverseSides = !reverseSides;
					else
						isContrib = !isContrib;
					edge = edge->prevInAEL;
				}
			break;
			}
		case ctUnion:
			{
				isContrib = true;
				while( edge->prevInAEL )
				{
					if(  edge->prevInAEL->polyType != polyType )
						isContrib = !isContrib;
					edge = edge->prevInAEL;
				}
			break;
			}
		case ctDifference:
			{
				isContrib = (polyType == ptSubject);
				while( edge->prevInAEL )
				{
					if(  edge->prevInAEL->polyType != polyType )
						isContrib = !isContrib;
					edge = edge->prevInAEL;
				}
			break;
			}
		case ctXor:
			return true;
	}
	return isContrib;
}
//------------------------------------------------------------------------------

void Clipper::InsertLocalMinimaIntoAEL( double const &botY)
{
	bool reverseSides;
	TEdge *e;
	TDoublePoint pt;
	TLocalMinima *lm;

	while(  m_localMinimaList  && ( m_localMinimaList->Y == botY ) )
	{
		InsertEdgeIntoAEL( m_localMinimaList->leftBound );
		InsertScanbeam( m_localMinimaList->leftBound->ytop );
		InsertEdgeIntoAEL( m_localMinimaList->rightBound );

		e = m_localMinimaList->leftBound->nextInAEL;
		if(  IsHorizontal( *m_localMinimaList->rightBound ) )
		{
			//nb: only rightbounds can have a horizontal bottom edge
			AddHorzEdgeToSEL( m_localMinimaList->rightBound );
			InsertScanbeam( m_localMinimaList->rightBound->nextInLML->ytop );
		}
		else
			InsertScanbeam( m_localMinimaList->rightBound->ytop );

		lm = m_localMinimaList;
		if( IsContributing(lm->leftBound, reverseSides) )
		{
			AddLocalMinPoly( lm->leftBound,
				lm->rightBound, DoublePoint( lm->leftBound->xbot , lm->Y ) );
			if( reverseSides && (lm->leftBound->nextInAEL == lm->rightBound) )
			{
				lm->leftBound->side = esRight;
				lm->rightBound->side = esLeft;
			}
		}

		if( lm->leftBound->nextInAEL != lm->rightBound )
		{
			e = lm->leftBound->nextInAEL;
			pt = DoublePoint( lm->leftBound->xbot, lm->leftBound->ybot );
			while( e != lm->rightBound )
			{
				if( !e ) throw "AddLocalMinima: missing rightbound!";
				IntersectEdges( lm->rightBound , e , pt , 0);
				e = e->nextInAEL;
				if( !e ) throw "AddLocalMinima: intersect with maxima!??";
			}
		}
		PopLocalMinima();
	}
}
//------------------------------------------------------------------------------

void Clipper::SwapPositionsInAEL(TEdge *edge1, TEdge *edge2)
{
	TEdge *prev, *next;

	if(  !( edge1->nextInAEL ) &&  !( edge1->prevInAEL ) ) return;
	if(  !( edge2->nextInAEL ) &&  !( edge2->prevInAEL ) ) return;

	if(  edge1->nextInAEL == edge2 )
	{
		next = edge2->nextInAEL;
		if( next ) next->prevInAEL = edge1;
		prev = edge1->prevInAEL;
		if( prev ) prev->nextInAEL = edge2;
		edge2->prevInAEL = prev;
		edge2->nextInAEL = edge1;
		edge1->prevInAEL = edge2;
		edge1->nextInAEL = next;
	}
	else if(  edge2->nextInAEL == edge1 )
	{
		next = edge1->nextInAEL;
		if( next ) next->prevInAEL = edge2;
		prev = edge2->prevInAEL;
		if( prev ) prev->nextInAEL = edge1;
		edge1->prevInAEL = prev;
		edge1->nextInAEL = edge2;
		edge2->prevInAEL = edge1;
		edge2->nextInAEL = next;
	}
	else
	{
		next = edge1->nextInAEL;
		prev = edge1->prevInAEL;
		edge1->nextInAEL = edge2->nextInAEL;
		if( edge1->nextInAEL ) edge1->nextInAEL->prevInAEL = edge1;
		edge1->prevInAEL = edge2->prevInAEL;
		if( edge1->prevInAEL ) edge1->prevInAEL->nextInAEL = edge1;
		edge2->nextInAEL = next;
		if( edge2->nextInAEL ) edge2->nextInAEL->prevInAEL = edge2;
		edge2->prevInAEL = prev;
		if( edge2->prevInAEL ) edge2->prevInAEL->nextInAEL = edge2;
	}

	if( !edge1->prevInAEL ) m_ActiveEdges = edge1;
	else if( !edge2->prevInAEL ) m_ActiveEdges = edge2;
}
//------------------------------------------------------------------------------

TEdge *GetNextInAEL(TEdge *e, TDirection Direction)
{
	if( Direction == dLeftToRight ) return e->nextInAEL;
	else return e->prevInAEL;
}
//------------------------------------------------------------------------------

TEdge *GetPrevInAEL(TEdge *e, TDirection Direction)
{
	if( Direction == dLeftToRight ) return e->prevInAEL;
	else return e->nextInAEL;
}
//------------------------------------------------------------------------------

bool IsMinima(TEdge *e)
{
	return e  && (e->prev->nextInLML != e) && (e->next->nextInLML != e);
}
//------------------------------------------------------------------------------

bool IsMaxima(TEdge *e, double const &Y)
{
	return e  && (e->ytop == Y) &&  !e->nextInLML;
}
//------------------------------------------------------------------------------

bool IsIntermediate(TEdge *e, double const &Y)
{
	return (( e->ytop == Y ) && e->nextInLML);
}
//------------------------------------------------------------------------------

TEdge *GetMaximaPair(TEdge *e)
{
	if( !IsMaxima(e->next, e->ytop) || (e->next->xtop != e->xtop) ) return e->prev;
	else return e->next;
}
//------------------------------------------------------------------------------

void Clipper::DoMaxima(TEdge *e, double const &topY)
{
	TEdge *eNext, *eMaxPair;
	double X;

	eMaxPair = GetMaximaPair(e);
	X = e->xtop;
	eNext = e->nextInAEL;
	while( eNext != eMaxPair )
	{
		IntersectEdges( e , eNext , DoublePoint( X , topY ), (ipLeft | ipRight) );
		eNext = eNext->nextInAEL;
	}
	if(  ( e->polyIdx < 0 ) && ( eMaxPair->polyIdx < 0 ) )
	{
		DeleteFromAEL( e );
		DeleteFromAEL( eMaxPair );
	}
	else if(  ( e->polyIdx >= 0 ) && ( eMaxPair->polyIdx >= 0 ) )
	{
		IntersectEdges( e , eMaxPair , DoublePoint(X, topY), 0 );
	}
	else throw "DoMaxima error";
}
//------------------------------------------------------------------------------

void Clipper::ProcessHorizontals()
{
	TEdge *horzEdge;

	horzEdge = m_SortedEdges;
	while( horzEdge )
	{
		DeleteFromSEL( horzEdge );
		ProcessHorizontal( horzEdge );
		horzEdge = m_SortedEdges;
	}
}
//------------------------------------------------------------------------------

bool Clipper::IsTopHorz(TEdge *horzEdge, double const &XPos)
{
	TEdge *e;

	e = m_SortedEdges;
	while( e )
	{
		if(  ( XPos >= std::min(e->xbot, e->xtop) ) &&
			( XPos <= std::max(e->xbot, e->xtop) ) ) return false;
		e = e->nextInSEL;
	}
	return true;
}
//------------------------------------------------------------------------------

unsigned ProtectLeft(bool val){
	if (val) return (ipLeft | ipRight); else return (ipRight);
}
//------------------------------------------------------------------------------

unsigned ProtectRight(bool val){
	if (val) return (ipLeft | ipRight); else return (ipLeft);
}
//------------------------------------------------------------------------------

void Clipper::ProcessHorizontal(TEdge *horzEdge)
{
	TEdge *e, *eNext, *eMaxPair;
	double horzLeft, horzRight;
	TDirection Direction;

	if( horzEdge->xbot < horzEdge->xtop )
	{
		horzLeft = horzEdge->xbot; horzRight = horzEdge->xtop;
		Direction = dLeftToRight;
	} else
	{
		horzLeft = horzEdge->xtop; horzRight = horzEdge->xbot;
		Direction = dRightToLeft;
	}

	if( horzEdge->nextInLML ) eMaxPair = 0;
	else eMaxPair = GetMaximaPair( horzEdge );

	e = GetNextInAEL( horzEdge , Direction );
	while( e )
	{
		eNext = GetNextInAEL( e, Direction );
		if(  ( e->xbot >= horzLeft - tolerance ) && ( e->xbot <= horzRight + tolerance ) )
		{
			//ok, we seem to be in range of the horizontal edge ...
			if(  ( e->xbot == horzEdge->xtop ) && horzEdge->nextInLML  &&
				(e->dx < horzEdge->nextInLML->dx) ) break;
			if( e == eMaxPair )
			{
				//horzEdge is evidently a maxima horizontal and we've arrived at its end.
				IntersectEdges( e , horzEdge , DoublePoint( e->xbot , horzEdge->ybot ), ipRight );
				break;
			}
			else if(  IsHorizontal(*e) &&  !IsMinima(e) &&  !(e->xbot > e->xtop) )
			{
				if(  Direction == dLeftToRight )
					IntersectEdges( horzEdge , e , DoublePoint(e->xbot, horzEdge->ybot),
						ProtectRight(!IsTopHorz( horzEdge , e->xbot )) );
				else
					IntersectEdges( e , horzEdge , DoublePoint(e->xbot, horzEdge->ybot),
						ProtectLeft(!IsTopHorz( horzEdge , e->xbot )) );
			}
			else if( Direction == dLeftToRight )
			{
				IntersectEdges( horzEdge , e , DoublePoint(e->xbot, horzEdge->ybot),
					ProtectRight(!IsTopHorz( horzEdge , e->xbot )) );
			}
			else
			{
				IntersectEdges( e , horzEdge , DoublePoint(e->xbot, horzEdge->ybot),
					ProtectLeft(!IsTopHorz( horzEdge , e->xbot )) );
			}
			SwapPositionsInAEL( horzEdge , e );
		}
		else if(  ( Direction == dLeftToRight ) &&
			( e->xbot > horzRight + tolerance ) &&  !horzEdge->nextInSEL ) break;
		else if(  ( Direction == dRightToLeft ) &&
			( e->xbot < horzLeft - tolerance ) &&  !horzEdge->nextInSEL  ) break;
		e = eNext;
	} //end while ( e )

	if( horzEdge->nextInLML )
	{
		if( horzEdge->polyIdx >= 0 )
			AddPolyPt( horzEdge->polyIdx ,
				DoublePoint(horzEdge->xtop, horzEdge->ytop), horzEdge->side == esLeft);
		UpdateEdgeIntoAEL( horzEdge );
	}
	else DeleteFromAEL( horzEdge );
}
//------------------------------------------------------------------------------

bool Clipper::Execute(TClipType clipType, TPolyPolygon &polypoly)
{
	double ybot; double ytop;

	polypoly.resize(0);
	if(  m_ExecuteLocked || !InitializeScanbeam() ) return false;
	try {
		m_ExecuteLocked = true;
		m_ActiveEdges = 0;
		m_SortedEdges = 0;
		m_ClipType = clipType;

		ybot = PopScanbeam();
		do {
			InsertLocalMinimaIntoAEL( ybot );
			ProcessHorizontals();
			ytop = PopScanbeam();
			ProcessIntersections( ytop );
			ProcessEdgesAtTopOfScanbeam( ytop );
			ybot = ytop;
		} while( m_Scanbeam );

		//build the return polygons ...
		BuildResult(polypoly);
		DisposeAllPolyPts();
		m_ExecuteLocked = false;
		return true;
	}
	catch(...) {
		//returns false ...
	}
	DisposeAllPolyPts();
	m_ExecuteLocked = false;
	return false;
}
//------------------------------------------------------------------------------

void Clipper::BuildResult(TPolyPolygon &polypoly){
	unsigned i, j, k, cnt;
	TPolyPt *pt;

	k = 0;
	polypoly.resize(m_PolyPts.size());
	for (i = 0; i < m_PolyPts.size(); i++) {
		if (m_PolyPts[i]) {
			pt = m_PolyPts[i];

			//first, validate the orientation of simple polygons ...
			if ( ForceAlternateOrientation() &&
				!ValidateOrientation(pt) ) ReversePolyPtLinks(*pt);

			cnt = 0;
			do {
				pt = pt->next;
				cnt++;
			} while (pt != m_PolyPts[i]);
			if ( cnt < 2 ) continue;

			polypoly[k].resize(cnt);
			for (j = 0; j < cnt; j++) {
				polypoly[k][j].X = pt->pt.X;
				polypoly[k][j].Y = pt->pt.Y;
				pt = pt->next;
			}
			k++;
		}
	}
	polypoly.resize(k);
}
//------------------------------------------------------------------------------

bool Clipper::ForceAlternateOrientation(){
	return m_ForceAlternateOrientation;
}
//------------------------------------------------------------------------------

void Clipper::ForceAlternateOrientation(bool value){
	m_ForceAlternateOrientation = value;
}
//------------------------------------------------------------------------------

TEdge *Clipper::BubbleSwap(TEdge *edge)
{
	TEdge *result, *e;
	int i, cnt = 1;

	result = edge->nextInAEL;
	while( result  && ( std::fabs(result->xbot - edge->xbot) <= tolerance ) )
	{
		++cnt;
		result = result->nextInAEL;
	}

	if( cnt == 2 )
	{
		if( SlopesEqual(*edge, *edge->nextInAEL) ||
			(edge->dx > edge->nextInAEL->dx)) return result;
		IntersectEdges( edge , edge->nextInAEL ,
			DoublePoint(edge->xbot, edge->ybot), 0 );
		SwapPositionsInAEL( edge, edge->nextInAEL );
	}
	else
	{
		//create the sort list ...
		try {
			m_SortedEdges = edge;
			edge->prevInSEL = 0;
			e = edge->nextInAEL;
			for( i = 2 ; i <= cnt ; ++i )
			{
				e->prevInSEL = e->prevInAEL;
				e->prevInSEL->nextInSEL = e;
				if(  i == cnt ) e->nextInSEL = 0;
				e = e->nextInAEL;
			}
			while( m_SortedEdges  && m_SortedEdges->nextInSEL )
			{
				e = m_SortedEdges;
				while( e->nextInSEL )
				{
					if( e->nextInSEL->dx > e->dx )
					{
						IntersectEdges( e, e->nextInSEL, DoublePoint(e->xbot, e->ybot), 0 );
						SwapPositionsInAEL( e , e->nextInSEL );
						SwapWithNextInSEL( e );
					}
					else
						e = e->nextInSEL;
				}
				e->prevInSEL->nextInSEL = 0; //removes 'e' from SEL
			}
		}
		catch(...) {
			m_SortedEdges = 0;
			throw "BubbleSwap error";
		}
		m_SortedEdges = 0;
	}
return result;
}
//------------------------------------------------------------------------------

void Clipper::ProcessEdgesAtTopOfScanbeam( double const &topY)
{
	TEdge *e, *ePrior;

	e = m_ActiveEdges;
	while( e )
	{
		//1. process all maxima ...
		//   logic behind code - maxima are treated as if 'bent' horizontal edges
		if( IsMaxima(e, topY) &&  !IsHorizontal(*GetMaximaPair(e)) )
		{
			//'e' might be removed from AEL, as may any following edges so ...
			ePrior = e->prevInAEL;
			DoMaxima( e , topY );
			if( !ePrior ) e = m_ActiveEdges;
			else e = ePrior->nextInAEL;
		}
		else
		{
			//2. promote horizontal edges, otherwise update xbot and ybot ...
			if(  IsIntermediate( e , topY ) && IsHorizontal( *e->nextInLML ) )
			{
				if(  ( e->polyIdx >= 0 ) )
					AddPolyPt( e->polyIdx , DoublePoint( e->xtop , e->ytop ) , e->side == esLeft );
				UpdateEdgeIntoAEL( e );
				AddHorzEdgeToSEL( e );
			} else
			{
				//this just simplifies horizontal processing ...
				e->xbot = TopX( e , topY );
				e->ybot = topY;
			}
			e = e->nextInAEL;
		}
	}

	//3. Process horizontals at the top of the scanbeam ...
	ProcessHorizontals();

	//4. Promote intermediate vertices ...
	e = m_ActiveEdges;
	while( e )
	{
		if( IsIntermediate( e, topY ) )
		{
			if( e->polyIdx >= 0 )
				AddPolyPt( e->polyIdx , DoublePoint(e->xtop, e->ytop), e->side == esLeft );
			UpdateEdgeIntoAEL(e);
		}
		e = e->nextInAEL;
	}

	//5. Process (non-horizontal) intersections at the top of the scanbeam ...
	e = m_ActiveEdges;
	while( e )
	{
		if( !e->nextInAEL ) break;
		if( e->nextInAEL->xbot < e->xbot - tolerance )
			throw "ProcessEdgesAtTopOfScanbeam: Broken AEL order";
		if( e->nextInAEL->xbot > e->xbot + tolerance )
			e = e->nextInAEL;
		else
			e = BubbleSwap( e );
	}

}
//------------------------------------------------------------------------------

void Clipper::AddLocalMaxPoly(TEdge *e1, TEdge *e2, TDoublePoint const &pt)
{
	AddPolyPt( e1->polyIdx , pt , e1->side == esLeft );
	if(  EdgesShareSamePoly(*e1, *e2) )
	{
		e1->polyIdx = -1;
		e2->polyIdx = -1;
	}
	else AppendPolygon( e1, e2 );
}
//------------------------------------------------------------------------------

int Clipper::AddPolyPt(int idx, TDoublePoint const &pt, bool ToFront)
{
	TPolyPt *pp, *newPolyPt;
	TEdge *e;

	newPolyPt = new TPolyPt;
	newPolyPt->pt = pt;
	if(  idx < 0 )
	{
		m_PolyPts.push_back(newPolyPt);
		newPolyPt->next = newPolyPt;
		newPolyPt->prev = newPolyPt;

		newPolyPt->isHole = false;
		if( m_ForceAlternateOrientation )
		{
			e = m_ActiveEdges;
			while( e  && ( e->xbot < pt.X ) )
			{
				if( e->polyIdx >= 0 ) newPolyPt->isHole = !newPolyPt->isHole;
				e = e->nextInAEL;
			}
		}
		return m_PolyPts.size()-1;
	} else
	{
		pp = m_PolyPts[idx];

		if( (ToFront && PointsEqual(pt, pp->pt)) ||
			(!ToFront && PointsEqual(pt, pp->prev->pt)) )
		{
			delete newPolyPt;
			return idx;
		}

		newPolyPt->isHole = pp->isHole;
		newPolyPt->next = pp;
		newPolyPt->prev = pp->prev;
		newPolyPt->prev->next = newPolyPt;
		pp->prev = newPolyPt;
		if( ToFront ) m_PolyPts[idx] = newPolyPt;
		return idx;
	}
}
//------------------------------------------------------------------------------

void Clipper::AddLocalMinPoly(TEdge *e1, TEdge *e2, TDoublePoint const &pt)
{
	e1->polyIdx = AddPolyPt( e1->polyIdx , pt , true );
	e2->polyIdx = e1->polyIdx;
}
//------------------------------------------------------------------------------

void Clipper::AppendPolygon(TEdge *e1, TEdge *e2)
{
	TPolyPt *p1_lft, *p1_rt, *p2_lft, *p2_rt;
	TEdgeSide side;
	TEdge *e;
	int ObsoleteIdx;

	if( (e1->polyIdx < 0) || (e2->polyIdx < 0) ) throw  "AppendPolygon error";

	//get the start and ends of both output polygons ...
	p1_lft = m_PolyPts[e1->polyIdx];
	p1_rt = p1_lft->prev;
	p2_lft = m_PolyPts[e2->polyIdx];
	p2_rt = p2_lft->prev;

	//join e2 poly onto e1 poly and delete pointers to e2 ...
	if(  e1->side == esLeft )
	{
		if(  e2->side == esLeft )
		{
			//z y x a b c
			ReversePolyPtLinks(*p2_lft);
			p2_lft->next = p1_lft;
			p1_lft->prev = p2_lft;
			p1_rt->next = p2_rt;
			p2_rt->prev = p1_rt;
			m_PolyPts[e1->polyIdx] = p2_rt;
		} else
		{
			//x y z a b c
			p2_rt->next = p1_lft;
			p1_lft->prev = p2_rt;
			p2_lft->prev = p1_rt;
			p1_rt->next = p2_lft;
			m_PolyPts[e1->polyIdx] = p2_lft;
		}
		side = esLeft;
	} else
	{
		if(  e2->side == esRight )
		{
			//a b c z y x
			ReversePolyPtLinks( *p2_lft );
			p1_rt->next = p2_rt;
			p2_rt->prev = p1_rt;
			p2_lft->next = p1_lft;
			p1_lft->prev = p2_lft;
		} else
		{
			//a b c x y z
			p1_rt->next = p2_lft;
			p2_lft->prev = p1_rt;
			p1_lft->prev = p2_rt;
			p2_rt->next = p1_lft;
		}
		side = esRight;
	}

	ObsoleteIdx = e2->polyIdx;
	e2->polyIdx = -1;
	e = m_ActiveEdges;
	while( e )
	{
		if( e->
		polyIdx == ObsoleteIdx )
		{
			e->polyIdx = e1->polyIdx;
			e->side = side;
			break;
		}
		e = e->nextInAEL;
	}
	e1->polyIdx = -1;
	m_PolyPts[ObsoleteIdx] = 0;
}
//------------------------------------------------------------------------------

} //namespace clipper

