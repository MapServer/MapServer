/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  4.2.0                                                           *
* Date      :  11 April 2011                                                   *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2011                                         *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*                                                                              *
* Attributions:                                                                *
* The code in this library is an extension of Bala Vatti's clipping algorithm: *
* "A generic solution to polygon clipping"                                     *
* Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.             *
* http://portal.acm.org/citation.cfm?id=129906                                 *
*                                                                              *
* Computer graphics and geometric modeling: implementation and algorithms      *
* By Max K. Agoston                                                            *
* Springer; 1 edition (January 4, 2005)                                        *
* http://books.google.com/books?q=vatti+clipping+agoston                       *
*                                                                              *
*******************************************************************************/

/*******************************************************************************
*                                                                              *
* This is a translation of the Delphi Clipper library and the naming style     *
* used has retained a Delphi flavour.                                          *
*                                                                              *
*******************************************************************************/

#include "clipper.hpp"
#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

namespace clipper {

static double const horizontal = -3.4E+38;
static double const pi = 3.14159265359;
enum Direction { dRightToLeft, dLeftToRight };
enum Position  { pFirst, pMiddle, pSecond };

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool IsClockwise(const Polygon &poly)
{
  int highI = poly.size() -1;
  if (highI < 2) return false;
  double a;
  a = static_cast<double>(poly[highI].X) * static_cast<double>(poly[0].Y) -
    static_cast<double>(poly[0].X) * static_cast<double>(poly[highI].Y);
  for (int i = 0; i < highI; ++i)
    a += static_cast<double>(poly[i].X) * static_cast<double>(poly[i+1].Y) -
      static_cast<double>(poly[i+1].X) * static_cast<double>(poly[i].Y);
  //area := area/2;
  return a > 0; //ie reverse of normal formula because assume Y axis inverted
}
//------------------------------------------------------------------------------

bool IsClockwise(PolyPt *pt)
{
  double a = 0;
  PolyPt* startPt = pt;
  do
  {
    a += static_cast<double>(pt->pt.X) * static_cast<double>(pt->next->pt.Y) -
      static_cast<double>(pt->next->pt.X) * static_cast<double>(pt->pt.Y);
    pt = pt->next;
  }
  while (pt != startPt);
  //area = area /2;
  return a > 0; //ie reverse of normal formula because Y axis inverted
}
//------------------------------------------------------------------------------

inline bool PointsEqual( const IntPoint &pt1, const IntPoint &pt2)
{
  return ( pt1.X == pt2.X && pt1.Y == pt2.Y );
}
//------------------------------------------------------------------------------

double Area(const Polygon &poly)
{
  int highI = poly.size() -1;
  if (highI < 2) return 0;
  double a;
  a = static_cast<double>(poly[highI].X) * static_cast<double>(poly[0].Y) -
    static_cast<double>(poly[0].X) * static_cast<double>(poly[highI].Y);
  for (int i = 0; i < highI; ++i)
    a += static_cast<double>(poly[i].X) * static_cast<double>(poly[i+1].Y) -
      static_cast<double>(poly[i+1].X) * static_cast<double>(poly[i].Y);
  return a/2;
}
//------------------------------------------------------------------------------

bool PointIsVertex(const IntPoint &pt, PolyPt *pp)
{
  PolyPt *pp2 = pp;
  do
  {
    if (PointsEqual(pp2->pt, pt)) return true;
    pp2 = pp2->next;
  }
  while (pp2 != pp);
  return false;
}
//------------------------------------------------------------------------------

bool PointInPolygon(const IntPoint &pt, PolyPt *pp)
{
  PolyPt *pp2 = pp;
  bool result = false;
  do
  {
    if ((((pp2->pt.Y <= pt.Y) && (pt.Y < pp2->prev->pt.Y)) ||
      ((pp2->prev->pt.Y <= pt.Y) && (pt.Y < pp2->pt.Y))) &&
      (pt.X - pp2->pt.X < (pp2->prev->pt.X - pp2->pt.X) * (pt.Y - pp2->pt.Y) /
      (pp2->prev->pt.Y - pp2->pt.Y))) result = !result;
    pp2 = pp2->next;
  }
  while (pp2 != pp);
  return result;
}
//------------------------------------------------------------------------------

bool SlopesEqual(TEdge &e1, TEdge &e2)
{
  if (e1.ybot == e1.ytop) return (e2.ybot == e2.ytop);
  else if (e2.ybot == e2.ytop) return false;
  else return (e1.ytop - e1.ybot)*(e2.xtop - e2.xbot) -
      (e1.xtop - e1.xbot)*(e2.ytop - e2.ybot) == 0;
}
//------------------------------------------------------------------------------

bool SlopesEqual(const IntPoint pt1, const IntPoint pt2, const IntPoint pt3)
{
  if (pt1.Y == pt2.Y) return (pt2.Y == pt3.Y);
  else if (pt2.Y == pt3.Y) return false;
  else return
    (pt1.Y-pt2.Y)*(pt2.X-pt3.X) - (pt1.X-pt2.X)*(pt2.Y-pt3.Y) == 0;
}
//------------------------------------------------------------------------------

void SetDx(TEdge &e)
{
  if (e.ybot == e.ytop) e.dx = horizontal;
  else e.dx =
    static_cast<double>(e.xtop - e.xbot) / static_cast<double>(e.ytop - e.ybot);
}
//---------------------------------------------------------------------------

double GetDx(const IntPoint pt1, const IntPoint pt2)
{
  if (pt1.Y == pt2.Y) return horizontal;
  else return
    static_cast<double>(pt2.X - pt1.X) / static_cast<double>(pt2.Y - pt1.Y);
}
//---------------------------------------------------------------------------

void SwapSides(TEdge &edge1, TEdge &edge2)
{
  EdgeSide side =  edge1.side;
  edge1.side = edge2.side;
  edge2.side = side;
}
//------------------------------------------------------------------------------

void SwapPolyIndexes(TEdge &edge1, TEdge &edge2)
{
  int outIdx =  edge1.outIdx;
  edge1.outIdx = edge2.outIdx;
  edge2.outIdx = outIdx;
}
//------------------------------------------------------------------------------

inline long64 Round(double val)
{
  if ((val < 0)) return static_cast<long64>(val - 0.5);
  else return static_cast<long64>(val + 0.5);
}
//------------------------------------------------------------------------------

inline long64 Abs(long64 val)
{
  if ((val < 0)) return -val; else return val;
}
//------------------------------------------------------------------------------

long64 TopX(TEdge &edge, const long64 currentY)
{
  if( currentY == edge.ytop ) return edge.xtop;
  return edge.xbot + Round(edge.dx *(currentY - edge.ybot));
}
//------------------------------------------------------------------------------

long64 TopX(const IntPoint pt1, const IntPoint pt2, const long64 currentY)
{
  //preconditions: pt1.Y <> pt2.Y and pt1.Y > pt2.Y
  if (currentY >= pt1.Y) return pt1.X;
  else if (currentY == pt2.Y) return pt2.X;
  else if (pt1.X == pt2.X) return pt1.X;
  else
  {
    double q = static_cast<double>(pt1.X-pt2.X)/static_cast<double>(pt1.Y-pt2.Y);
    return static_cast<long64>(pt1.X + (currentY - pt1.Y) *q);
  }
}
//------------------------------------------------------------------------------

bool IntersectPoint(TEdge &edge1, TEdge &edge2, IntPoint &ip)
{
  double b1, b2;
  if (SlopesEqual(edge1, edge2)) return false;
  else if (edge1.dx == 0)
  {
    ip.X = edge1.xbot;
    if (edge2.dx == horizontal)
    {
      ip.Y = edge2.ybot;
    } else
    {
      b2 = edge2.ybot - (edge2.xbot/edge2.dx);
      ip.Y = Round(ip.X/edge2.dx + b2);
    }
  }
  else if (edge2.dx == 0)
  {
    ip.X = edge2.xbot;
    if (edge1.dx == horizontal)
    {
      ip.Y = edge1.ybot;
    } else
    {
      b1 = edge1.ybot - (edge1.xbot/edge1.dx);
      ip.Y = Round(ip.X/edge1.dx + b1);
    }
  } else
  {
    b1 = edge1.xbot - edge1.ybot * edge1.dx;
    b2 = edge2.xbot - edge2.ybot * edge2.dx;
    b2 = (b2-b1)/(edge1.dx - edge2.dx);
    ip.Y = Round(b2);
    ip.X = Round(edge1.dx * b2 + b1);
  }

  return
    //can be *so close* to the top of one edge that the rounded Y equals one ytop ...
    (ip.Y == edge1.ytop && ip.Y >= edge2.ytop && edge1.tmpX > edge2.tmpX) ||
    (ip.Y == edge2.ytop && ip.Y >= edge1.ytop && edge1.tmpX > edge2.tmpX) ||
    (ip.Y > edge1.ytop && ip.Y > edge2.ytop);
}
//------------------------------------------------------------------------------

void ReversePolyPtLinks(PolyPt &pp)
{
  PolyPt *pp1, *pp2;
  pp1 = &pp;
  do {
  pp2 = pp1->next;
  pp1->next = pp1->prev;
  pp1->prev = pp2;
  pp1 = pp2;
  } while( pp1 != &pp );
}
//------------------------------------------------------------------------------

void DisposePolyPts(PolyPt*& pp)
{
  if (pp == 0) return;
  PolyPt *tmpPp;
  pp->prev->next = 0;
  while( pp )
  {
    tmpPp = pp;
    pp = pp->next;
    delete tmpPp ;
  }
}
//------------------------------------------------------------------------------

void InitEdge(TEdge *e, TEdge *eNext,
  TEdge *ePrev, const IntPoint &pt, PolyType polyType)
{
  std::memset( e, 0, sizeof( TEdge ));

  e->next = eNext;
  e->prev = ePrev;
  e->xcurr = pt.X;
  e->ycurr = pt.Y;
  if (e->ycurr >= e->next->ycurr)
  {
    e->xbot = e->xcurr;
    e->ybot = e->ycurr;
    e->xtop = e->next->xcurr;
    e->ytop = e->next->ycurr;
    e->windDelta = 1;
  } else
  {
    e->xtop = e->xcurr;
    e->ytop = e->ycurr;
    e->xbot = e->next->xcurr;
    e->ybot = e->next->ycurr;
    e->windDelta = -1;
  }
  SetDx(*e);
  e->polyType = polyType;
  e->outIdx = -1;
}
//------------------------------------------------------------------------------

inline void SwapX(TEdge &e)
{
  //swap horizontal edges' top and bottom x's so they follow the natural
  //progression of the bounds - ie so their xbots will align with the
  //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
  e.xcurr = e.xtop;
  e.xtop = e.xbot;
  e.xbot = e.xcurr;
}
//------------------------------------------------------------------------------

void SwapPoints(IntPoint &pt1, IntPoint &pt2)
{
  IntPoint tmp = pt1;
  pt1 = pt2;
  pt2 = tmp;
}
//------------------------------------------------------------------------------

bool GetOverlapSegment(IntPoint pt1a, IntPoint pt1b, IntPoint pt2a,
  IntPoint pt2b, IntPoint &pt1, IntPoint &pt2)
{
  //precondition: segments are colinear.
  if ( pt1a.Y == pt1b.Y || Abs((pt1a.X - pt1b.X)/(pt1a.Y - pt1b.Y)) > 1 )
  {
    if (pt1a.X > pt1b.X) SwapPoints(pt1a, pt1b);
    if (pt2a.X > pt2b.X) SwapPoints(pt2a, pt2b);
    if (pt1a.X > pt2a.X) pt1 = pt1a; else pt1 = pt2a;
    if (pt1b.X < pt2b.X) pt2 = pt1b; else pt2 = pt2b;
    return pt1.X < pt2.X;
  } else
  {
    if (pt1a.Y < pt1b.Y) SwapPoints(pt1a, pt1b);
    if (pt2a.Y < pt2b.Y) SwapPoints(pt2a, pt2b);
    if (pt1a.Y < pt2a.Y) pt1 = pt1a; else pt1 = pt2a;
    if (pt1b.Y > pt2b.Y) pt2 = pt1b; else pt2 = pt2b;
    return pt1.Y > pt2.Y;
  }
}
//------------------------------------------------------------------------------

PolyPt* PolygonBottom(PolyPt* pp)
{
  PolyPt* p = pp->next;
  PolyPt* result = pp;
  while (p != pp)
  {
    if (p->pt.Y > result->pt.Y) result = p;
    else if (p->pt.Y == result->pt.Y && p->pt.X < result->pt.X) result = p;
    p = p->next;
  }
  return result;
}
//------------------------------------------------------------------------------

bool FindSegment(PolyPt* &pp, const IntPoint pt1, const IntPoint pt2)
{
  if (!pp) return false;
  PolyPt* pp2 = pp;
  do
  {
    if (PointsEqual(pp->pt, pt1) &&
      (PointsEqual(pp->next->pt, pt2) || PointsEqual(pp->prev->pt, pt2)))
        return true;
    pp = pp->next;
  }
  while (pp != pp2);
  return false;
}
//------------------------------------------------------------------------------

Position GetPosition(const IntPoint pt1, const IntPoint pt2, const IntPoint pt)
{
  if (PointsEqual(pt1, pt)) return pFirst;
  else if (PointsEqual(pt2, pt)) return pSecond;
  else return pMiddle;
}
//------------------------------------------------------------------------------

bool Pt3IsBetweenPt1AndPt2(const IntPoint pt1,
  const IntPoint pt2, const IntPoint pt3)
{
  if (PointsEqual(pt1, pt3) || PointsEqual(pt2, pt3)) return true;
  else if (pt1.X != pt2.X) return (pt1.X < pt3.X) == (pt3.X < pt2.X);
  else return (pt1.Y < pt3.Y) == (pt3.Y < pt2.Y);
}
//------------------------------------------------------------------------------

PolyPt* InsertPolyPtBetween(PolyPt* p1, PolyPt* p2, const IntPoint pt)
{
  PolyPt* result = new PolyPt;
  result->pt = pt;
  result->isHole = p1->isHole;
  if (p2 == p1->next)
  {
    p1->next = result;
    p2->prev = result;
    result->next = p2;
    result->prev = p1;
  } else
  {
    p2->next = result;
    p1->prev = result;
    result->next = p1;
    result->prev = p2;
  }
  return result;
}

//------------------------------------------------------------------------------
// ClipperBase class methods ...
//------------------------------------------------------------------------------

ClipperBase::ClipperBase() //constructor
{
  m_MinimaList = 0;
  m_CurrentLM = 0;
}
//------------------------------------------------------------------------------

ClipperBase::~ClipperBase() //destructor
{
  Clear();
}
//------------------------------------------------------------------------------

bool ClipperBase::AddPolygon( const Polygon &pg, PolyType polyType)
{
  int len = pg.size();
  if (len < 3) return false;
  Polygon p(len);
  p[0] = pg[0];
  int j = 0;
  const long64 MaxVal = 1500000000; //~ Sqrt(2^63)/2

  for (int i = 1; i < len; ++i)
  {
    if (Abs(pg[i].X) > MaxVal|| Abs(pg[i].Y) > MaxVal)
      throw clipperException("Integer exceeds range bounds");
    else if (PointsEqual(p[j], pg[i])) continue;
    else if (j > 0 && SlopesEqual(p[j-1], p[j], pg[i]))
    {
      if (PointsEqual(p[j-1], pg[i])) j--;
    } else j++;
    p[j] = pg[i];
  }
  if (j < 2) return false;

  len = j+1;
  for (;;)
  {
    //nb: test for point equality before testing slopes ...
    if (PointsEqual(p[j], p[0])) j--;
    else if (PointsEqual(p[0], p[1]) || SlopesEqual(p[j], p[0], p[1]))
      p[0] = p[j--];
    else if (SlopesEqual(p[j-1], p[j], p[0])) j--;
    else if (SlopesEqual(p[0], p[1], p[2]))
    {
      for (int i = 2; i <= j; ++i) p[i-1] = p[i];
      j--;
    }
    //exit loop if nothing is changed or there are too few vertices ...
    if (j == len-1 || j < 2) break;
    len = j +1;
  }
  if (len < 3) return false;

  //create a new edge array ...
  TEdge *edges = new TEdge [len];
  m_edges.push_back(edges);

  //convert vertices to a double-linked-list of edges and initialize ...
  edges[0].xcurr = p[0].X;
  edges[0].ycurr = p[0].Y;
  InitEdge(&edges[len-1], &edges[0], &edges[len-2], p[len-1], polyType);
  for (int i = len-2; i > 0; --i)
    InitEdge(&edges[i], &edges[i+1], &edges[i-1], p[i], polyType);
  InitEdge(&edges[0], &edges[1], &edges[len-1], p[0], polyType);

  //reset xcurr & ycurr and find 'eHighest' (given the Y axis coordinates
  //increase downward so the 'highest' edge will have the smallest ytop) ...
  TEdge *e = &edges[0];
  TEdge *eHighest = e;
  do
  {
    e->xcurr = e->xbot;
    e->ycurr = e->ybot;
    if (e->ytop < eHighest->ytop) eHighest = e;
    e = e->next;
  }
  while ( e != &edges[0]);

  //make sure eHighest is positioned so the following loop works safely ...
  if (eHighest->windDelta > 0) eHighest = eHighest->next;
  if (eHighest->dx == horizontal) eHighest = eHighest->next;

  //finally insert each local minima ...
  e = eHighest;
  do {
    e = AddBoundsToLML(e);
  }
  while( e != eHighest );
  return true;
}
//------------------------------------------------------------------------------

void ClipperBase::InsertLocalMinima(LocalMinima *newLm)
{
  if( ! m_MinimaList )
  {
    m_MinimaList = newLm;
  }
  else if( newLm->Y >= m_MinimaList->Y )
  {
    newLm->next = m_MinimaList;
    m_MinimaList = newLm;
  } else
  {
    LocalMinima* tmpLm = m_MinimaList;
    while( tmpLm->next  && ( newLm->Y < tmpLm->next->Y ) )
      tmpLm = tmpLm->next;
    newLm->next = tmpLm->next;
    tmpLm->next = newLm;
  }
}
//------------------------------------------------------------------------------

TEdge* ClipperBase::AddBoundsToLML(TEdge *e)
{
  //Starting at the top of one bound we progress to the bottom where there's
  //a local minima. We then go to the top of the next bound. These two bounds
  //form the left and right (or right and left) bounds of the local minima.
  e->nextInLML = 0;
  e = e->next;
  for (;;)
  {
    if ( e->dx == horizontal )
    {
      //nb: proceed through horizontals when approaching from their right,
      //    but break on horizontal minima if approaching from their left.
      //    This ensures 'local minima' are always on the left of horizontals.
      if (e->next->ytop < e->ytop && e->next->xbot > e->prev->xbot) break;
      if (e->xtop != e->prev->xbot) SwapX(*e);
      e->nextInLML = e->prev;
    }
    else if (e->ycurr == e->prev->ycurr) break;
    else e->nextInLML = e->prev;
    e = e->next;
  }

  //e and e.prev are now at a local minima ...
  LocalMinima* newLm = new LocalMinima;
  newLm->next = 0;
  newLm->Y = e->prev->ybot;

  if ( e->dx == horizontal ) //horizontal edges never start a left bound
  {
    if (e->xbot != e->prev->xbot) SwapX(*e);
    newLm->leftBound = e->prev;
    newLm->rightBound = e;
  } else if (e->dx < e->prev->dx)
  {
    newLm->leftBound = e->prev;
    newLm->rightBound = e;
  } else
  {
    newLm->leftBound = e;
    newLm->rightBound = e->prev;
  }
  newLm->leftBound->side = esLeft;
  newLm->rightBound->side = esRight;
  InsertLocalMinima( newLm );

  for (;;)
  {
    if ( e->next->ytop == e->ytop && e->next->dx != horizontal ) break;
    e->nextInLML = e->next;
    e = e->next;
    if ( e->dx == horizontal && e->xbot != e->prev->xtop) SwapX(*e);
  }
  return e->next;
}
//------------------------------------------------------------------------------

bool ClipperBase::AddPolygons(const Polygons &ppg, PolyType polyType)
{
  bool result = false;
  for (Polygons::size_type i = 0; i < ppg.size(); ++i)
    if (AddPolygon(ppg[i], polyType)) result = true;
  return result;
}
//------------------------------------------------------------------------------

void ClipperBase::Clear()
{
  DisposeLocalMinimaList();
  for (EdgeList::size_type i = 0; i < m_edges.size(); ++i) delete [] m_edges[i];
  m_edges.clear();
}
//------------------------------------------------------------------------------

void ClipperBase::Reset()
{
  m_CurrentLM = m_MinimaList;
  if( !m_CurrentLM ) return; //ie nothing to process

  //reset all edges ...
  LocalMinima* lm = m_MinimaList;
  while( lm )
  {
    TEdge* e = lm->leftBound;
    while( e )
    {
      e->xcurr = e->xbot;
      e->ycurr = e->ybot;
      e->side = esLeft;
      e->outIdx = -1;
      e = e->nextInLML;
    }
    e = lm->rightBound;
    while( e )
    {
      e->xcurr = e->xbot;
      e->ycurr = e->ybot;
      e->side = esRight;
      e->outIdx = -1;
      e = e->nextInLML;
    }
    lm = lm->next;
  }
}
//------------------------------------------------------------------------------

void ClipperBase::DisposeLocalMinimaList()
{
  while( m_MinimaList )
  {
    LocalMinima* tmpLm = m_MinimaList->next;
    delete m_MinimaList;
    m_MinimaList = tmpLm;
  }
  m_CurrentLM = 0;
}
//------------------------------------------------------------------------------

void ClipperBase::PopLocalMinima()
{
  if( ! m_CurrentLM ) return;
  m_CurrentLM = m_CurrentLM->next;
}
//------------------------------------------------------------------------------

IntRect ClipperBase::GetBounds()
{
  IntRect result;
  LocalMinima* lm = m_MinimaList;
  if (!lm)
  {
    result.left = result.top = result.right = result.bottom = 0;
    return result;
  }
  result.left = lm->leftBound->xbot;
  result.top = lm->leftBound->ybot;
  result.right = lm->leftBound->xbot;
  result.bottom = lm->leftBound->ybot;
  while (lm)
  {
    if (lm->leftBound->ybot > result.bottom)
      result.bottom = lm->leftBound->ybot;
    TEdge* e = lm->leftBound;
    for (;;) {
      while (e->nextInLML)
      {
        if (e->xbot < result.left) result.left = e->xbot;
        if (e->xbot > result.right) result.right = e->xbot;
        e = e->nextInLML;
      }
      if (e->xbot < result.left) result.left = e->xbot;
      if (e->xbot > result.right) result.right = e->xbot;
      if (e->xtop < result.left) result.left = e->xtop;
      if (e->xtop > result.right) result.right = e->xtop;
      if (e->ytop < result.top) result.top = e->ytop;

      if (e == lm->leftBound) e = lm->rightBound;
      else break;
    }
    lm = lm->next;
  }
  return result;
}


//------------------------------------------------------------------------------
// TClipper methods ...
//------------------------------------------------------------------------------

Clipper::Clipper() : ClipperBase() //constructor
{
  m_Scanbeam = 0;
  m_ActiveEdges = 0;
  m_SortedEdges = 0;
  m_IntersectNodes = 0;
  m_ExecuteLocked = false;
};
//------------------------------------------------------------------------------

Clipper::~Clipper() //destructor
{
  DisposeScanbeamList();
};
//------------------------------------------------------------------------------

void Clipper::DisposeScanbeamList()
{
  while ( m_Scanbeam ) {
  Scanbeam* sb2 = m_Scanbeam->next;
  delete m_Scanbeam;
  m_Scanbeam = sb2;
  }
}
//------------------------------------------------------------------------------

void Clipper::Reset()
{
  ClipperBase::Reset();
  m_Scanbeam = 0;
  m_ActiveEdges = 0;
  m_SortedEdges = 0;
  LocalMinima* lm = m_MinimaList;
  while (lm)
  {
    InsertScanbeam(lm->Y);
    InsertScanbeam(lm->leftBound->ytop);
    lm = lm->next;
  }
}
//------------------------------------------------------------------------------

bool Clipper::Execute(ClipType clipType, Polygons &solution,
    PolyFillType subjFillType, PolyFillType clipFillType)
{
  if( m_ExecuteLocked ) return false;
  bool succeeded;
  try {
    m_ExecuteLocked = true;
    solution.resize(0);
    Reset();
    if (!m_CurrentLM )
    {
      m_ExecuteLocked = false;
      return true;
    }
    m_SubjFillType = subjFillType;
    m_ClipFillType = clipFillType;
    m_ClipType = clipType;

    long64 botY = PopScanbeam();
    do {
      InsertLocalMinimaIntoAEL(botY);
      ClearHorzJoins();
      ProcessHorizontals();
      long64 topY = PopScanbeam();
      succeeded = ProcessIntersections(topY);
      if (succeeded) ProcessEdgesAtTopOfScanbeam(topY);
      botY = topY;
    } while( succeeded && m_Scanbeam );

    //build the return polygons ...
    if (succeeded) BuildResult(solution);
  }
  catch(...) {
    ClearJoins();
    ClearHorzJoins();
    solution.resize(0);
    succeeded = false;
  }
  ClearJoins();
  ClearHorzJoins();
  DisposeAllPolyPts();
  m_ExecuteLocked = false;
  return succeeded;
}
//------------------------------------------------------------------------------

void Clipper::InsertScanbeam(const long64 Y)
{
  if( !m_Scanbeam )
  {
    m_Scanbeam = new Scanbeam;
    m_Scanbeam->next = 0;
    m_Scanbeam->Y = Y;
  }
  else if(  Y > m_Scanbeam->Y )
  {
    Scanbeam* newSb = new Scanbeam;
    newSb->Y = Y;
    newSb->next = m_Scanbeam;
    m_Scanbeam = newSb;
  } else
  {
    Scanbeam* sb2 = m_Scanbeam;
    while( sb2->next  && ( Y <= sb2->next->Y ) ) sb2 = sb2->next;
    if(  Y == sb2->Y ) return; //ie ignores duplicates
    Scanbeam* newSb = new Scanbeam;
    newSb->Y = Y;
    newSb->next = sb2->next;
    sb2->next = newSb;
  }
}
//------------------------------------------------------------------------------

long64 Clipper::PopScanbeam()
{
  long64 Y = m_Scanbeam->Y;
  Scanbeam* sb2 = m_Scanbeam;
  m_Scanbeam = m_Scanbeam->next;
  delete sb2;
  return Y;
}
//------------------------------------------------------------------------------

void Clipper::DisposeAllPolyPts(){
  for (PolyPtList::size_type i = 0; i < m_PolyPts.size(); ++i)
    DisposePolyPts(m_PolyPts[i]);
  m_PolyPts.clear();
}
//------------------------------------------------------------------------------

void Clipper::SetWindingCount(TEdge &edge)
{
  TEdge *e = edge.prevInAEL;
  //find the edge of the same polytype that immediately preceeds 'edge' in AEL
  while ( e  && e->polyType != edge.polyType ) e = e->prevInAEL;
  if ( !e )
  {
    edge.windCnt = edge.windDelta;
    edge.windCnt2 = 0;
    e = m_ActiveEdges; //ie get ready to calc windCnt2
  } else if ( IsNonZeroFillType(edge) )
  {
    //nonZero filling ...
    if ( e->windCnt * e->windDelta < 0 )
    {
      if (Abs(e->windCnt) > 1)
      {
        if (e->windDelta * edge.windDelta < 0) edge.windCnt = e->windCnt;
        else edge.windCnt = e->windCnt + edge.windDelta;
      } else
        edge.windCnt = e->windCnt + e->windDelta + edge.windDelta;
    } else
    {
      if ( Abs(e->windCnt) > 1 && e->windDelta * edge.windDelta < 0)
        edge.windCnt = e->windCnt;
      else if ( e->windCnt + edge.windDelta == 0 )
        edge.windCnt = e->windCnt;
      else edge.windCnt = e->windCnt + edge.windDelta;
    }
    edge.windCnt2 = e->windCnt2;
    e = e->nextInAEL; //ie get ready to calc windCnt2
  } else
  {
    //even-odd filling ...
    edge.windCnt = 1;
    edge.windCnt2 = e->windCnt2;
    e = e->nextInAEL; //ie get ready to calc windCnt2
  }

  //update windCnt2 ...
  if ( IsNonZeroAltFillType(edge) )
  {
    //nonZero filling ...
    while ( e != &edge )
    {
      edge.windCnt2 += e->windDelta;
      e = e->nextInAEL;
    }
  } else
  {
    //even-odd filling ...
    while ( e != &edge )
    {
      edge.windCnt2 = (edge.windCnt2 == 0) ? 1 : 0;
      e = e->nextInAEL;
    }
  }
}
//------------------------------------------------------------------------------

bool Clipper::IsNonZeroFillType(const TEdge& edge) const
{
  if (edge.polyType == ptSubject)
    return m_SubjFillType == pftNonZero; else
    return m_ClipFillType == pftNonZero;
}
//------------------------------------------------------------------------------

bool Clipper::IsNonZeroAltFillType(const TEdge& edge) const
{
  if (edge.polyType == ptSubject)
    return m_ClipFillType == pftNonZero; else
    return m_SubjFillType == pftNonZero;
}
//------------------------------------------------------------------------------

bool Clipper::IsContributing(const TEdge& edge) const
{
  switch( m_ClipType ){
    case ctIntersection:
      if ( edge.polyType == ptSubject )
        return Abs(edge.windCnt) == 1 && edge.windCnt2 != 0; else
        return Abs(edge.windCnt2) > 0 && Abs(edge.windCnt) == 1;
    case ctUnion:
      return Abs(edge.windCnt) == 1 && edge.windCnt2 == 0;
    case ctDifference:
      if ( edge.polyType == ptSubject )
        return std::abs(edge.windCnt) == 1 && edge.windCnt2 == 0; else
        return std::abs(edge.windCnt) == 1 && edge.windCnt2 != 0;
    default: //case ctXor:
      return std::abs(edge.windCnt) == 1;
  }
}
//------------------------------------------------------------------------------

void Clipper::AddLocalMinPoly(TEdge *e1, TEdge *e2, const IntPoint &pt)
{
  if( e2->dx == horizontal || ( e1->dx > e2->dx ) )
  {
    AddPolyPt( e1, pt );
    e2->outIdx = e1->outIdx;
    e1->side = esLeft;
    e2->side = esRight;
  } else
  {
    AddPolyPt( e2, pt );
    e1->outIdx = e2->outIdx;
    e1->side = esRight;
    e2->side = esLeft;
  }
}
//------------------------------------------------------------------------------

void Clipper::AddLocalMaxPoly(TEdge *e1, TEdge *e2, const IntPoint &pt)
{
  AddPolyPt( e1, pt );
  if( e1->outIdx == e2->outIdx )
  {
    e1->outIdx = -1;
    e2->outIdx = -1;
  }
  else
    AppendPolygon( e1, e2 );
}
//------------------------------------------------------------------------------

void Clipper::AddEdgeToSEL(TEdge *edge)
{
  //SEL pointers in PEdge are reused to build a list of horizontal edges.
  //However, we don't need to worry about order with horizontal edge processing.
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

void Clipper::CopyAELToSEL()
{
  TEdge* e = m_ActiveEdges;
  m_SortedEdges = e;
  if (!m_ActiveEdges) return;
  m_SortedEdges->prevInSEL = 0;
  e = e->nextInAEL;
  while ( e )
  {
    e->prevInSEL = e->prevInAEL;
    e->prevInSEL->nextInSEL = e;
    e->nextInSEL = 0;
    e = e->nextInAEL;
  }
}
//------------------------------------------------------------------------------

void Clipper::AddJoin(TEdge *e1, TEdge *e2, int e1OutIdx)
{
  JoinRec* jr = new JoinRec;
  if (e1OutIdx >= 0)
    jr->poly1Idx = e1OutIdx; else
    jr->poly1Idx = e1->outIdx;
  jr->pt1a = IntPoint(e1->xbot, e1->ybot);
  jr->pt1b = IntPoint(e1->xtop, e1->ytop);
  jr->poly2Idx = e2->outIdx;
  jr->pt2a = IntPoint(e2->xbot, e2->ybot);
  jr->pt2b = IntPoint(e2->xtop, e2->ytop);
  m_Joins.push_back(jr);
}
//------------------------------------------------------------------------------

void Clipper::ClearJoins()
{
  for (JoinList::size_type i = 0; i < m_Joins.size(); i++)
    delete m_Joins[i];
  m_Joins.resize(0);
}
//------------------------------------------------------------------------------

void Clipper::AddHorzJoin(TEdge *e, int idx)
{
  HorzJoinRec* hj = new HorzJoinRec;
  hj->edge = e;
  hj->savedIdx = idx;
  m_HorizJoins.push_back(hj);
}
//------------------------------------------------------------------------------

void Clipper::ClearHorzJoins()
{
  for (HorzJoinList::size_type i = 0; i < m_HorizJoins.size(); i++)
    delete m_HorizJoins[i];
  m_HorizJoins.resize(0);
}
//------------------------------------------------------------------------------

void Clipper::InsertLocalMinimaIntoAEL( const long64 botY)
{
  while(  m_CurrentLM  && ( m_CurrentLM->Y == botY ) )
  {
    TEdge* lb = m_CurrentLM->leftBound;
    TEdge* rb = m_CurrentLM->rightBound;

    InsertEdgeIntoAEL( lb );
    InsertScanbeam( lb->ytop );
    InsertEdgeIntoAEL( rb );

    if ( IsNonZeroFillType( *lb) )
      rb->windDelta = -lb->windDelta;
    else
    {
      lb->windDelta = 1;
      rb->windDelta = 1;
    }
    SetWindingCount( *lb );
    rb->windCnt = lb->windCnt;
    rb->windCnt2 = lb->windCnt2;

    if(  rb->dx == horizontal )
    {
      //nb: only rightbounds can have a horizontal bottom edge
      AddEdgeToSEL( rb );
      InsertScanbeam( rb->nextInLML->ytop );
    }
    else
      InsertScanbeam( rb->ytop );

    if( IsContributing(*lb) )
      AddLocalMinPoly( lb, rb, IntPoint(lb->xcurr, m_CurrentLM->Y) );

    //if output polygons share an edge, they'll need joining later ...
    if (lb->outIdx >= 0 && lb->prevInAEL &&
      lb->prevInAEL->outIdx >= 0 && lb->prevInAEL->xcurr == lb->xbot &&
       SlopesEqual(*lb, *lb->prevInAEL))
         AddJoin(lb, lb->prevInAEL);

    //if any output polygons share an edge, they'll need joining later ...
    if (rb->outIdx >= 0)
    {
      if (rb->dx == horizontal)
      {
        for (HorzJoinList::size_type i = 0; i < m_HorizJoins.size(); ++i)
        {
          IntPoint pt, pt2; //returned by GetOverlapSegment() but unused here.
          HorzJoinRec* hj = m_HorizJoins[i];
          //if horizontals rb and hj.edge overlap, flag for joining later ...
          if (GetOverlapSegment(IntPoint(hj->edge->xbot, hj->edge->ybot),
            IntPoint(hj->edge->xtop, hj->edge->ytop),
            IntPoint(rb->xbot, rb->ybot),
            IntPoint(rb->xtop, rb->ytop), pt, pt2))
              AddJoin(hj->edge, rb, hj->savedIdx);
        }
      }
    }

    if( lb->nextInAEL != rb )
    {
      TEdge* e = lb->nextInAEL;
      IntPoint pt = IntPoint(lb->xcurr, lb->ycurr);
      while( e != rb )
      {
        if(!e) throw clipperException("InsertLocalMinimaIntoAEL: missing rightbound!");
        //nb: For calculating winding counts etc, IntersectEdges() assumes
        //that param1 will be to the right of param2 ABOVE the intersection ...
        IntersectEdges( rb , e , pt , ipNone); //order important here
        e = e->nextInAEL;
      }
    }
    PopLocalMinima();
  }
}
//------------------------------------------------------------------------------

void Clipper::DeleteFromAEL(TEdge *e)
{
  TEdge* AelPrev = e->prevInAEL;
  TEdge* AelNext = e->nextInAEL;
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
  TEdge* SelPrev = e->prevInSEL;
  TEdge* SelNext = e->nextInSEL;
  if(  !SelPrev &&  !SelNext && (e != m_SortedEdges) ) return; //already deleted
  if( SelPrev ) SelPrev->nextInSEL = SelNext;
  else m_SortedEdges = SelNext;
  if( SelNext ) SelNext->prevInSEL = SelPrev;
  e->nextInSEL = 0;
  e->prevInSEL = 0;
}
//------------------------------------------------------------------------------

void Clipper::IntersectEdges(TEdge *e1, TEdge *e2,
     const IntPoint &pt, IntersectProtects protects)
{
  //e1 will be to the left of e2 BELOW the intersection. Therefore e1 is before
  //e2 in AEL except when e1 is being inserted at the intersection point ...
  bool e1stops = !(ipLeft & protects) &&  !e1->nextInLML &&
    e1->xtop == pt.X && e1->ytop == pt.Y;
  bool e2stops = !(ipRight & protects) &&  !e2->nextInLML &&
    e2->xtop == pt.X && e2->ytop == pt.Y;
  bool e1Contributing = ( e1->outIdx >= 0 );
  bool e2contributing = ( e2->outIdx >= 0 );

  //update winding counts...
  //assumes that e1 will be to the right of e2 ABOVE the intersection
  if ( e1->polyType == e2->polyType )
  {
    if ( IsNonZeroFillType( *e1) )
    {
      if (e1->windCnt + e2->windDelta == 0 ) e1->windCnt = -e1->windCnt;
      else e1->windCnt += e2->windDelta;
      if ( e2->windCnt - e1->windDelta == 0 ) e2->windCnt = -e2->windCnt;
      else e2->windCnt -= e1->windDelta;
    } else
    {
      int oldE1WindCnt = e1->windCnt;
      e1->windCnt = e2->windCnt;
      e2->windCnt = oldE1WindCnt;
    }
  } else
  {
    if ( IsNonZeroFillType(*e2) ) e1->windCnt2 += e2->windDelta;
    else e1->windCnt2 = ( e1->windCnt2 == 0 ) ? 1 : 0;
    if ( IsNonZeroFillType(*e1) ) e2->windCnt2 -= e1->windDelta;
    else e2->windCnt2 = ( e2->windCnt2 == 0 ) ? 1 : 0;
  }

  if ( e1Contributing && e2contributing )
  {
    if ( e1stops || e2stops || std::abs(e1->windCnt) > 1 ||
      std::abs(e2->windCnt) > 1 ||
      (e1->polyType != e2->polyType && m_ClipType != ctXor) )
        AddLocalMaxPoly(e1, e2, pt); else
        DoBothEdges( e1, e2, pt );
  }
  else if ( e1Contributing )
  {
    switch( m_ClipType ) {
      case ctIntersection:
        if ( (e2->polyType == ptSubject || e2->windCnt2 != 0) &&
           std::abs(e2->windCnt) < 2 ) DoEdge1( e1, e2, pt);
        break;
      default:
        if ( std::abs(e2->windCnt) < 2 ) DoEdge1(e1, e2, pt);
    }
  }
  else if ( e2contributing )
  {
    if ( m_ClipType == ctIntersection )
    {
        if ( (e1->polyType == ptSubject || e1->windCnt2 != 0) &&
          std::abs(e1->windCnt) < 2 ) DoEdge2( e1, e2, pt );
    }
    else
      if (std::abs(e1->windCnt) < 2) DoEdge2( e1, e2, pt );

  } else
  {
    //neither edge is currently contributing ...
    if ( std::abs(e1->windCnt) > 1 && std::abs(e2->windCnt) > 1 ) ;// do nothing
    else if ( e1->polyType != e2->polyType && !e1stops && !e2stops &&
      std::abs(e1->windCnt) < 2 && std::abs(e2->windCnt) < 2 )
        AddLocalMinPoly(e1, e2, pt);
    else if ( std::abs(e1->windCnt) == 1 && std::abs(e2->windCnt) == 1 )
      switch( m_ClipType ) {
        case ctIntersection:
          if ( std::abs(e1->windCnt2) > 0 && std::abs(e2->windCnt2) > 0 )
            AddLocalMinPoly(e1, e2, pt);
          break;
        case ctUnion:
          if ( e1->windCnt2 == 0 && e2->windCnt2 == 0 )
            AddLocalMinPoly(e1, e2, pt);
          break;
        case ctDifference:
          if ( (e1->polyType == ptClip && e2->polyType == ptClip &&
            e1->windCnt2 != 0 && e2->windCnt2 != 0) ||
            (e1->polyType == ptSubject && e2->polyType == ptSubject &&
            e1->windCnt2 == 0 && e2->windCnt2 == 0) )
              AddLocalMinPoly(e1, e2, pt);
          break;
        case ctXor:
          AddLocalMinPoly(e1, e2, pt);
      }
    else if ( std::abs(e1->windCnt) < 2 && std::abs(e2->windCnt) < 2 )
      SwapSides( *e1, *e2 );
  }

  if(  (e1stops != e2stops) &&
    ( (e1stops && (e1->outIdx >= 0)) || (e2stops && (e2->outIdx >= 0)) ) )
  {
    SwapSides( *e1, *e2 );
    SwapPolyIndexes( *e1, *e2 );
  }

  //finally, delete any non-contributing maxima edges  ...
  if( e1stops ) DeleteFromAEL( e1 );
  if( e2stops ) DeleteFromAEL( e2 );
}
//------------------------------------------------------------------------------

void SetHoleState(PolyPt *pp, bool isHole)
{
  PolyPt *pp2 = pp;
  do
  {
    pp2->isHole = isHole;
    pp2 = pp2->next;
  }
  while (pp2 != pp);
}
//------------------------------------------------------------------------------

void Clipper::AppendPolygon(TEdge *e1, TEdge *e2)
{
  //get the start and ends of both output polygons ...
  PolyPt* p1_lft = m_PolyPts[e1->outIdx];
  PolyPt* p1_rt = p1_lft->prev;
  PolyPt* p2_lft = m_PolyPts[e2->outIdx];
  PolyPt* p2_rt = p2_lft->prev;

  //fixup orientation (hole) flag if necessary ...
  if (p1_lft->isHole != p2_lft->isHole)
  {
    PolyPt *p;
    PolyPt *bottom1 = PolygonBottom(p1_lft);
    PolyPt *bottom2 = PolygonBottom(p2_lft);
    if (bottom1->pt.Y > bottom2->pt.Y) p = p2_lft;
    else if (bottom1->pt.Y < bottom2->pt.Y) p = p1_lft;
    else if (bottom1->pt.X < bottom2->pt.X) p = p2_lft;
    else if (bottom1->pt.X > bottom2->pt.X) p = p1_lft;
    //todo - the following line really only a best guess ...
    else if (bottom1->isHole) p = p1_lft; else p = p2_lft;

    SetHoleState(p, !p->isHole);
  }

  EdgeSide side;
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
      m_PolyPts[e1->outIdx] = p2_rt;
    } else
    {
      //x y z a b c
      p2_rt->next = p1_lft;
      p1_lft->prev = p2_rt;
      p2_lft->prev = p1_rt;
      p1_rt->next = p2_lft;
      m_PolyPts[e1->outIdx] = p2_lft;
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

  int OKIdx = e1->outIdx;
  int ObsoleteIdx = e2->outIdx;
  m_PolyPts[ObsoleteIdx] = 0;

  e1->outIdx = -1; //nb: safe because we only get here via AddLocalMaxPoly
  e2->outIdx = -1;

  TEdge* e = m_ActiveEdges;
  while( e )
  {
    if( e->outIdx == ObsoleteIdx )
    {
      e->outIdx = OKIdx;
      e->side = side;
      break;
    }
    e = e->nextInAEL;
  }

  for (JoinList::size_type i = 0; i < m_Joins.size(); ++i)
  {
      if (m_Joins[i]->poly1Idx == ObsoleteIdx) m_Joins[i]->poly1Idx = OKIdx;
      if (m_Joins[i]->poly2Idx == ObsoleteIdx) m_Joins[i]->poly2Idx = OKIdx;
  }

  for (HorzJoinList::size_type i = 0; i < m_HorizJoins.size(); ++i)
  {
      if (m_HorizJoins[i]->savedIdx == ObsoleteIdx)
        m_HorizJoins[i]->savedIdx = OKIdx;
  }

}
//------------------------------------------------------------------------------

PolyPt* Clipper::AddPolyPt(TEdge *e, const IntPoint &pt)
{
  bool ToFront = (e->side == esLeft);
  if(  e->outIdx < 0 )
  {
    PolyPt* newPolyPt = new PolyPt;
    newPolyPt->pt = pt;
    newPolyPt->isHole = IsHole(e);
    m_PolyPts.push_back(newPolyPt);
    newPolyPt->next = newPolyPt;
    newPolyPt->prev = newPolyPt;
    e->outIdx = m_PolyPts.size()-1;
    return newPolyPt;
  } else
  {
    PolyPt* pp = m_PolyPts[e->outIdx];
    if (ToFront && PointsEqual(pt, pp->pt)) return pp;
    if (!ToFront && PointsEqual(pt, pp->prev->pt)) return pp->prev;

    PolyPt* newPolyPt = new PolyPt;
    newPolyPt->pt = pt;
    newPolyPt->isHole = pp->isHole;
    newPolyPt->next = pp;
    newPolyPt->prev = pp->prev;
    newPolyPt->prev->next = newPolyPt;
    pp->prev = newPolyPt;
    if (ToFront) m_PolyPts[e->outIdx] = newPolyPt;
    return newPolyPt;
  }
}
//------------------------------------------------------------------------------

void Clipper::ProcessHorizontals()
{
  TEdge* horzEdge = m_SortedEdges;
  while( horzEdge )
  {
    DeleteFromSEL( horzEdge );
    ProcessHorizontal( horzEdge );
    horzEdge = m_SortedEdges;
  }
}
//------------------------------------------------------------------------------

bool Clipper::IsTopHorz(const long64 XPos)
{
  TEdge* e = m_SortedEdges;
  while( e )
  {
    if(  ( XPos >= std::min(e->xcurr, e->xtop) ) &&
      ( XPos <= std::max(e->xcurr, e->xtop) ) ) return false;
    e = e->nextInSEL;
  }
  return true;
}
//------------------------------------------------------------------------------

bool IsMinima(TEdge *e)
{
  return e  && (e->prev->nextInLML != e) && (e->next->nextInLML != e);
}
//------------------------------------------------------------------------------

bool IsMaxima(TEdge *e, const long64 Y)
{
  return e && e->ytop == Y && !e->nextInLML;
}
//------------------------------------------------------------------------------

bool IsIntermediate(TEdge *e, const long64 Y)
{
  return e->ytop == Y && e->nextInLML;
}
//------------------------------------------------------------------------------

TEdge *GetMaximaPair(TEdge *e)
{
  if( !IsMaxima(e->next, e->ytop) || (e->next->xtop != e->xtop) )
    return e->prev; else
    return e->next;
}
//------------------------------------------------------------------------------

void Clipper::SwapPositionsInAEL(TEdge *edge1, TEdge *edge2)
{
  if(  !( edge1->nextInAEL ) &&  !( edge1->prevInAEL ) ) return;
  if(  !( edge2->nextInAEL ) &&  !( edge2->prevInAEL ) ) return;

  if(  edge1->nextInAEL == edge2 )
  {
    TEdge* next = edge2->nextInAEL;
    if( next ) next->prevInAEL = edge1;
    TEdge* prev = edge1->prevInAEL;
    if( prev ) prev->nextInAEL = edge2;
    edge2->prevInAEL = prev;
    edge2->nextInAEL = edge1;
    edge1->prevInAEL = edge2;
    edge1->nextInAEL = next;
  }
  else if(  edge2->nextInAEL == edge1 )
  {
    TEdge* next = edge1->nextInAEL;
    if( next ) next->prevInAEL = edge2;
    TEdge* prev = edge2->prevInAEL;
    if( prev ) prev->nextInAEL = edge1;
    edge1->prevInAEL = prev;
    edge1->nextInAEL = edge2;
    edge2->prevInAEL = edge1;
    edge2->nextInAEL = next;
  }
  else
  {
    TEdge* next = edge1->nextInAEL;
    TEdge* prev = edge1->prevInAEL;
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

void Clipper::SwapPositionsInSEL(TEdge *edge1, TEdge *edge2)
{
  if(  !( edge1->nextInSEL ) &&  !( edge1->prevInSEL ) ) return;
  if(  !( edge2->nextInSEL ) &&  !( edge2->prevInSEL ) ) return;

  if(  edge1->nextInSEL == edge2 )
  {
    TEdge* next = edge2->nextInSEL;
    if( next ) next->prevInSEL = edge1;
    TEdge* prev = edge1->prevInSEL;
    if( prev ) prev->nextInSEL = edge2;
    edge2->prevInSEL = prev;
    edge2->nextInSEL = edge1;
    edge1->prevInSEL = edge2;
    edge1->nextInSEL = next;
  }
  else if(  edge2->nextInSEL == edge1 )
  {
    TEdge* next = edge1->nextInSEL;
    if( next ) next->prevInSEL = edge2;
    TEdge* prev = edge2->prevInSEL;
    if( prev ) prev->nextInSEL = edge1;
    edge1->prevInSEL = prev;
    edge1->nextInSEL = edge2;
    edge2->prevInSEL = edge1;
    edge2->nextInSEL = next;
  }
  else
  {
    TEdge* next = edge1->nextInSEL;
    TEdge* prev = edge1->prevInSEL;
    edge1->nextInSEL = edge2->nextInSEL;
    if( edge1->nextInSEL ) edge1->nextInSEL->prevInSEL = edge1;
    edge1->prevInSEL = edge2->prevInSEL;
    if( edge1->prevInSEL ) edge1->prevInSEL->nextInSEL = edge1;
    edge2->nextInSEL = next;
    if( edge2->nextInSEL ) edge2->nextInSEL->prevInSEL = edge2;
    edge2->prevInSEL = prev;
    if( edge2->prevInSEL ) edge2->prevInSEL->nextInSEL = edge2;
  }

  if( !edge1->prevInSEL ) m_SortedEdges = edge1;
  else if( !edge2->prevInSEL ) m_SortedEdges = edge2;
}
//------------------------------------------------------------------------------

TEdge* GetNextInAEL(TEdge *e, Direction dir)
{
  if( dir == dLeftToRight ) return e->nextInAEL;
  else return e->prevInAEL;
}
//------------------------------------------------------------------------------

void Clipper::ProcessHorizontal(TEdge *horzEdge)
{
  Direction dir;
  long64 horzLeft, horzRight;

  if( horzEdge->xcurr < horzEdge->xtop )
  {
    horzLeft = horzEdge->xcurr;
    horzRight = horzEdge->xtop;
    dir = dLeftToRight;
  } else
  {
    horzLeft = horzEdge->xtop;
    horzRight = horzEdge->xcurr;
    dir = dRightToLeft;
  }

  TEdge* eMaxPair;
  if( horzEdge->nextInLML ) eMaxPair = 0;
  else eMaxPair = GetMaximaPair(horzEdge);

  TEdge* e = GetNextInAEL( horzEdge , dir );
  while( e )
  {
    TEdge* eNext = GetNextInAEL( e, dir );
    if( e->xcurr >= horzLeft && e->xcurr <= horzRight )
    {
      //ok, so far it looks like we're still in range of the horizontal edge
      if ( e->xcurr == horzEdge->xtop && horzEdge->nextInLML)
      {
        if (SlopesEqual(*e, *horzEdge->nextInLML))
        {
          //if output polygons share an edge, they'll need joining later ...
          if (horzEdge->outIdx >= 0 && e->outIdx >= 0)
            AddJoin(horzEdge->nextInLML, e, horzEdge->outIdx);
          break; //we've reached the end of the horizontal line
        }
        else if (e->dx < horzEdge->nextInLML->dx)
        //we really have got to the end of the intermediate horz edge so quit.
        //nb: More -ve slopes follow more +ve slopes ABOVE the horizontal.
          break;
      }

      if( e == eMaxPair )
      {
        //horzEdge is evidently a maxima horizontal and we've arrived at its end.
        if (dir == dLeftToRight)
          IntersectEdges(horzEdge, e, IntPoint(e->xcurr, horzEdge->ycurr), ipNone);
        else
          IntersectEdges(e, horzEdge, IntPoint(e->xcurr, horzEdge->ycurr), ipNone);
        return;
      }
      else if( e->dx == horizontal &&  !IsMinima(e) && !(e->xcurr > e->xtop) )
      {
        //An overlapping horizontal edge. Overlapping horizontal edges are
        //processed as if layered with the current horizontal edge (horizEdge)
        //being infinitesimally lower that the next (e). Therfore, we
        //intersect with e only if e.xcurr is within the bounds of horzEdge ...
        if( dir == dLeftToRight )
          IntersectEdges( horzEdge , e, IntPoint(e->xcurr, horzEdge->ycurr),
            (IsTopHorz( e->xcurr ))? ipLeft : ipBoth );
        else
          IntersectEdges( e, horzEdge, IntPoint(e->xcurr, horzEdge->ycurr),
            (IsTopHorz( e->xcurr ))? ipRight : ipBoth );
      }
      else if( dir == dLeftToRight )
      {
        IntersectEdges( horzEdge, e, IntPoint(e->xcurr, horzEdge->ycurr),
          (IsTopHorz( e->xcurr ))? ipLeft : ipBoth );
      }
      else
      {
        IntersectEdges( e, horzEdge, IntPoint(e->xcurr, horzEdge->ycurr),
          (IsTopHorz( e->xcurr ))? ipRight : ipBoth );
      }
      SwapPositionsInAEL( horzEdge, e );
    }
    else if( dir == dLeftToRight &&
      e->xcurr > horzRight  && m_SortedEdges ) break;
    else if( dir == dRightToLeft &&
      e->xcurr < horzLeft && m_SortedEdges ) break;
    e = eNext;
  } //end while

  if( horzEdge->nextInLML )
  {
    if( horzEdge->outIdx >= 0 )
      AddPolyPt( horzEdge, IntPoint(horzEdge->xtop, horzEdge->ytop));
    UpdateEdgeIntoAEL( horzEdge );
  }
  else
  {
    if ( horzEdge->outIdx >= 0 )
      IntersectEdges( horzEdge, eMaxPair,
      IntPoint(horzEdge->xtop, horzEdge->ycurr), ipBoth);
    if (eMaxPair->outIdx >= 0) throw clipperException("ProcessHorizontal error");
    DeleteFromAEL(eMaxPair);
    DeleteFromAEL(horzEdge);
  }
}
//------------------------------------------------------------------------------

void Clipper::UpdateEdgeIntoAEL(TEdge *&e)
{
  if( !e->nextInLML ) throw
    clipperException("UpdateEdgeIntoAEL: invalid call");
  TEdge* AelPrev = e->prevInAEL;
  TEdge* AelNext = e->nextInAEL;
  e->nextInLML->outIdx = e->outIdx;
  if( AelPrev ) AelPrev->nextInAEL = e->nextInLML;
  else m_ActiveEdges = e->nextInLML;
  if( AelNext ) AelNext->prevInAEL = e->nextInLML;
  e->nextInLML->side = e->side;
  e->nextInLML->windDelta = e->windDelta;
  e->nextInLML->windCnt = e->windCnt;
  e->nextInLML->windCnt2 = e->windCnt2;
  e = e->nextInLML;
  e->prevInAEL = AelPrev;
  e->nextInAEL = AelNext;
  if( e->dx != horizontal )
  {
    InsertScanbeam( e->ytop );
    //if output polygons share an edge, they'll need joining later ...
    if (e->outIdx >= 0 && AelPrev && AelPrev->outIdx >= 0 &&
      AelPrev->xbot == e->xcurr && SlopesEqual(*e, *AelPrev))
        AddJoin(e, AelPrev);
  }
}
//------------------------------------------------------------------------------

bool Clipper::ProcessIntersections( const long64 topY)
{
  if( !m_ActiveEdges ) return true;
  try {
    BuildIntersectList(topY);
    if ( !m_IntersectNodes) return true;
    if ( FixupIntersections() ) ProcessIntersectList();
    else return false;
  }
  catch(...) {
    m_SortedEdges = 0;
    DisposeIntersectNodes();
    throw clipperException("ProcessIntersections error");
  }
  return true;
}
//------------------------------------------------------------------------------

void Clipper::DisposeIntersectNodes()
{
  while ( m_IntersectNodes )
  {
    IntersectNode* iNode = m_IntersectNodes->next;
    delete m_IntersectNodes;
    m_IntersectNodes = iNode;
  }
}
//------------------------------------------------------------------------------

void Clipper::BuildIntersectList(const long64 topY)
{
  if ( !m_ActiveEdges ) return;

  //prepare for sorting ...
  TEdge* e = m_ActiveEdges;
  e->tmpX = TopX( *e, topY );
  m_SortedEdges = e;
  m_SortedEdges->prevInSEL = 0;
  e = e->nextInAEL;
  while( e )
  {
    e->prevInSEL = e->prevInAEL;
    e->prevInSEL->nextInSEL = e;
    e->nextInSEL = 0;
    e->tmpX = TopX( *e, topY );
    e = e->nextInAEL;
  }

  //bubblesort ...
  bool isModified = true;
  while( isModified && m_SortedEdges )
  {
    isModified = false;
    e = m_SortedEdges;
    while( e->nextInSEL )
    {
      TEdge *eNext = e->nextInSEL;
      IntPoint pt;
      if(e->tmpX > eNext->tmpX && IntersectPoint(*e, *eNext, pt))
      {
        AddIntersectNode( e, eNext, pt );
        SwapPositionsInSEL(e, eNext);
        isModified = true;
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

bool Process1Before2(IntersectNode &node1, IntersectNode &node2)
{
  bool result;
  if (node1.pt.Y == node2.pt.Y)
  {
    if (node1.edge1 == node2.edge1 || node1.edge2 == node2.edge1)
    {
      result = node2.pt.X > node1.pt.X;
      if (node2.edge1->dx > 0) return result; else return !result;
    }
    else if (node1.edge1 == node2.edge2 || node1.edge2 == node2.edge2)
    {
      result = node2.pt.X > node1.pt.X;
      if (node2.edge2->dx > 0) return result; else return !result;
    }
    else return node2.pt.X > node1.pt.X;
  }
  else return node1.pt.Y > node2.pt.Y;
}
//------------------------------------------------------------------------------

void Clipper::AddIntersectNode(TEdge *e1, TEdge *e2, const IntPoint &pt)
{
  IntersectNode* newNode = new IntersectNode;
  newNode->edge1 = e1;
  newNode->edge2 = e2;
  newNode->pt = pt;
  newNode->next = 0;
  if( !m_IntersectNodes ) m_IntersectNodes = newNode;
  else if(  Process1Before2(*newNode, *m_IntersectNodes) )
  {
    newNode->next = m_IntersectNodes;
    m_IntersectNodes = newNode;
  }
  else
  {
    IntersectNode* iNode = m_IntersectNodes;
    while( iNode->next  && Process1Before2(*iNode->next, *newNode) )
        iNode = iNode->next;
    newNode->next = iNode->next;
    iNode->next = newNode;
  }
}
//------------------------------------------------------------------------------

void Clipper::ProcessIntersectList()
{
  while( m_IntersectNodes )
  {
    IntersectNode* iNode = m_IntersectNodes->next;
    {
      IntersectEdges( m_IntersectNodes->edge1 ,
        m_IntersectNodes->edge2 , m_IntersectNodes->pt, ipBoth );
      SwapPositionsInAEL( m_IntersectNodes->edge1 , m_IntersectNodes->edge2 );
    }
    delete m_IntersectNodes;
    m_IntersectNodes = iNode;
  }
}
//------------------------------------------------------------------------------

void Clipper::DoMaxima(TEdge *e, long64 topY)
{
  TEdge* eMaxPair = GetMaximaPair(e);
  long64 X = e->xtop;
  TEdge* eNext = e->nextInAEL;
  while( eNext != eMaxPair )
  {
    if (!eNext) throw clipperException("DoMaxima error");
    IntersectEdges( e, eNext, IntPoint(X, topY), ipBoth );
    eNext = eNext->nextInAEL;
  }
  if( e->outIdx < 0 && eMaxPair->outIdx < 0 )
  {
    DeleteFromAEL( e );
    DeleteFromAEL( eMaxPair );
  }
  else if( e->outIdx >= 0 && eMaxPair->outIdx >= 0 )
  {
    IntersectEdges( e, eMaxPair, IntPoint(X, topY), ipNone );
  }
  else throw clipperException("DoMaxima error");
}
//------------------------------------------------------------------------------

void Clipper::ProcessEdgesAtTopOfScanbeam(const long64 topY)
{
  TEdge* e = m_ActiveEdges;
  while( e )
  {
    //1. process maxima, treating them as if they're 'bent' horizontal edges,
    //   but exclude maxima with horizontal edges. nb: e can't be a horizontal.
    if( IsMaxima(e, topY) && GetMaximaPair(e)->dx != horizontal )
    {
      //'e' might be removed from AEL, as may any following edges so ...
      TEdge* ePrior = e->prevInAEL;
      DoMaxima(e, topY);
      if( !ePrior ) e = m_ActiveEdges;
      else e = ePrior->nextInAEL;
    }
    else
    {
      //2. promote horizontal edges, otherwise update xcurr and ycurr ...
      if(  IsIntermediate(e, topY) && e->nextInLML->dx == horizontal )
      {
        if (e->outIdx >= 0)
        {
          AddPolyPt(e, IntPoint(e->xtop, e->ytop));
          AddHorzJoin(e->nextInLML, e->outIdx);
        }
        UpdateEdgeIntoAEL(e);
        AddEdgeToSEL(e);
      } else
      {
        //this just simplifies horizontal processing ...
        e->xcurr = TopX( *e, topY );
        e->ycurr = topY;
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
      if( e->outIdx >= 0 ) AddPolyPt(e, IntPoint(e->xtop,e->ytop));
      UpdateEdgeIntoAEL(e);
    }
    e = e->nextInAEL;
  }
}
//------------------------------------------------------------------------------

PolyPt* FixupOutPolygon(PolyPt *p)
{
  //FixupOutPolygon() - removes duplicate points and simplifies consecutive
  //parallel edges by removing the middle vertex.
  if (!p) return 0;
  PolyPt *pp = p, *result = p, *lastOK = 0;
  for (;;)
  {
    if (pp->prev == pp || pp->prev == pp->next )
    {
      DisposePolyPts(pp);
      return 0;
    }
    //test for duplicate points and for same slope (cross-product) ...
    if ( PointsEqual(pp->pt, pp->next->pt) ||
      SlopesEqual(pp->prev->pt, pp->pt, pp->next->pt) )
    {
      lastOK = 0;
      pp->prev->next = pp->next;
      pp->next->prev = pp->prev;
      PolyPt* tmp = pp;
      if (pp == result) result = pp->prev;
      pp = pp->prev;
      delete tmp;
    }
    else if (pp == lastOK) break;
    else
    {
      if (!lastOK) lastOK = pp;
      pp = pp->next;
    }
  }
  return result;
}
//------------------------------------------------------------------------------

void Clipper::BuildResult(Polygons &polypoly)
{
  for (PolyPtList::size_type i = 0; i < m_PolyPts.size(); ++i)
    if (m_PolyPts[i])
    {
      m_PolyPts[i] = FixupOutPolygon(m_PolyPts[i]);
      //fix orientation ...
      PolyPt *p = m_PolyPts[i];
      if (p && p->isHole == IsClockwise(p))
        ReversePolyPtLinks(*p);
    }
  JoinCommonEdges();

  int k = 0;
  polypoly.resize(m_PolyPts.size());
  for (unsigned i = 0; i < m_PolyPts.size(); ++i) {
    if (m_PolyPts[i]) {
      Polygon* pg = &polypoly[k];
      pg->clear();
      PolyPt* p = m_PolyPts[i];

      do {
        pg->push_back(p->pt);
        p = p->next;
      } while (p != m_PolyPts[i]);
      //make sure each polygon has at least 3 vertices ...
      if (pg->size() < 3) pg->clear(); else k++;
    }
  }
  polypoly.resize(k);
}
//------------------------------------------------------------------------------

void SwapIntersectNodes(IntersectNode &int1, IntersectNode &int2)
{
  TEdge *e1 = int1.edge1;
  TEdge *e2 = int1.edge2;
  IntPoint p = int1.pt;

  int1.edge1 = int2.edge1;
  int1.edge2 = int2.edge2;
  int1.pt = int2.pt;

  int2.edge1 = e1;
  int2.edge2 = e2;
  int2.pt = p;
}
//------------------------------------------------------------------------------

bool Clipper::FixupIntersections()
{
  if ( !m_IntersectNodes->next ) return true;

  CopyAELToSEL();
  IntersectNode *int1 = m_IntersectNodes;
  IntersectNode *int2 = m_IntersectNodes->next;
  while (int2)
  {
    TEdge *e1 = int1->edge1;
    TEdge *e2;
    if (e1->prevInSEL == int1->edge2) e2 = e1->prevInSEL;
    else if (e1->nextInSEL == int1->edge2) e2 = e1->nextInSEL;
    else
    {
      //The current intersection is out of order, so try and swap it with
      //a subsequent intersection ...
      while (int2)
      {
        if (int2->edge1->nextInSEL == int2->edge2 ||
          int2->edge1->prevInSEL == int2->edge2) break;
        else int2 = int2->next;
      }
      if ( !int2 ) return false; //oops!!!

      //found an intersect node that can be swapped ...
      SwapIntersectNodes(*int1, *int2);
      e1 = int1->edge1;
      e2 = int1->edge2;
    }
    SwapPositionsInSEL(e1, e2);
    int1 = int1->next;
    int2 = int1->next;
  }

  m_SortedEdges = 0;

  //finally, check the last intersection too ...
  return (int1->edge1->prevInSEL == int1->edge2 ||
    int1->edge1->nextInSEL == int1->edge2);
}
//------------------------------------------------------------------------------

bool E2InsertsBeforeE1(TEdge &e1, TEdge &e2)
{
  if (e2.xcurr == e1.xcurr) return e2.dx > e1.dx;
  else return e2.xcurr < e1.xcurr;
}
//------------------------------------------------------------------------------

void Clipper::InsertEdgeIntoAEL(TEdge *edge)
{
  edge->prevInAEL = 0;
  edge->nextInAEL = 0;
  if( !m_ActiveEdges )
  {
    m_ActiveEdges = edge;
  }
  else if( E2InsertsBeforeE1(*m_ActiveEdges, *edge) )
  {
    edge->nextInAEL = m_ActiveEdges;
    m_ActiveEdges->prevInAEL = edge;
    m_ActiveEdges = edge;
  } else
  {
    TEdge* e = m_ActiveEdges;
    while( e->nextInAEL  && !E2InsertsBeforeE1(*e->nextInAEL , *edge) )
      e = e->nextInAEL;
    edge->nextInAEL = e->nextInAEL;
    if( e->nextInAEL ) e->nextInAEL->prevInAEL = edge;
    edge->prevInAEL = e;
    e->nextInAEL = edge;
  }
}
//----------------------------------------------------------------------

void Clipper::DoEdge1(TEdge *edge1, TEdge *edge2, const IntPoint &pt)
{
  AddPolyPt(edge1, pt);
  SwapSides(*edge1, *edge2);
  SwapPolyIndexes(*edge1, *edge2);
}
//----------------------------------------------------------------------

void Clipper::DoEdge2(TEdge *edge1, TEdge *edge2, const IntPoint &pt)
{
  AddPolyPt(edge2, pt);
  SwapSides(*edge1, *edge2);
  SwapPolyIndexes(*edge1, *edge2);
}
//----------------------------------------------------------------------

void Clipper::DoBothEdges(TEdge *edge1, TEdge *edge2, const IntPoint &pt)
{
  AddPolyPt(edge1, pt);
  AddPolyPt(edge2, pt);
  SwapSides( *edge1 , *edge2 );
  SwapPolyIndexes( *edge1 , *edge2 );
}
//----------------------------------------------------------------------

bool Clipper::IsHole(TEdge *e)
{
  bool hole = false;
  TEdge *e2 = m_ActiveEdges;
  while (e2 && e2 != e)
  {
    if (e2->outIdx >= 0) hole = !hole;
    e2 = e2->nextInAEL;
  }
  return hole;
}
//----------------------------------------------------------------------

PolyPt* DeletePolyPt(PolyPt* pp)
{
  if (pp->next == pp)
  {
    delete pp;
    return 0;
  } else
  {
    PolyPt* result = pp->prev;
    pp->next->prev = result;
    result->next = pp->next;
    delete pp;
    return result;
  }
}
//------------------------------------------------------------------------------

PolyPt* FixSpikes(PolyPt *pp)
{
  PolyPt *pp2 = pp, *pp3;
  PolyPt *result = pp;
  do
  {
    if (SlopesEqual(pp2->prev->pt, pp2->pt, pp2->next->pt) &&
      ((((pp2->prev->pt.X < pp2->pt.X) == (pp2->next->pt.X < pp2->pt.X)) &&
      ((pp2->prev->pt.X != pp2->pt.X) || (pp2->next->pt.X != pp2->pt.X))) ||
      ((((pp2->prev->pt.Y < pp2->pt.Y) == (pp2->next->pt.Y < pp2->pt.Y))) &&
      ((pp2->prev->pt.Y != pp2->pt.Y) || (pp2->next->pt.Y != pp2->pt.Y)))))
    {
      if (pp2 == result) result = pp2->prev;
      pp3 = pp2->next;
      DeletePolyPt(pp2);
      pp2 = pp3;
    } else
      pp2 = pp2->next;
  }
  while (pp2 != result);
  return result;
}
//------------------------------------------------------------------------------

void Clipper::JoinCommonEdges()
{
  for (JoinList::size_type i = 0; i < m_Joins.size(); i++)
  {
    PolyPt *pp1a, *pp1b, *pp2a, *pp2b;
    IntPoint pt1, pt2;
    JoinRec* j = m_Joins[i];

    pp1a = m_PolyPts[j->poly1Idx];
    pp2a = m_PolyPts[j->poly2Idx];
    bool found = FindSegment(pp1a, j->pt1a, j->pt1b);
    if (found)
    {
      if (j->poly1Idx == j->poly2Idx)
      {
        //we're searching the same polygon for overlapping segments so
        //we really don't want segment 2 to be the same as segment 1 ...
        pp2a = pp1a->next;
        found = FindSegment(pp2a, j->pt2a, j->pt2b) && (pp2a != pp1a);
      }
      else
        found = FindSegment(pp2a, j->pt2a, j->pt2b);
    }

    if (found)
    {
      if (PointsEqual(pp1a->next->pt, j->pt1b))
        pp1b = pp1a->next; else pp1b = pp1a->prev;
      if (PointsEqual(pp2a->next->pt, j->pt2b))
        pp2b = pp2a->next; else pp2b = pp2a->prev;
      if (GetOverlapSegment(pp1a->pt, pp1b->pt, pp2a->pt, pp2b->pt, pt1, pt2))
      {
        PolyPt *p1, *p2, *p3, *p4;
        //get p1 & p2 polypts - the overlap start & endpoints on poly1
        Position pos1 = GetPosition(pp1a->pt, pp1b->pt, pt1);
        if (pos1 == pFirst) p1 = pp1a;
        else if (pos1 == pSecond) p1 = pp1b;
        else p1 = InsertPolyPtBetween(pp1a, pp1b, pt1);
        Position pos2 = GetPosition(pp1a->pt, pp1b->pt, pt2);
        if (pos2 == pMiddle)
        {
          if (pos1 == pMiddle)
          {
            if (Pt3IsBetweenPt1AndPt2(pp1a->pt, p1->pt, pt2))
              p2 = InsertPolyPtBetween(pp1a, p1, pt2); else
              p2 = InsertPolyPtBetween(p1, pp1b, pt2);
          }
          else if (pos2 == pFirst) p2 = pp1a;
          else p2 = pp1b;
        }
        else if (pos2 == pFirst) p2 = pp1a;
        else p2 = pp1b;
        //get p3 & p4 polypts - the overlap start & endpoints on poly2
        pos1 = GetPosition(pp2a->pt, pp2b->pt, pt1);
        if (pos1 == pFirst) p3 = pp2a;
        else if (pos1 == pSecond) p3 = pp2b;
        else p3 = InsertPolyPtBetween(pp2a, pp2b, pt1);
        pos2 = GetPosition(pp2a->pt, pp2b->pt, pt2);
        if (pos2 == pMiddle)
        {
          if (pos1 == pMiddle)
          {
            if (Pt3IsBetweenPt1AndPt2(pp2a->pt, p3->pt, pt2))
              p4 = InsertPolyPtBetween(pp2a, p3, pt2); else
              p4 = InsertPolyPtBetween(p3, pp2b, pt2);
          }
          else if (pos2 == pFirst) p4 = pp2a;
          else p4 = pp2b;
        }
        else if (pos2 == pFirst) p4 = pp2a;
        else p4 = pp2b;

        //p1.pt should equal p3.pt and p2.pt should equal p4.pt here, so ...
        //join p1 to p3 and p2 to p4 ...
        if (p1->next == p2 && p3->prev == p4)
        {
          p1->next = p3;
          p3->prev = p1;
          p2->prev = p4;
          p4->next = p2;
        }
        else if (p1->prev == p2 && p3->next == p4)
        {
          p1->prev = p3;
          p3->next = p1;
          p2->next = p4;
          p4->prev = p2;
        }
        else
          continue; //an orientation is probably wrong

        //delete duplicate points  ...
        if (PointsEqual(p1->pt, p3->pt)) DeletePolyPt(p3);
        if (PointsEqual(p2->pt, p4->pt)) DeletePolyPt(p4);

        if (j->poly2Idx == j->poly1Idx)
        {
          //instead of joining two polygons, we've just created
          //a new one by splitting one polygon into two.
          m_PolyPts[j->poly1Idx] = p1;
          m_PolyPts.push_back(p2);
          j->poly2Idx = m_PolyPts.size()-1;

          if (PointInPolygon(p2->pt, p1)) SetHoleState(p2, !p1->isHole);
          else if (PointInPolygon(p1->pt, p2)) SetHoleState(p1, !p2->isHole);

          //now fixup any subsequent m_Joins that match this polygon
          for (JoinList::size_type k = i+1; k < m_Joins.size(); k++)
          {
            JoinRec* j2 = m_Joins[k];
            if (j2->poly1Idx == j->poly1Idx && PointIsVertex(j2->pt1a, p2))
              j2->poly1Idx = j->poly2Idx;
            if (j2->poly2Idx == j->poly1Idx && PointIsVertex(j2->pt2a, p2))
              j2->poly2Idx = j->poly2Idx;
          }
        } else
        {
          //having joined 2 polygons together, delete the obsolete pointer ...
          m_PolyPts[j->poly2Idx] = 0;

          //now fixup any subsequent fJoins that match this polygon
          for (JoinList::size_type k = i+1; k < m_Joins.size(); k++)
          {
            JoinRec* j2 = m_Joins[k];
            if (j2->poly1Idx == j->poly2Idx) j2->poly1Idx = j->poly1Idx;
            if (j2->poly2Idx == j->poly2Idx) j2->poly2Idx = j->poly1Idx;
          }
          j->poly2Idx = j->poly1Idx;
        }

        //now cleanup redundant edges too ...
        m_PolyPts[j->poly1Idx] = FixSpikes(p1);
        if (j->poly2Idx != j->poly1Idx)
          m_PolyPts[j->poly2Idx] = FixSpikes(p2);

      }
    }
  }
}

//------------------------------------------------------------------------------
// OffsetPolygon functions ...
//------------------------------------------------------------------------------

struct DoublePoint
{
  double X;
  double Y;
  DoublePoint(double x = 0, double y = 0) : X(x), Y(y) {}
};

Polygon BuildArc(const IntPoint &pt,
  const double a1, const double a2, const double r)
{
  int steps = std::max(6, int(std::sqrt(std::abs(r)) * std::abs(a2 - a1)));
  Polygon result(steps);
  int n = steps - 1;
  double da = (a2 - a1) / n;
  double a = a1;
  for (int i = 0; i <= n; ++i)
  {
    result[i].X = pt.X + Round(std::cos(a)*r);
    result[i].Y = pt.Y + Round(std::sin(a)*r);
    a += da;
  }
  return result;
}
//------------------------------------------------------------------------------

DoublePoint GetUnitNormal( const IntPoint &pt1, const IntPoint &pt2)
{
  double dx = static_cast<double>(pt2.X - pt1.X);
  double dy = static_cast<double>(pt2.Y - pt1.Y);
  if(  ( dx == 0 ) && ( dy == 0 ) ) return DoublePoint( 0, 0 );

  double f = 1 *1.0/ std::sqrt( dx*dx + dy*dy );
  dx *= f;
  dy *= f;
  return DoublePoint(dy, -dx);
}
//------------------------------------------------------------------------------

Polygons OffsetPolygons(const Polygons &pts, const float &delta)
{
  if (delta == 0) return pts;

  double deltaSq = delta*delta;
  Polygons result(pts.size());

  for (int j = 0; j < (int)pts.size(); ++j)
  {
    int highI = (int)pts[j].size() -1;
    //to minimize artefacts, strip out those polygons where
    //it's shrinking and where its area < Sqr(delta) ...
    double a1 = Area(pts[j]);
    if (delta < 0) { if (a1 > 0 && a1 < deltaSq) highI = 0;}
    else if (a1 < 0 && -a1 < deltaSq) highI = 0; //nb: a hole if area < 0

    Polygon pg;
    pg.reserve(highI*2+2);

    if (highI < 2)
    {
      result.push_back(pg);
      continue;
    }

    std::vector < DoublePoint > normals(highI+1);
    normals[0] = GetUnitNormal(pts[j][highI], pts[j][0]);
    for (int i = 1; i <= highI; ++i)
      normals[i] = GetUnitNormal(pts[j][i-1], pts[j][i]);

    for (int i = 0; i < highI; ++i)
    {
      pg.push_back(IntPoint(pts[j][i].X + Round(delta *normals[i].X),
        pts[j][i].Y + Round(delta *normals[i].Y)));
      pg.push_back(IntPoint(pts[j][i].X + Round(delta *normals[i+1].X),
        pts[j][i].Y + Round(delta *normals[i+1].Y)));
    }
    pg.push_back(IntPoint(pts[j][highI].X + Round(delta *normals[highI].X),
      pts[j][highI].Y + Round(delta *normals[highI].Y)));
    pg.push_back(IntPoint(pts[j][highI].X + Round(delta *normals[0].X),
      pts[j][highI].Y + Round(delta *normals[0].Y)));

    //round off reflex angles (ie > 180 deg) unless it's almost flat (ie < 10deg angle) ...
    //cross product normals < 0 -> reflex angle; dot product normals == 1 -> no angle
    if ((normals[highI].X *normals[0].Y - normals[0].X *normals[highI].Y) *delta > 0 &&
    (normals[0].X *normals[highI].X + normals[0].Y *normals[highI].Y) < 0.985)
    {
      double a1 = std::atan2(normals[highI].Y, normals[highI].X);
      double a2 = std::atan2(normals[0].Y, normals[0].X);
      if (delta > 0 && a2 < a1) a2 = a2 + pi*2;
      else if (delta < 0 && a2 > a1) a2 = a2 - pi*2;
      Polygon arc = BuildArc(pts[j][highI], a1, a2, delta);
      Polygon::iterator it = pg.begin() +highI*2+1;
      pg.insert(it, arc.begin(), arc.end());
    }
    for (int i = highI; i > 0; --i)
      if ((normals[i-1].X*normals[i].Y - normals[i].X*normals[i-1].Y) *delta > 0 &&
      (normals[i].X*normals[i-1].X + normals[i].Y*normals[i-1].Y) < 0.985)
      {
        double a1 = std::atan2(normals[i-1].Y, normals[i-1].X);
        double a2 = std::atan2(normals[i].Y, normals[i].X);
        if (delta > 0 && a2 < a1) a2 = a2 + pi*2;
        else if (delta < 0 && a2 > a1) a2 = a2 - pi*2;
        Polygon arc = BuildArc(pts[j][i-1], a1, a2, delta);
        Polygon::iterator it = pg.begin() +(i-1)*2+1;
        pg.insert(it, arc.begin(), arc.end());
      }
    result.push_back(pg);
  }

  //finally, clean up untidy corners ...
  Clipper c4;
  c4.AddPolygons(result, ptSubject);
  if (delta > 0){
    if(!c4.Execute(ctUnion, result, pftNonZero, pftNonZero))
      result.clear();
  }
  else
  {
    IntRect r = c4.GetBounds();
    Polygon outer(4);
    outer[0] = IntPoint(r.left-10, r.bottom+10);
    outer[1] = IntPoint(r.right+10, r.bottom+10);
    outer[2] = IntPoint(r.right+10, r.top-10);
    outer[3] = IntPoint(r.left-10, r.top-10);
    c4.AddPolygon(outer, ptSubject);
    if (c4.Execute(ctUnion, result, pftNonZero, pftNonZero))
    {
      Polygons::iterator it = result.begin();
      result.erase(it);
    }
    else
      result.clear();
  }
  return result;
}
//------------------------------------------------------------------------------

} //namespace clipper


