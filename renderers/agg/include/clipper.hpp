
/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  1.4i                                                            *
* Date      :  18 June 2010                                                    *
* Copyright :  Angus Johnson                                                   *
*                                                                              *
* This is an implementation of Bala Vatti's clipping algorithm outlined in:    *
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

#pragma once
#ifndef clipper_hpp
#define clipper_hpp

#include <vector>

namespace clipper {

typedef enum { ctIntersection, ctUnion, ctDifference, ctXor } TClipType;
typedef enum { ptSubject, ptClip } TPolyType;
typedef enum { esLeft, esRight } TEdgeSide;
typedef unsigned TIntersectProtects;

struct TDoublePoint { double X; double Y; };
TDoublePoint DoublePoint(double const &X, double const &Y);
typedef std::vector<TDoublePoint> TPolygon;
typedef std::vector< TPolygon > TPolyPolygon;

struct TEdge {
	double xbot;
	double ybot;
	double xtop;
	double ytop;
	double dx;
	double tmpX;
	TPolyType polyType;
	TEdgeSide side;
	int polyIdx;
	TEdge *next;
	TEdge *prev;
	TEdge *nextInLML;
	TEdge *nextInAEL;
	TEdge *prevInAEL;
	TEdge *nextInSEL;
	TEdge *prevInSEL;
	TDoublePoint savedBot;
};

struct TIntersectNode {
	TEdge *edge1;
	TEdge *edge2;
	TDoublePoint pt;
	TIntersectNode *next;
	TIntersectNode *prev;
};

struct TLocalMinima {
	double Y;
	TEdge *leftBound;
	TEdge *rightBound;
	TLocalMinima *nextLm;
};

struct TScanbeam {
	double Y;
	TScanbeam *nextSb;
};

struct TPolyPt {
	TDoublePoint pt;
	TPolyPt *next;
	TPolyPt *prev;
	bool isHole;
};

class ClipperBase
{
private:
	std::vector< TEdge * >  m_edges;
protected:
	TLocalMinima      *m_localMinimaList;
	TLocalMinima      *m_recycledLocMin;
	TLocalMinima      *m_recycledLocMinEnd;
	void DisposeLocalMinimaList();
	void InsertLocalMinima(TLocalMinima *newLm);
	TEdge* AddLML(TEdge *e);
	void PopLocalMinima();
	bool Reset();
public:
	ClipperBase();
	virtual ~ClipperBase();
	void AddPolygon(TPolygon &pg, TPolyType polyType);
	void AddPolyPolygon( TPolyPolygon &ppg, TPolyType polyType);
	void Clear();
};

class Clipper : public ClipperBase
{
private:
	std::vector < TPolyPt * >  m_PolyPts;
	TClipType             m_ClipType ;
	TScanbeam            *m_Scanbeam;
	TEdge                *m_ActiveEdges;
	TEdge                *m_SortedEdges;
	TIntersectNode       *m_IntersectNodes;
	bool                  m_ExecuteLocked;
	bool                  m_ForceAlternateOrientation;
	void DisposeScanbeamList();
	bool InitializeScanbeam();
	void InsertScanbeam( double const &Y);
	double PopScanbeam();
	void InsertLocalMinimaIntoAEL( double const &botY);
	void InsertEdgeIntoAEL(TEdge *edge);
	void AddHorzEdgeToSEL(TEdge *edge);
	void DeleteFromSEL(TEdge *e);
	void DeleteFromAEL(TEdge *e);
	void UpdateEdgeIntoAEL(TEdge *&e);
	void SwapWithNextInSEL(TEdge *edge);
	bool IsContributing(TEdge *edge, bool &reverseSides);
	bool IsTopHorz(TEdge *horzEdge, double const &XPos);
	void SwapPositionsInAEL(TEdge *edge1, TEdge *edge2);
	void DoMaxima(TEdge *e, double const &topY);
	void ProcessHorizontals();
	void ProcessHorizontal(TEdge *horzEdge);
	void AddLocalMaxPoly(TEdge *e1, TEdge *e2, TDoublePoint const &pt);
	void AddLocalMinPoly(TEdge *e1, TEdge *e2, TDoublePoint const &pt);
	void AppendPolygon(TEdge *e1, TEdge *e2);
	void DoEdge1(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt);
	void DoEdge2(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt);
	void DoBothEdges(TEdge *edge1, TEdge *edge2, TDoublePoint const &pt);
	void IntersectEdges(TEdge *e1, TEdge *e2,
		 TDoublePoint const &pt, TIntersectProtects protects);
	int AddPolyPt(int idx, TDoublePoint const &pt, bool ToFront);
	void DisposeAllPolyPts();
	void ProcessIntersections( double const &topY);
	void AddIntersectNode(TEdge *e1, TEdge *e2, TDoublePoint const &pt);
	void BuildIntersectList(double const &topY);
	void ProcessIntersectList();
	TEdge *BubbleSwap(TEdge *edge);
	void ProcessEdgesAtTopOfScanbeam( double const &topY);
	void BuildResult(TPolyPolygon &polypoly);
public:
	Clipper();
	~Clipper();
	bool Execute(TClipType clipType, TPolyPolygon &polypoly);
	bool ForceAlternateOrientation();
	void ForceAlternateOrientation(bool value);
};

} //clipper namespace
#endif //clipper_hpp

