/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Functions for basic undirected, weighted graph support.
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

#include "mapserver.h"
#include "mapgraph.h"

graphObj *msCreateGraph(int numnodes)
{
  int i;
  graphObj *graph=nullptr;

  if(numnodes <= 0) return nullptr;

  graph = (graphObj *) malloc(sizeof(graphObj));
  if(!graph) return nullptr;

  graph->head = (graphNodeObj **) malloc(numnodes * sizeof(graphNodeObj));
  if(!graph->head) {
    free(graph);
    return nullptr;
  }
  graph->numnodes = numnodes;

  for(i=0; i<numnodes; i++)
    graph->head[i] = nullptr;

  return graph;
}

void msFreeGraph(graphObj *graph)
{
  if(!graph) return;

  graphNodeObj *tmp=nullptr;
  
  for(int i=0; i<graph->numnodes; i++) {
    while(graph->head[i] != nullptr) {
      tmp = graph->head[i];
      graph->head[i] = graph->head[i]->next;
      free(tmp);
    }
  }

  free(graph->head);
  free(graph);
}

int msGraphAddEdge(graphObj *graph, int src, int dest, double weight)
{
  graphNodeObj *node=nullptr;

  if(!graph) return MS_FAILURE;

  // src -> dest
  node = (graphNodeObj *) malloc(sizeof(graphNodeObj));
  if(!node) return MS_FAILURE;

  node->dest = dest;
  node->weight = weight;
  node->next = graph->head[src];
  graph->head[src] = node;

  // dest -> src
  node = (graphNodeObj *) malloc(sizeof(graphNodeObj));
  if(!node) return MS_FAILURE;

  node->dest = src;
  node->weight = weight;
  node->next = graph->head[dest];
  graph->head[dest] = node;

  return MS_SUCCESS;
}

void msPrintGraph(graphObj *graph)
{
  int i;
  graphNodeObj *node=nullptr;

  if(!graph) return;

  for(i=0; i<graph->numnodes; i++) {
    node = graph->head[i];
    if(node != nullptr) {
      do {
        msDebug("%d -> %d (%.6f)\t", i, node->dest, node->weight);
        node = node->next;
      } while (node != nullptr);
      msDebug("\n");
    }
  }
}

/*
** Derived from an number web resources including:
**
**   https://www.geeksforgeeks.org/dijkstras-algorithm-for-adjacency-list-representation-greedy-algo-8/
**   https://youtube.com/watch?v=pSqmAO-m7Lk
**   https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
**
** Much of the bulk here is for min-heap (key,value) management.
*/

typedef struct {
  int idx;
  double dist;
} minHeapNodeObj;

typedef struct {
  int size;
  int capacity;
  int *pos;
  minHeapNodeObj **nodes;
} minHeapObj;

static minHeapNodeObj *newMinHeapNode(int idx, double dist)
{
  minHeapNodeObj *node = (minHeapNodeObj *) malloc(sizeof(minHeapNodeObj));
  if(!node) return nullptr;
  node->idx = idx;
  node->dist = dist;
  return node;
}

static void freeMinHeap(minHeapObj *minHeap)
{
  if(!minHeap) return;

  free(minHeap->pos);
  for(int i=0; i<minHeap->size; i++) {
    free(minHeap->nodes[i]);
  }
  free(minHeap->nodes);
  free(minHeap);
}

static minHeapObj *createMinHeap(int capacity)
{
  minHeapObj *minHeap = (minHeapObj *) malloc(sizeof(minHeapObj));
  if(!minHeap) return nullptr;

  minHeap->pos = (int *) malloc(capacity * sizeof(int));
  if(!minHeap->pos) {
    free(minHeap);
    return nullptr;
  }
  minHeap->size = 0;
  minHeap->capacity = capacity;
  minHeap->nodes = (minHeapNodeObj **) malloc(capacity * sizeof(minHeapNodeObj *));
  if(!minHeap->nodes) {
    free(minHeap->pos);
    free(minHeap);
    return nullptr;
  }
  return minHeap;
}

static void swapMinHeapNode(minHeapNodeObj **a, minHeapNodeObj **b)
{
  minHeapNodeObj *t = *a;
  *a = *b;
  *b = t;
}

static void minHeapify(minHeapObj *minHeap, int idx)
{
  int smallest, left, right;
  smallest = idx;
  left = 2*idx + 1;
  right = 2*idx + 2;
 
  if (left < minHeap->size && minHeap->nodes[left]->dist < minHeap->nodes[smallest]->dist)
    smallest = left;
 
  if (right < minHeap->size && minHeap->nodes[right]->dist < minHeap->nodes[smallest]->dist)
    smallest = right;
 
  if (smallest != idx) {
    minHeapNodeObj *smallestNode = minHeap->nodes[smallest];
    minHeapNodeObj *idxNode = minHeap->nodes[idx];
 
    minHeap->pos[smallestNode->idx] = idx; // swap positions
    minHeap->pos[idxNode->idx] = smallest;
 
    swapMinHeapNode(&minHeap->nodes[smallest], &minHeap->nodes[idx]); // swap nodes
    minHeapify(minHeap, smallest);
  }
}

static int isEmpty(minHeapObj *minHeap)
{
  return minHeap->size == 0;
}

