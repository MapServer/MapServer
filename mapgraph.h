/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Declarations for basic undirected, wighted graph support.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 ****************************************************************************/

#ifndef MAPGRAPH_H
#define MAPGRAPH_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct graphNodeObj {
  int dest;
  double weight;
  struct graphNodeObj *next;
} graphNodeObj;

typedef struct {
  int numnodes;
  struct graphNodeObj **head;
} graphObj;

graphObj *msCreateGraph(int numnodes);
void msFreeGraph(graphObj *graph);
int msGraphAddEdge(graphObj *graph, int src, int dest, double weight);
void msPrintGraph(graphObj *graph);

int *msGraphGetLongestShortestPath(graphObj *graph, int src, int *path_size, double *path_dist);

#ifdef  __cplusplus
}
#endif

#endif /* MAPGRAPH_H */