static minHeapNodeObj *extractMin(minHeapObj *minHeap)
{
  if (isEmpty(minHeap)) return nullptr;
 
  // store root node
  minHeapNodeObj *root = minHeap->nodes[0];
 
  // replace root node with last node
  minHeapNodeObj *lastNode = minHeap->nodes[minHeap->size - 1];
  minHeap->nodes[0] = lastNode;
 
  // update position of last node
  minHeap->pos[root->idx] = minHeap->size - 1;
  minHeap->pos[lastNode->idx] = 0;
 
  // Reduce heap size and heapify root
  --minHeap->size;
  minHeapify(minHeap, 0);
 
  return root;
}

static void decreaseKey(minHeapObj *minHeap, int idx, int dist)
{
  // get the index of idx in min heap nodes
  int i = minHeap->pos[idx];
 
  // get the node and update its dist value
  minHeap->nodes[i]->dist = dist;
 
  // travel up while the complete tree is not hepified (this is a O(Logn) loop)
  while (i && minHeap->nodes[i]->dist < minHeap->nodes[(i - 1) / 2]->dist) {
    // swap this node with its parent
    minHeap->pos[minHeap->nodes[i]->idx] = (i-1)/2;
    minHeap->pos[minHeap->nodes[(i-1)/2]->idx] = i;
    swapMinHeapNode(&minHeap->nodes[i], &minHeap->nodes[(i - 1) / 2]);
 
    // move to parent index
    i = (i - 1) / 2;
  }
}

static int isInMinHeap(minHeapObj *minHeap, int idx)
{
  if (minHeap->pos[idx] < minHeap->size) return MS_TRUE;
  return MS_FALSE;
}

typedef struct {
  double *dist;
  int *prev;
} dijkstraOutputObj;

static dijkstraOutputObj *dijkstra(graphObj *graph, int src)
{
  int n = graph->numnodes;

  minHeapObj *minHeap = createMinHeap(n); // priority queue implemented as a min heap structure
  if(!minHeap) return nullptr;

  dijkstraOutputObj *output = nullptr;
  output = (dijkstraOutputObj *) malloc(sizeof(dijkstraOutputObj));
  output->dist = (double *) malloc(n * sizeof(double));
  output->prev = (int *) malloc(n * sizeof(int));
  if(!output->dist || !output->prev) {
    msFree(output->dist);
    msFree(output->prev);
    free(output);
    freeMinHeap(minHeap);
    return nullptr;
  }

  // initialize 
  for (int i=0; i<n; i++) {
    output->dist[i] = HUGE_VAL;
    output->prev[i] = -1;
    minHeap->nodes[i] = newMinHeapNode(i, output->dist[i]);
    minHeap->pos[i] = i;
  }
 
  // make dist value of src vertex as 0 so that it is extracted first
  minHeap->nodes[src] = newMinHeapNode(src, output->dist[src]);
  minHeap->pos[src] = src;
  output->dist[src] = 0;
  decreaseKey(minHeap, src, output->dist[src]);
 
  // initially size of min heap is equal to graph->numnodes (n)
  minHeap->size = n;
 
  // In the following loop, minHeap contains all nodes
  // whose shortest distance is not yet finalized.
  while (!isEmpty(minHeap)) {

    // extract the vertex with minimum distance value and store the node index
    minHeapNodeObj *minHeapNode = extractMin(minHeap);
    int u = minHeapNode->idx;
 
    // traverse through all adjacent nodes of u and update their distance values
    graphNodeObj *node = graph->head[u];
    while (node != nullptr) {
      int v = node->dest;
 
      // if shortest distance to v is not finalized yet, and distance to v
      // through u is less than its previously calculated distance
      if (isInMinHeap(minHeap, v) && output->dist[u] != HUGE_VAL && node->weight + output->dist[u] < output->dist[v]) {
        output->dist[v] = output->dist[u] + node->weight;
        output->prev[v] = u;
        decreaseKey(minHeap, v, output->dist[v]); 
      }
      node = node->next;
    }
  }
 
  freeMinHeap(minHeap);

  return output;
}

static void reverse(int *arr, int n)
{
  for(int i=0, j=n-1; i<j; i++, j--) {
    int tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  }
}

int *msGraphGetLongestShortestPath(graphObj *graph, int src, int *path_size, double *path_dist)
{
  int *path=nullptr;
  int i, j, dest;

  if(!graph || src < 0 || src > graph->numnodes) return nullptr;

  path = (int *) malloc((graph->numnodes)*sizeof(int)); // worst case is path traverses all nodes
  if(!path) return nullptr;

  dijkstraOutputObj *output = dijkstra(graph, src);
  if(!output) {
    free(path);
    return nullptr; // algorithm failed for some reason
  }

  // get longest shortest distance from src to another node (our dest)
  *path_dist = -1;
  for(i=0; i<graph->numnodes; i++) {
    if(output->dist[i] != HUGE_VAL && *path_dist < output->dist[i]) {
      *path_dist = output->dist[i];
      dest = i;
    }
  }

  // construct the path from src to dest
  for(i=dest,j=0; i!=-1; i=output->prev[i],j++) path[j] = i;
  reverse(path, j);
  *path_size = j;

  // clean up dijkstra output
  free(output->dist);
  free(output->prev);
  free(output);

  return path;
}
