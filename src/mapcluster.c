/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of the cluster layer data provider (RFC-69).
 * Author:   Tamas Szekeres (szekerest@gmail.com).
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

#define _CRT_SECURE_NO_WARNINGS 1

/* $Id$ */
#include <assert.h>
#include "mapserver.h"

#ifdef USE_CLUSTER_PLUGIN
#define USE_CLUSTER_EXTERNAL
#endif

/* custom attributes provided by this layer data source */
#define MSCLUSTER_NUMITEMS 3
#define MSCLUSTER_FEATURECOUNT "Cluster_FeatureCount"
#define MSCLUSTER_FEATURECOUNTINDEX -100
#define MSCLUSTER_GROUP "Cluster_Group"
#define MSCLUSTER_GROUPINDEX -101
#define MSCLUSTER_BASEFID "Cluster_BaseFID"
#define MSCLUSTER_BASEFIDINDEX -102

typedef struct cluster_tree_node clusterTreeNode;
typedef struct cluster_info clusterInfo;
typedef struct cluster_layer_info msClusterLayerInfo;

/* forward declarations */
void msClusterLayerCopyVirtualTable(layerVTableObj *vtable);
static void clusterTreeNodeDestroy(msClusterLayerInfo *layerinfo,
                                   clusterTreeNode *node);

/* cluster compare func */
typedef int (*clusterCompareRegionFunc)(clusterInfo *current,
                                        clusterInfo *other);

/* quadtree constants */
#define SPLITRATIO 0.55
#define TREE_MAX_DEPTH 10

/* cluster algorithm */
#define MSCLUSTER_ALGORITHM_FULL 0
#define MSCLUSTER_ALGORITHM_SIMPLE 1

/* cluster data */
struct cluster_info {
  double x;       /* x position of the current point */
  double y;       /* y position of the current point */
  double avgx;    /* average x positions of this cluster */
  double avgy;    /* average y positions of this cluster */
  double varx;    /* variance of the x positions of this cluster */
  double vary;    /* variance of the y positions of this cluster */
  shapeObj shape; /* current shape */
  rectObj bounds; /* clustering region */
  /* number of the neighbouring shapes */
  int numsiblings;
  /* diagnostics */
  int numcollected;
  int numremoved;
  int index;
  clusterTreeNode *node;
  /* collection of the siblings */
  clusterInfo *siblings;
  /* next shape in the linked list */
  clusterInfo *next;
  /* current group */
  char *group;
  int filter;
};

/* quadtree node */
struct cluster_tree_node {
  /* area covered by this node */
  rectObj rect;
  /* linked list of the shapes stored at this node. */
  int numshapes;
  int index;
  int position;
  clusterInfo *shapes;
  /* quad tree subnodes */
  clusterTreeNode *subnode[4];
};

/* layeinfo */
struct cluster_layer_info {
  /* array of features (finalized clusters) */
  clusterInfo *finalized;
  clusterInfo *finalizedSiblings;
  clusterInfo *filtered;
  int numFeatures;
  int numFinalized;
  int numFinalizedSiblings;
  int numFiltered;
  /* variables for collecting the best cluster and iterating with NextShape */
  clusterInfo *current;
  /* check whether all shapes should be returned behind a cluster */
  int get_all_shapes;
  /* check whether the location of the shapes should be preserved (no averaging)
   */
  int keep_locations;
  /* the maxdistance and the buffer parameters are specified in map units (scale
   * independent clustering) */
  int use_map_units;
  double rank;
  /* root node of the quad tree */
  clusterTreeNode *root;
  int numNodes;
  clusterTreeNode *finalizedNodes;
  int numFinalizedNodes;
  /* map extent used for building cluster data */
  rectObj searchRect;
  /* source layer parameters */
  layerObj srcLayer;
  /* distance comparator function */
  clusterCompareRegionFunc fnCompare;
  /* diagnostics */
  int depth;
  /* processing algorithm */
  int algorithm;
};

extern int yyparse(parseObj *p);

/* evaluate the filter expression */
int msClusterEvaluateFilter(expressionObj *expression, shapeObj *shape) {
  if (expression->type == MS_EXPRESSION) {
    int status;
    parseObj p;

    p.shape = shape;
    p.expr = expression;
    p.expr->curtoken = p.expr->tokens; /* reset */
    p.type = MS_PARSE_TYPE_BOOLEAN;

    status = yyparse(&p);

    if (status != 0) {
      msSetError(MS_PARSEERR, "Failed to parse expression: %s",
                 "msClusterEvaluateFilter", expression->string);
      return 0;
    }

    return p.result.intval;
  }

  return 0;
}

/* get the group text when creating the clusters */
char *msClusterGetGroupText(expressionObj *expression, shapeObj *shape) {
  char *tmpstr = NULL;

  if (expression->string) {
    switch (expression->type) {
    case (MS_STRING):
      tmpstr = msStrdup(expression->string);
      break;
    case (MS_EXPRESSION): {
      int status;
      parseObj p;

      p.shape = shape;
      p.expr = expression;
      p.expr->curtoken = p.expr->tokens; /* reset */
      p.type = MS_PARSE_TYPE_STRING;

      status = yyparse(&p);

      if (status != 0) {
        msSetError(MS_PARSEERR, "Failed to process text expression: %s",
                   "msClusterGetGroupText", expression->string);
        return NULL;
      }

      tmpstr = p.result.strval;
      break;
    }
    default:
      break;
    }
  }

  return (tmpstr);
}

int CompareEllipseRegion(clusterInfo *current, clusterInfo *other) {
  if (current->group && other->group && !EQUAL(current->group, other->group))
    return MS_FALSE;

  if ((other->x - current->x) * (other->x - current->x) /
              ((current->bounds.maxx - current->x) *
               (current->bounds.maxx - current->x)) +
          (other->y - current->y) * (other->y - current->y) /
              ((current->bounds.maxy - current->y) *
               (current->bounds.maxy - current->y)) >
      1)
    return MS_FALSE;

  return MS_TRUE;
}

int CompareRectangleRegion(clusterInfo *current, clusterInfo *other) {
  if (current->group && other->group && !EQUAL(current->group, other->group))
    return MS_FALSE;

  if (other->x < current->bounds.minx)
    return MS_FALSE;
  if (other->x > current->bounds.maxx)
    return MS_FALSE;
  if (other->y < current->bounds.miny)
    return MS_FALSE;
  if (other->y > current->bounds.maxy)
    return MS_FALSE;

  return MS_TRUE;
}

static void treeSplitBounds(rectObj *in, rectObj *out1, rectObj *out2) {
  double range;

  /* -------------------------------------------------------------------- */
  /*      The output bounds will be very similar to the input bounds,     */
  /*      so just copy over to start.                                     */
  /* -------------------------------------------------------------------- */
  memcpy(out1, in, sizeof(rectObj));
  memcpy(out2, in, sizeof(rectObj));

  /* -------------------------------------------------------------------- */
  /*      Split in X direction.                                           */
  /* -------------------------------------------------------------------- */
  if ((in->maxx - in->minx) > (in->maxy - in->miny)) {
    range = in->maxx - in->minx;

    out1->maxx = in->minx + range * SPLITRATIO;
    out2->minx = in->maxx - range * SPLITRATIO;
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise split in Y direction.                                 */
  /* -------------------------------------------------------------------- */
  else {
    range = in->maxy - in->miny;

    out1->maxy = in->miny + range * SPLITRATIO;
    out2->miny = in->maxy - range * SPLITRATIO;
  }
}

/* alloc memory for a new tentative cluster */
static clusterInfo *clusterInfoCreate(msClusterLayerInfo *layerinfo) {
  clusterInfo *feature = (clusterInfo *)msSmallMalloc(sizeof(clusterInfo));
  msInitShape(&feature->shape);
  feature->numsiblings = 0;
  feature->numcollected = 0;
  feature->numremoved = 0;
  feature->next = NULL;
  feature->group = NULL;
  feature->node = NULL;
  feature->siblings = NULL;
  feature->index = layerinfo->numFeatures;
  feature->filter = -1; /* not yet calculated */
  ++layerinfo->numFeatures;
  return feature;
}

/* destroy memory of the cluster list */
static void clusterInfoDestroyList(msClusterLayerInfo *layerinfo,
                                   clusterInfo *feature) {
  clusterInfo *s = feature;
  clusterInfo *next;
  /* destroy the shapes added to this node */
  while (s) {
    next = s->next;
    if (s->siblings) {
      clusterInfoDestroyList(layerinfo, s->siblings);
    }
    msFreeShape(&s->shape);
    msFree(s->group);
    msFree(s);
    --layerinfo->numFeatures;
    s = next;
  }
}

/* alloc memory for a new treenode */
static clusterTreeNode *clusterTreeNodeCreate(msClusterLayerInfo *layerinfo,
                                              rectObj rect) {
  clusterTreeNode *node =
      (clusterTreeNode *)msSmallMalloc(sizeof(clusterTreeNode));
  node->rect = rect;
  node->numshapes = 0;
  node->shapes = NULL;
  node->subnode[0] = node->subnode[1] = node->subnode[2] = node->subnode[3] =
      NULL;
  node->index = layerinfo->numNodes;
  node->position = 0;
  ++layerinfo->numNodes;
  return node;
}

/* traverse the quadtree and destroy all sub elements */
static void clusterTreeNodeDestroy(msClusterLayerInfo *layerinfo,
                                   clusterTreeNode *node) {
  int i;
  /* destroy the shapes added to this node */
  clusterInfoDestroyList(layerinfo, node->shapes);

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      clusterTreeNodeDestroy(layerinfo, node->subnode[i]);
  }

  msFree(node);
  --layerinfo->numNodes;
}

/* destroy memory of the cluster finalized list (without recursion) */
static void clusterTreeNodeDestroyList(msClusterLayerInfo *layerinfo,
                                       clusterTreeNode *node) {
  clusterTreeNode *n = node;
  clusterTreeNode *next;
  /* destroy the list of nodes */
  while (n) {
    next = n->subnode[0];
    n->subnode[0] = NULL;
    clusterTreeNodeDestroy(layerinfo, n);
    --layerinfo->numFinalizedNodes;
    n = next;
  }
}

void clusterDestroyData(msClusterLayerInfo *layerinfo) {
  if (layerinfo->finalized) {
    clusterInfoDestroyList(layerinfo, layerinfo->finalized);
    layerinfo->finalized = NULL;
  }
  layerinfo->numFinalized = 0;

  if (layerinfo->finalizedSiblings) {
    clusterInfoDestroyList(layerinfo, layerinfo->finalizedSiblings);
    layerinfo->finalizedSiblings = NULL;
  }
  layerinfo->numFinalizedSiblings = 0;

  if (layerinfo->filtered) {
    clusterInfoDestroyList(layerinfo, layerinfo->filtered);
    layerinfo->filtered = NULL;
  }
  layerinfo->numFiltered = 0;

  if (layerinfo->finalizedNodes) {
    clusterTreeNodeDestroyList(layerinfo, layerinfo->finalizedNodes);
    layerinfo->finalizedNodes = NULL;
  }

  layerinfo->numFinalizedNodes = 0;

  if (layerinfo->root) {
    clusterTreeNodeDestroy(layerinfo, layerinfo->root);
    layerinfo->root = NULL;
  }

  layerinfo->numNodes = 0;
}

/* traverse the quadtree to find the neighbouring shapes and update some data
on the related shapes (when adding a new feature)*/
static void findRelatedShapes(msClusterLayerInfo *layerinfo,
                              clusterTreeNode *node, clusterInfo *current) {
  int i;
  clusterInfo *s;

  /* -------------------------------------------------------------------- */
  /*      Does this node overlap the area of interest at all?  If not,    */
  /*      return without adding to the list at all.                       */
  /* -------------------------------------------------------------------- */
  if (!msRectOverlap(&node->rect, &current->bounds))
    return;

  /* Modify the feature count of the related shapes */
  s = node->shapes;
  while (s) {
    if (layerinfo->fnCompare(current, s)) {
      ++current->numsiblings;
      /* calculating the average positions */
      current->avgx = (current->avgx * current->numsiblings + s->x) /
                      (current->numsiblings + 1);
      current->avgy = (current->avgy * current->numsiblings + s->y) /
                      (current->numsiblings + 1);
      /* calculating the variance */
      current->varx =
          current->varx * current->numsiblings / (current->numsiblings + 1) +
          (s->x - current->avgx) * (s->x - current->avgx) /
              (current->numsiblings + 1);
      current->vary =
          current->vary * current->numsiblings / (current->numsiblings + 1) +
          (s->y - current->avgy) * (s->y - current->avgy) /
              (current->numsiblings + 1);

      if (layerinfo->fnCompare(s, current)) {
        /* this feature falls into the region of the other as well */
        ++s->numsiblings;
        /* calculating the average positions */
        s->avgx =
            (s->avgx * s->numsiblings + current->x) / (s->numsiblings + 1);
        s->avgy =
            (s->avgy * s->numsiblings + current->y) / (s->numsiblings + 1);
        /* calculating the variance */
        s->varx = s->varx * s->numsiblings / (s->numsiblings + 1) +
                  (current->x - s->avgx) * (current->x - s->avgx) /
                      (s->numsiblings + 1);
        s->vary = s->vary * s->numsiblings / (s->numsiblings + 1) +
                  (current->y - s->avgy) * (current->y - s->avgy) /
                      (s->numsiblings + 1);
      }
    }
    s = s->next;
  }

  if (node->subnode[0] == NULL)
    return;

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      findRelatedShapes(layerinfo, node->subnode[i], current);
  }
}

/* traverse the quadtree to find the neighbouring clusters and update data on
 * the cluster*/
static void findRelatedShapes2(msClusterLayerInfo *layerinfo,
                               clusterTreeNode *node, clusterInfo *current) {
  int i;
  clusterInfo *s;

  /* -------------------------------------------------------------------- */
  /*      Does this node overlap the area of interest at all?  If not,    */
  /*      return without adding to the list at all.                       */
  /* -------------------------------------------------------------------- */
  if (!msRectOverlap(&node->rect, &current->bounds))
    return;

  /* Modify the feature count of the related shapes */
  s = node->shapes;
  while (s) {
    if (layerinfo->fnCompare(s, current)) {
      if (layerinfo->rank > 0) {
        double r = (current->x - s->x) * (current->x - s->x) +
                   (current->y - s->y) * (current->y - s->y);
        if (r < layerinfo->rank) {
          layerinfo->current = s;
          layerinfo->rank = r;
        }
      } else {
        /* no rank was specified, return immediately */
        layerinfo->current = s;
        return;
      }
    }
    s = s->next;
  }

  if (node->subnode[0] == NULL)
    return;

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      findRelatedShapes2(layerinfo, node->subnode[i], current);
  }
}

/* traverse the quadtree to find the neighbouring shapes and update some data
on the related shapes (when removing a feature) */
static void findRelatedShapesRemove(msClusterLayerInfo *layerinfo,
                                    clusterTreeNode *node,
                                    clusterInfo *current) {
  int i;
  clusterInfo *s;

  /* -------------------------------------------------------------------- */
  /*      Does this node overlap the area of interest at all?  If not,    */
  /*      return without adding to the list at all.                       */
  /* -------------------------------------------------------------------- */
  if (!msRectOverlap(&node->rect, &current->bounds))
    return;

  /* Modify the feature count of the related shapes */
  s = node->shapes;
  while (s) {
    if (layerinfo->fnCompare(current, s)) {
      if (s->numsiblings > 0) {
        /* calculating the average positions */
        s->avgx =
            (s->avgx * (s->numsiblings + 1) - current->x) / s->numsiblings;
        s->avgy =
            (s->avgy * (s->numsiblings + 1) - current->y) / s->numsiblings;
        /* calculating the variance */
        s->varx = (s->varx - (current->x - s->avgx) * (current->x - s->avgx) /
                                 s->numsiblings) *
                  (s->numsiblings + 1) / s->numsiblings;
        s->vary = (s->vary - (current->y - s->avgy) * (current->y - s->avgy) /
                                 s->numsiblings) *
                  (s->numsiblings + 1) / s->numsiblings;
        --s->numsiblings;
        ++s->numremoved;
      }
    }
    s = s->next;
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      findRelatedShapesRemove(layerinfo, node->subnode[i], current);
  }
}

/* setting the aggregated attributes */
static void InitShapeAttributes(layerObj *layer, clusterInfo *base) {
  int i;
  int *itemindexes = layer->iteminfo;

  for (i = 0; i < layer->numitems; i++) {
    if (base->shape.numvalues <= i)
      break;

    if (itemindexes[i] == MSCLUSTER_FEATURECOUNTINDEX) {
      if (base->shape.values[i])
        msFree(base->shape.values[i]);

      base->shape.values[i] = msIntToString(base->numsiblings + 1);
    } else if (itemindexes[i] == MSCLUSTER_GROUPINDEX) {
      if (base->shape.values[i])
        msFree(base->shape.values[i]);

      if (base->group)
        base->shape.values[i] = msStrdup(base->group);
      else
        base->shape.values[i] = msStrdup("");
    } else if (itemindexes[i] == MSCLUSTER_BASEFIDINDEX) {
      if (base->shape.values[i])
        msFree(base->shape.values[i]);

      base->shape.values[i] = msIntToString(base->shape.index);
    } else if (EQUALN(layer->items[i], "Count:", 6)) {
      if (base->shape.values[i])
        msFree(base->shape.values[i]);

      base->shape.values[i] = msStrdup("1"); /* initial count */
    }
  }
}

/* update the shape attributes (aggregate) */
static void UpdateShapeAttributes(layerObj *layer, clusterInfo *base,
                                  clusterInfo *current) {
  int i;
  int *itemindexes = layer->iteminfo;

  for (i = 0; i < layer->numitems; i++) {
    if (base->shape.numvalues <= i)
      break;

    if (itemindexes[i] == MSCLUSTER_FEATURECOUNTINDEX ||
        itemindexes[i] == MSCLUSTER_GROUPINDEX)
      continue;

    if (current->shape.numvalues <= i)
      break;

    /* setting the base feature index for each cluster member */
    if (itemindexes[i] == MSCLUSTER_BASEFIDINDEX) {
      msFree(current->shape.values[i]);
      current->shape.values[i] = msIntToString(base->shape.index);
    }

    if (current->shape.values[i]) {
      if (EQUALN(layer->items[i], "Min:", 4)) {
        if (strcasecmp(base->shape.values[i], current->shape.values[i]) > 0) {
          msFree(base->shape.values[i]);
          base->shape.values[i] = msStrdup(current->shape.values[i]);
        }
      } else if (EQUALN(layer->items[i], "Max:", 4)) {
        if (strcasecmp(base->shape.values[i], current->shape.values[i]) < 0) {
          msFree(base->shape.values[i]);
          base->shape.values[i] = msStrdup(current->shape.values[i]);
        }
      } else if (EQUALN(layer->items[i], "Sum:", 4)) {
        double sum =
            atof(base->shape.values[i]) + atof(current->shape.values[i]);
        msFree(base->shape.values[i]);
        base->shape.values[i] = msDoubleToString(sum, MS_FALSE);
      } else if (EQUALN(layer->items[i], "Count:", 6)) {
        int count = atoi(base->shape.values[i]) + 1;
        msFree(base->shape.values[i]);
        base->shape.values[i] = msIntToString(count);
      }
    }
  }
}

static int BuildFeatureAttributes(layerObj *layer,
                                  msClusterLayerInfo *layerinfo,
                                  shapeObj *shape) {
  char **values;
  int i;
  int *itemindexes = layer->iteminfo;

  if (layer->numitems == layerinfo->srcLayer.numitems)
    return MS_SUCCESS; /* we don't have custom attributes, no need to
                          reconstruct the array */

  values = msSmallMalloc(sizeof(char *) * (layer->numitems));

  for (i = 0; i < layer->numitems; i++) {
    if (itemindexes[i] == MSCLUSTER_FEATURECOUNTINDEX) {
      values[i] = NULL; /* not yet assigned */
    } else if (itemindexes[i] == MSCLUSTER_GROUPINDEX) {
      values[i] = NULL; /* not yet assigned */
    } else if (itemindexes[i] == MSCLUSTER_BASEFIDINDEX) {
      values[i] = NULL; /* not yet assigned */
    } else if (shape->values[itemindexes[i]])
      values[i] = msStrdup(shape->values[itemindexes[i]]);
    else
      values[i] = msStrdup("");
  }

  if (shape->values)
    msFreeCharArray(shape->values, shape->numvalues);

  shape->values = values;
  shape->numvalues = layer->numitems;

  return MS_SUCCESS;
}

/* traverse the quadtree to find the best renking cluster */
static void findBestCluster(layerObj *layer, msClusterLayerInfo *layerinfo,
                            clusterTreeNode *node) {
  int i;
  double rank;
  clusterInfo *s = node->shapes;
  while (s) {
    if (s->filter < 0 && layer->cluster.filter.string != NULL) {
      InitShapeAttributes(layer, s);
      s->filter = msClusterEvaluateFilter(&layer->cluster.filter, &s->shape);
    }

    if (s->numsiblings == 0 || s->filter == 0) {
      /* individual or filtered shapes must be removed for sure */
      layerinfo->current = s;
      return;
    }

    /* calculating the rank */
    rank = (s->x - s->avgx) * (s->x - s->avgx) +
           (s->y - s->avgy) * (s->y - s->avgy) /*+ s->varx + s->vary*/ +
           (double)1 / (1 + s->numsiblings);

    if (rank < layerinfo->rank) {
      layerinfo->current = s;
      layerinfo->rank = rank;
    }
    s = s->next;
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      findBestCluster(layer, layerinfo, node->subnode[i]);
  }
}

/* adding the shape based on the shape bounds (point) */
static int treeNodeAddShape(msClusterLayerInfo *layerinfo,
                            clusterTreeNode *node, clusterInfo *shape,
                            int depth) {
  int i;

  /* -------------------------------------------------------------------- */
  /*      If there are subnodes, then consider whether this object        */
  /*      will fit in them.                                               */
  /* -------------------------------------------------------------------- */
  if (depth > 1 && node->subnode[0] != NULL) {
    for (i = 0; i < 4; i++) {
      if (msRectContained(&shape->shape.bounds, &node->subnode[i]->rect)) {
        return treeNodeAddShape(layerinfo, node->subnode[i], shape, depth - 1);
      }
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Otherwise, consider creating four subnodes if could fit into    */
  /*      them, and adding to the appropriate subnode.                    */
  /* -------------------------------------------------------------------- */
  else if (depth > 1 && node->subnode[0] == NULL) {
    rectObj half1, half2, quad1, quad2, quad3, quad4;
    int subnode = -1;

    treeSplitBounds(&node->rect, &half1, &half2);
    treeSplitBounds(&half1, &quad1, &quad2);
    treeSplitBounds(&half2, &quad3, &quad4);

    if (msRectContained(&shape->shape.bounds, &quad1))
      subnode = 0;
    else if (msRectContained(&shape->shape.bounds, &quad2))
      subnode = 1;
    else if (msRectContained(&shape->shape.bounds, &quad3))
      subnode = 2;
    else if (msRectContained(&shape->shape.bounds, &quad4))
      subnode = 3;

    if (subnode >= 0) {
      if ((node->subnode[0] = clusterTreeNodeCreate(layerinfo, quad1)) == NULL)
        return MS_FAILURE;
      node->subnode[0]->position = node->position * 4;

      if ((node->subnode[1] = clusterTreeNodeCreate(layerinfo, quad2)) == NULL)
        return MS_FAILURE;
      node->subnode[1]->position = node->position * 4 + 1;

      if ((node->subnode[2] = clusterTreeNodeCreate(layerinfo, quad3)) == NULL)
        return MS_FAILURE;
      node->subnode[2]->position = node->position * 4 + 2;

      if ((node->subnode[3] = clusterTreeNodeCreate(layerinfo, quad4)) == NULL)
        return MS_FAILURE;
      node->subnode[3]->position = node->position * 4 + 3;

      /* add to subnode */
      return treeNodeAddShape(layerinfo, node->subnode[subnode], shape,
                              depth - 1);
    }
  }

  /* found the right place, add this shape to the node */
  node->numshapes++;
  shape->next = node->shapes;
  node->shapes = shape;
  shape->node = node;

  return MS_SUCCESS;
}

/* collecting the cluster shapes, returns true if this subnode must be removed
 */
static int collectClusterShapes(msClusterLayerInfo *layerinfo,
                                clusterTreeNode *node, clusterInfo *current) {
  int i;
  clusterInfo *prev = NULL;
  clusterInfo *s = node->shapes;

  if (!msRectOverlap(&node->rect, &current->bounds))
    return (!node->shapes && !node->subnode[0] && !node->subnode[1] &&
            !node->subnode[2] && !node->subnode[3]);

  /* removing the shapes from this node if overlap with the cluster */
  while (s) {
    if (s == current || layerinfo->fnCompare(current, s)) {
      if (s != current && current->filter == 0) {
        /* skip siblings of the filtered shapes */
        prev = s;
        s = prev->next;
        continue;
      }

      /* removing from the list */
      if (!prev)
        node->shapes = s->next;
      else
        prev->next = s->next;

      ++current->numcollected;

      /* adding the shape to the finalization list */
      if (s == current) {
        if (s->filter) {
          s->next = layerinfo->finalized;
          layerinfo->finalized = s;
          ++layerinfo->numFinalized;
        } else {
          /* this shape is filtered */
          s->next = layerinfo->filtered;
          layerinfo->filtered = s;
          ++layerinfo->numFiltered;
        }
      } else {
        s->next = layerinfo->finalizedSiblings;
        layerinfo->finalizedSiblings = s;
        ++layerinfo->numFinalizedSiblings;
      }

      if (!prev)
        s = node->shapes;
      else
        s = prev->next;
    } else {
      prev = s;
      s = prev->next;
    }
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i] &&
        collectClusterShapes(layerinfo, node->subnode[i], current)) {
      /* placing this empty node to the finalization queue */
      node->subnode[i]->subnode[0] = layerinfo->finalizedNodes;
      layerinfo->finalizedNodes = node->subnode[i];
      node->subnode[i] = NULL;
      ++layerinfo->numFinalizedNodes;
    }
  }

  /* returns true is this subnode must be removed */
  return (!node->shapes && !node->subnode[0] && !node->subnode[1] &&
          !node->subnode[2] && !node->subnode[3]);
}

/* collecting the cluster shapes, returns true if this subnode must be removed
 */
static int collectClusterShapes2(layerObj *layer, msClusterLayerInfo *layerinfo,
                                 clusterTreeNode *node) {
  int i;
  clusterInfo *current = NULL;
  clusterInfo *s;

  while (node->shapes) {
    s = node->shapes;
    /* removing from the list */
    node->shapes = s->next;

    InitShapeAttributes(layer, s);

    if (s->filter) {
      s->next = layerinfo->finalized;
      layerinfo->finalized = s;
      ++layerinfo->numFinalized;
    } else {
      /* this shape is filtered */
      s->next = layerinfo->filtered;
      layerinfo->filtered = s;
      ++layerinfo->numFiltered;
    }

    /* update the parameters of the related shapes if any */
    if (s->siblings) {
      current = s->siblings;
      while (current) {
        UpdateShapeAttributes(layer, s, current);

        /* setting the average position to the cluster position */
        current->avgx = s->x;
        current->avgy = s->y;

        if (current->next == NULL) {
          if (layerinfo->get_all_shapes == MS_TRUE) {
            /* insert the siblings into the finalization list */
            current->next = layerinfo->finalized;
            layerinfo->finalized = s->siblings;
            s->siblings = NULL;
          }
          break;
        }

        current = current->next;
      }
    }
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i] &&
        collectClusterShapes2(layer, layerinfo, node->subnode[i])) {
      /* placing this empty node to the finalization queue */
      node->subnode[i]->subnode[0] = layerinfo->finalizedNodes;
      layerinfo->finalizedNodes = node->subnode[i];
      node->subnode[i] = NULL;
      ++layerinfo->numFinalizedNodes;
    }
  }

  /* returns true is this subnode must be removed */
  return (!node->shapes && !node->subnode[0] && !node->subnode[1] &&
          !node->subnode[2] && !node->subnode[3]);
}

int selectClusterShape(layerObj *layer, long shapeindex) {
  int i;
  clusterInfo *current;
  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer not open: %s", "selectClusterShape()",
               layer->name);
    return MS_FAILURE;
  }

  i = 0;
  current = layerinfo->finalized;
  while (current && i < shapeindex) {
    ++i;
    current = current->next;
  }
  assert(current);

  current->next = current->siblings;
  layerinfo->current = current;

  if (layerinfo->keep_locations == MS_FALSE) {
    current->shape.line[0].point[0].x = current->shape.bounds.minx =
        current->shape.bounds.maxx = current->avgx;
    current->shape.line[0].point[0].y = current->shape.bounds.miny =
        current->shape.bounds.maxy = current->avgy;
  }

  return MS_SUCCESS;
}

/* update the parameters from the related shapes */
#ifdef ms_notused
static void UpdateClusterParameters(msClusterLayerInfo *layerinfo,
                                    clusterTreeNode *node, clusterInfo *shape) {
  int i;
  clusterInfo *s = node->shapes;

  while (s) {
    if (layerinfo->fnCompare(shape, s)) {
      shape->avgx += s->x;
      shape->avgy += s->y;
      ++shape->numsiblings;
    }
    s = s->next;
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i])
      UpdateClusterParameters(layerinfo, node->subnode[i], shape);
  }
}

/* check for the validity of the clusters added to the tree (for debug purposes)
 */
static int ValidateTree(msClusterLayerInfo *layerinfo, clusterTreeNode *node) {
  int i;
  int isValid = MS_TRUE;

  clusterInfo *s = node->shapes;
  while (s) {
    double avgx = s->avgx;
    double avgy = s->avgy;
    int numsiblings = s->numsiblings;

    s->avgx = 0;
    s->avgy = 0;
    s->numsiblings = 0;

    UpdateClusterParameters(layerinfo, layerinfo->root, s);

    if (numsiblings + 1 != s->numsiblings)
      isValid = MS_FALSE;
    else if ((avgx * s->numsiblings - s->avgx) / s->avgx > 0.000001)
      isValid = MS_FALSE;
    else if ((avgy * s->numsiblings - s->avgy) / s->avgy > 0.000001)
      isValid = MS_FALSE;

    s->avgx = avgx;
    s->avgy = avgy;
    s->numsiblings = numsiblings;

    if (isValid == MS_FALSE)
      return MS_FALSE;

    s = s->next;
  }

  /* Recurse to subnodes if they exist */
  for (i = 0; i < 4; i++) {
    if (node->subnode[i] &&
        ValidateTree(layerinfo, node->subnode[i]) == MS_FALSE)
      return MS_FALSE;
  }

  /* returns true if this node contains only valid clusters */
  return MS_TRUE;
}
#endif

/* rebuild the clusters according to the current extent */
int RebuildClusters(layerObj *layer, int isQuery) {
  mapObj *map;
  layerObj *srcLayer;
  double distance, maxDistanceX, maxDistanceY, cellSizeX, cellSizeY;
  rectObj searchrect;
  int status;
  clusterInfo *current;
  int depth;
  const char *pszProcessing;
#ifdef USE_CLUSTER_EXTERNAL
  int layerIndex;
#endif
  reprojectionObj *reprojector = NULL;

  msClusterLayerInfo *layerinfo = layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer is not open: %s", "RebuildClusters()",
               layer->name);
    return MS_FAILURE;
  }

  if (!layer->map) {
    msSetError(MS_MISCERR, "No map associated with this layer: %s",
               "RebuildClusters()", layer->name);
    return MS_FAILURE;
  }

  if (layer->debug >= MS_DEBUGLEVEL_VVV)
    msDebug("Clustering started.\n");

  map = layer->map;

  layerinfo->current = layerinfo->finalized; /* restart */

  /* check whether the simplified algorithm was selected */
  pszProcessing = msLayerGetProcessingKey(layer, "CLUSTER_ALGORITHM");
  if (pszProcessing && !strncasecmp(pszProcessing, "SIMPLE", 6))
    layerinfo->algorithm = MSCLUSTER_ALGORITHM_SIMPLE;
  else
    layerinfo->algorithm = MSCLUSTER_ALGORITHM_FULL;

  /* check whether all shapes should be returned from a query */
  if (msLayerGetProcessingKey(layer, "CLUSTER_GET_ALL_SHAPES") != NULL)
    layerinfo->get_all_shapes = MS_TRUE;
  else
    layerinfo->get_all_shapes = MS_FALSE;

  /* check whether the location of the shapes should be preserved */
  if (msLayerGetProcessingKey(layer, "CLUSTER_KEEP_LOCATIONS") != NULL)
    layerinfo->keep_locations = MS_TRUE;
  else
    layerinfo->keep_locations = MS_FALSE;

  /* check whether the maxdistance and the buffer parameters
  are specified in map units (scale independent clustering) */
  if (msLayerGetProcessingKey(layer, "CLUSTER_USE_MAP_UNITS") != NULL)
    layerinfo->use_map_units = MS_TRUE;
  else
    layerinfo->use_map_units = MS_FALSE;

  /* identify the current extent */
  if (layer->transform == MS_TRUE)
    searchrect = map->extent;
  else {
    searchrect.minx = searchrect.miny = 0;
    searchrect.maxx = map->width - 1;
    searchrect.maxy = map->height - 1;
  }

  if (searchrect.minx == layerinfo->searchRect.minx &&
      searchrect.miny == layerinfo->searchRect.miny &&
      searchrect.maxx == layerinfo->searchRect.maxx &&
      searchrect.maxy == layerinfo->searchRect.maxy) {
    /* already built */
    return MS_SUCCESS;
  }

  /* destroy previous data*/
  clusterDestroyData(layerinfo);

  layerinfo->searchRect = searchrect;

  /* reproject the rectangle to layer coordinates */
  if ((map->projection.numargs > 0) && (layer->projection.numargs > 0))
    msProjectRect(&map->projection, &layer->projection,
                  &searchrect); /* project the searchrect to source coords */

  /* determine the compare method */
  layerinfo->fnCompare = CompareRectangleRegion;
  if (layer->cluster.region) {
    if (EQUAL(layer->cluster.region, "ellipse"))
      layerinfo->fnCompare = CompareEllipseRegion;
  }

  /* trying to find a reasonable quadtree depth */
  depth = 0;
  distance = layer->cluster.maxdistance;
  if (layerinfo->use_map_units == MS_TRUE) {
    while ((distance < (searchrect.maxx - searchrect.minx) ||
            distance < (searchrect.maxy - searchrect.miny)) &&
           depth <= TREE_MAX_DEPTH) {
      distance *= 2;
      ++depth;
    }
    cellSizeX = 1;
    cellSizeY = 1;
  } else {
    while ((distance < map->width || distance < map->height) &&
           depth <= TREE_MAX_DEPTH) {
      distance *= 2;
      ++depth;
    }
    cellSizeX = MS_CELLSIZE(searchrect.minx, searchrect.maxx, map->width);
    cellSizeY = MS_CELLSIZE(searchrect.miny, searchrect.maxy, map->height);
  }

  layerinfo->depth = depth;

  maxDistanceX = layer->cluster.maxdistance * cellSizeX;
  maxDistanceY = layer->cluster.maxdistance * cellSizeY;

  /* increase the search rectangle so that the neighbouring shapes are also
   * retrieved */
  searchrect.minx -= layer->cluster.buffer * cellSizeX;
  searchrect.maxx += layer->cluster.buffer * cellSizeX;
  searchrect.miny -= layer->cluster.buffer * cellSizeY;
  searchrect.maxy += layer->cluster.buffer * cellSizeY;

  /* create the root node */
  if (layerinfo->root)
    clusterTreeNodeDestroy(layerinfo, layerinfo->root);
  layerinfo->root = clusterTreeNodeCreate(layerinfo, searchrect);

  srcLayer = &layerinfo->srcLayer;

  /* start retrieving the shapes */
  status = msLayerWhichShapes(srcLayer, searchrect, isQuery);
  if (status == MS_DONE) {
    /* no overlap */
    return MS_SUCCESS;
  } else if (status != MS_SUCCESS) {
    return MS_FAILURE;
  }

  /* step through the source shapes and populate the quadtree with the tentative
   * clusters */
  if ((current = clusterInfoCreate(layerinfo)) == NULL)
    return MS_FAILURE;

#if defined(USE_CLUSTER_EXTERNAL)
  if (srcLayer->transform == MS_TRUE && srcLayer->project &&
      layer->transform == MS_TRUE && layer->project &&
      msProjectionsDiffer(&(srcLayer->projection), &(layer->projection))) {
    reprojector =
        msProjectCreateReprojector(&srcLayer->projection, &layer->projection);
  }
#endif

  while ((status = msLayerNextShape(srcLayer, &current->shape)) == MS_SUCCESS) {
#if defined(USE_CLUSTER_EXTERNAL)
    /* transform the shape to the projection of this layer */
    if (reprojector)
      msProjectShapeEx(reprojector, &current->shape);
#endif
    /* set up positions and variance */
    current->avgx = current->x = current->shape.bounds.minx;
    current->avgy = current->y = current->shape.bounds.miny;
    current->varx = current->vary = 0;
    /* set up the area of interest when searching for the neighboring shapes */
    current->bounds.minx = current->x - maxDistanceX;
    current->bounds.miny = current->y - maxDistanceY;
    current->bounds.maxx = current->x + maxDistanceX;
    current->bounds.maxy = current->y + maxDistanceY;

    /* if the shape doesn't overlap we must skip it to avoid further issues */
    if (!msRectOverlap(&searchrect, &current->bounds)) {
      msFreeShape(&current->shape);
      msInitShape(&current->shape);

      msDebug(
          "Skipping an invalid shape falling outside of the given extent\n");
      continue;
    }

    /* construct the item array */
    if (layer->iteminfo)
      BuildFeatureAttributes(layer, layerinfo, &current->shape);

    /* evaluate the group expression */
    if (layer->cluster.group.string)
      current->group =
          msClusterGetGroupText(&layer->cluster.group, &current->shape);

    if (layerinfo->algorithm == MSCLUSTER_ALGORITHM_FULL) {
      /*start a query for the related shapes */
      findRelatedShapes(layerinfo, layerinfo->root, current);

      /* add this shape to the tree */
      if (treeNodeAddShape(layerinfo, layerinfo->root, current, depth) !=
          MS_SUCCESS) {
        clusterInfoDestroyList(layerinfo, current);
        msProjectDestroyReprojector(reprojector);
        return MS_FAILURE;
      }
    } else if (layerinfo->algorithm == MSCLUSTER_ALGORITHM_SIMPLE) {
      /* find a related cluster and try to assign */
      layerinfo->rank = 0;
      layerinfo->current = NULL;
      findRelatedShapes2(layerinfo, layerinfo->root, current);
      if (layerinfo->current) {
        /* store these points until all clusters are created */
        current->next = layerinfo->finalizedSiblings;
        layerinfo->finalizedSiblings = current;
      } else {
        /* if not found add this shape as a new cluster */
        if (treeNodeAddShape(layerinfo, layerinfo->root, current, depth) !=
            MS_SUCCESS) {
          clusterInfoDestroyList(layerinfo, current);
          msProjectDestroyReprojector(reprojector);
          return MS_FAILURE;
        }
      }
    }

    if ((current = clusterInfoCreate(layerinfo)) == NULL) {
      clusterInfoDestroyList(layerinfo, current);
      msProjectDestroyReprojector(reprojector);
      return MS_FAILURE;
    }
  }

  msProjectDestroyReprojector(reprojector);

  clusterInfoDestroyList(layerinfo, current);

  if (layerinfo->algorithm == MSCLUSTER_ALGORITHM_FULL) {
    while (layerinfo->root) {
#ifdef TESTCOUNT
      int n;
      double avgx, avgy;
#endif

      /* pick up the best cluster from the tree and do the finalization */
      /* the initial rank must be big enough */
      layerinfo->rank = (searchrect.maxx - searchrect.minx) *
                            (searchrect.maxx - searchrect.minx) +
                        (searchrect.maxy - searchrect.miny) *
                            (searchrect.maxy - searchrect.miny) +
                        1;

      layerinfo->current = NULL;
      findBestCluster(layer, layerinfo, layerinfo->root);

      if (layerinfo->current == NULL) {
        if (layer->debug >= MS_DEBUGLEVEL_VVV)
          msDebug("Clustering terminated.\n");
        break; /* completed */
      }

      /* Update the feature count of the shape */
      InitShapeAttributes(layer, layerinfo->current);

      /* collecting the shapes of the cluster */
      collectClusterShapes(layerinfo, layerinfo->root, layerinfo->current);

      if (layer->debug >= MS_DEBUGLEVEL_VVV) {
        msDebug(
            "processing cluster %p: rank=%lf fcount=%d ncoll=%d nfin=%d "
            "nfins=%d nflt=%d bounds={%lf %lf %lf %lf}\n",
            layerinfo->current, layerinfo->rank,
            layerinfo->current->numsiblings + 1,
            layerinfo->current->numcollected, layerinfo->numFinalized,
            layerinfo->numFinalizedSiblings, layerinfo->numFiltered,
            layerinfo->current->bounds.minx, layerinfo->current->bounds.miny,
            layerinfo->current->bounds.maxx, layerinfo->current->bounds.maxy);
        if (layerinfo->current->node) {
          char pszBuffer[TREE_MAX_DEPTH + 1];
          clusterTreeNode *node = layerinfo->current->node;
          int position = node->position;
          int i = 1;
          while (position > 0 && i <= TREE_MAX_DEPTH) {
            pszBuffer[TREE_MAX_DEPTH - i] = '0' + (position % 4);
            position = position >> 2;
            ++i;
          }
          pszBuffer[TREE_MAX_DEPTH] = 0;

          msDebug(" ->node %p: count=%d index=%d pos=%s subn={%p %p %p %p} "
                  "rect={%lf %lf %lf %lf}\n",
                  node, node->numshapes, node->index,
                  pszBuffer + TREE_MAX_DEPTH - i + 1, node->subnode[0],
                  node->subnode[1], node->subnode[2], node->subnode[3],
                  node->rect.minx, node->rect.miny, node->rect.maxx,
                  node->rect.maxy);
        }
      }

#ifdef TESTCOUNT
      avgx = layerinfo->current->x;
      avgy = layerinfo->current->y;
      n = 0;
#endif

      if (layerinfo->current->numsiblings > 0) {
        /* update the parameters due to the shape removal */
        findRelatedShapesRemove(layerinfo, layerinfo->root, layerinfo->current);

        if (layerinfo->current->filter == 0) {
          /* filtered shapes has no siblings */
          layerinfo->current->numsiblings = 0;
          layerinfo->current->avgx = layerinfo->current->x;
          layerinfo->current->avgy = layerinfo->current->y;
        }

        /* update the parameters of the related shapes if any */
        if (layerinfo->finalizedSiblings) {
          current = layerinfo->finalizedSiblings;
          while (current) {
            /* update the parameters due to the shape removal */
            findRelatedShapesRemove(layerinfo, layerinfo->root, current);
            UpdateShapeAttributes(layer, layerinfo->current, current);
#ifdef TESTCOUNT
            avgx += current->x;
            avgy += current->y;
            ++n;
#endif
            /* setting the average position to the same value */
            current->avgx = layerinfo->current->avgx;
            current->avgy = layerinfo->current->avgy;

            if (current->next == NULL) {
              if (layerinfo->get_all_shapes == MS_TRUE) {
                /* insert the siblings into the finalization list */
                current->next = layerinfo->finalized;
                layerinfo->finalized = layerinfo->finalizedSiblings;
              } else {
                /* preserve the clustered siblings for later use */
                layerinfo->current->siblings = layerinfo->finalizedSiblings;
              }
              break;
            }

            current = current->next;
          }

          layerinfo->finalizedSiblings = NULL;
        }
      }

#ifdef TESTCOUNT
      avgx /= (n + 1);
      avgy /= (n + 1);

      if (layerinfo->current->numsiblings != n)
        layerinfo->current->numsiblings = n;

      if (fabs(layerinfo->current->avgx - avgx) / avgx > 0.000000001 ||
          fabs(layerinfo->current->avgy - avgy) / avgy > 0.000000001) {
        layerinfo->current->avgx = avgx;
        layerinfo->current->avgy = avgy;
      }
#endif
    }
  } else if (layerinfo->algorithm == MSCLUSTER_ALGORITHM_SIMPLE) {
    /* assingn stired points to clusters */
    while (layerinfo->finalizedSiblings) {
      current = layerinfo->finalizedSiblings;
      layerinfo->rank =
          maxDistanceX * maxDistanceX + maxDistanceY * maxDistanceY;
      layerinfo->current = NULL;
      findRelatedShapes2(layerinfo, layerinfo->root, current);
      if (layerinfo->current) {
        clusterInfo *s = layerinfo->current;
        /* found a matching cluster */
        ++s->numsiblings;
        /* assign to cluster */
        layerinfo->finalizedSiblings = current->next;
        current->next = s->siblings;
        s->siblings = current;
      } else {
        /* this appears to be a bug */
        layerinfo->finalizedSiblings = current->next;
        current->next = layerinfo->filtered;
        layerinfo->filtered = current;
        ++layerinfo->numFiltered;
      }
    }

    /* collecting the shapes of the cluster */
    collectClusterShapes2(layer, layerinfo, layerinfo->root);
  }

  /* set the pointer to the first shape */
  layerinfo->current = layerinfo->finalized;

  return MS_SUCCESS;
}

/* Close the the combined layer */
int msClusterLayerClose(layerObj *layer) {
  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (!layerinfo)
    return MS_SUCCESS;

  clusterDestroyData(layerinfo);

  msLayerClose(&layerinfo->srcLayer);
  freeLayer(&layerinfo->srcLayer);

  msFree(layerinfo);
  layer->layerinfo = NULL;

#ifndef USE_CLUSTER_EXTERNAL
  /* switch back to the source layer vtable */
  if (msInitializeVirtualTable(layer) != MS_SUCCESS)
    return MS_FAILURE;
#endif

  return MS_SUCCESS;
}

/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msClusterLayerIsOpen(layerObj *layer) {
  if (layer->layerinfo)
    return (MS_TRUE);
  else
    return (MS_FALSE);
}

/* Free the itemindexes array in a layer. */
void msClusterLayerFreeItemInfo(layerObj *layer) {
  msFree(layer->iteminfo);
  layer->iteminfo = NULL;
}

/* allocate the iteminfo index array - same order as the item list */
int msClusterLayerInitItemInfo(layerObj *layer) {
  int i, numitems;
  int *itemindexes;

  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (layer->numitems == 0) {
    return MS_SUCCESS;
  }

  if (!layerinfo)
    return MS_FAILURE;

  /* Cleanup any previous item selection */
  msClusterLayerFreeItemInfo(layer);

  layer->iteminfo = (int *)msSmallMalloc(sizeof(int) * layer->numitems);

  itemindexes = layer->iteminfo;

  /* check whether we require attributes from the source layers also */
  numitems = 0;
  for (i = 0; i < layer->numitems; i++) {
    if (EQUAL(layer->items[i], MSCLUSTER_FEATURECOUNT))
      itemindexes[i] = MSCLUSTER_FEATURECOUNTINDEX;
    else if (EQUAL(layer->items[i], MSCLUSTER_GROUP))
      itemindexes[i] = MSCLUSTER_GROUPINDEX;
    else if (EQUAL(layer->items[i], MSCLUSTER_BASEFID))
      itemindexes[i] = MSCLUSTER_BASEFIDINDEX;
    else
      itemindexes[i] = numitems++;
  }

  msLayerFreeItemInfo(&layerinfo->srcLayer);
  if (layerinfo->srcLayer.items) {
    msFreeCharArray(layerinfo->srcLayer.items, layerinfo->srcLayer.numitems);
    layerinfo->srcLayer.items = NULL;
    layerinfo->srcLayer.numitems = 0;
  }

  if (numitems > 0) {
    /* now allocate and set the layer item parameters  */
    layerinfo->srcLayer.items =
        (char **)msSmallMalloc(sizeof(char *) * numitems);
    layerinfo->srcLayer.numitems = numitems;

    for (i = 0; i < layer->numitems; i++) {
      if (itemindexes[i] >= 0) {
        if (EQUALN(layer->items[i], "Min:", 4))
          layerinfo->srcLayer.items[itemindexes[i]] =
              msStrdup(layer->items[i] + 4);
        else if (EQUALN(layer->items[i], "Max:", 4))
          layerinfo->srcLayer.items[itemindexes[i]] =
              msStrdup(layer->items[i] + 4);
        else if (EQUALN(layer->items[i], "Sum:", 4))
          layerinfo->srcLayer.items[itemindexes[i]] =
              msStrdup(layer->items[i] + 4);
        else if (EQUALN(layer->items[i], "Count:", 6))
          layerinfo->srcLayer.items[itemindexes[i]] =
              msStrdup(layer->items[i] + 6);
        else
          layerinfo->srcLayer.items[itemindexes[i]] = msStrdup(layer->items[i]);
      }
    }

    if (msLayerInitItemInfo(&layerinfo->srcLayer) != MS_SUCCESS)
      return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/* Execute a query for this layer */
int msClusterLayerWhichShapes(layerObj *layer, rectObj rect, int isQuery) {
  (void)rect;
  /* rebuild the cluster database */
  return RebuildClusters(layer, isQuery);
}

static int prepareShape(layerObj *layer, msClusterLayerInfo *layerinfo,
                        clusterInfo *current, shapeObj *shape) {
  (void)layer;
  if (msCopyShape(&(current->shape), shape) != MS_SUCCESS) {
    msSetError(
        MS_SHPERR,
        "Cannot retrieve inline shape. There some problem with the shape",
        "msClusterLayerNextShape()");
    return MS_FAILURE;
  }

  /* update the positions of the cluster shape */
  if (layerinfo->keep_locations == MS_FALSE) {
    shape->line[0].point[0].x = shape->bounds.minx = shape->bounds.maxx =
        current->avgx;
    shape->line[0].point[0].y = shape->bounds.miny = shape->bounds.maxy =
        current->avgy;
  }

  return MS_SUCCESS;
}

/* Execute a query on the DB based on fid. */
int msClusterLayerGetShape(layerObj *layer, shapeObj *shape,
                           resultObj *record) {
  clusterInfo *current;
  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer not open: %s", "msClusterLayerGetShape()",
               layer->name);
    return MS_FAILURE;
  }

  current = layerinfo->finalized;
  while (current) {
    if (record->shapeindex == current->shape.index &&
        record->tileindex == current->shape.tileindex)
      break;
    current = current->next;
  }

  if (current == NULL) {
    msSetError(MS_SHPERR, "No feature with this index.",
               "msClusterLayerGetShape()");
    return MS_FAILURE;
  }

  return prepareShape(layer, layerinfo, current, shape);
}

/* find the next shape with the appropriate shape type */
/* also, load in the attribute data */
/* MS_DONE => no more data */
int msClusterLayerNextShape(layerObj *layer, shapeObj *shape) {
  int rv;
  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer not open: %s", "msClusterLayerNextShape()",
               layer->name);
    return MS_FAILURE;
  }

  if (!layerinfo->current)
    return MS_DONE;

  rv = prepareShape(layer, layerinfo, layerinfo->current, shape);

  layerinfo->current = layerinfo->current->next;

  return rv;
}

/* Query for the items collection */
int msClusterLayerGetItems(layerObj *layer) {
  /* we support certain built in attributes */
  layer->numitems = MSCLUSTER_NUMITEMS;
  layer->items = msSmallMalloc(sizeof(char *) * (layer->numitems));
  layer->items[0] = msStrdup(MSCLUSTER_FEATURECOUNT);
  layer->items[1] = msStrdup(MSCLUSTER_GROUP);
  layer->items[2] = msStrdup(MSCLUSTER_BASEFID);

  return msClusterLayerInitItemInfo(layer);
}

int msClusterLayerGetNumFeatures(layerObj *layer) {
  msClusterLayerInfo *layerinfo = (msClusterLayerInfo *)layer->layerinfo;

  if (!layerinfo)
    return -1;

  return layerinfo->numFinalized;
}

static int msClusterLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                                      shapeObj *shape) {
  (void)map;
  (void)layer;
  (void)c;
  (void)shape;
  /* TODO */
  return MS_SUCCESS;
}

msClusterLayerInfo *msClusterInitialize(layerObj *layer) {
  msClusterLayerInfo *layerinfo =
      (msClusterLayerInfo *)msSmallMalloc(sizeof(msClusterLayerInfo));

  layer->layerinfo = layerinfo;

  layerinfo->searchRect.minx = -1;
  layerinfo->searchRect.miny = -1;
  layerinfo->searchRect.maxx = -1;
  layerinfo->searchRect.maxy = -1;

  layerinfo->root = NULL;

  layerinfo->get_all_shapes = MS_FALSE;

  layerinfo->numFeatures = 0;
  layerinfo->numNodes = 0;

  layerinfo->finalized = NULL;
  layerinfo->numFinalized = 0;
  layerinfo->finalizedSiblings = NULL;
  layerinfo->numFinalizedSiblings = 0;
  layerinfo->filtered = NULL;
  layerinfo->numFiltered = 0;

  layerinfo->finalizedNodes = NULL;
  layerinfo->numFinalizedNodes = 0;

  return layerinfo;
}

int msClusterLayerOpen(layerObj *layer) {
  msClusterLayerInfo *layerinfo;

  if (layer->type != MS_LAYER_POINT) {
    msSetError(MS_MISCERR, "Only point layers are supported for clustering: %s",
               "msClusterLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  if (!layer->map)
    return MS_FAILURE;

  if (layer->layerinfo) {
    if (layer->vtable->LayerOpen != msClusterLayerOpen)
      msLayerClose(layer);
    else
      return MS_SUCCESS; /* already open */
  }

  layerinfo = msClusterInitialize(layer);

  if (!layer->layerinfo)
    return MS_FAILURE;

  /* prepare the source layer */
  if (initLayer(&layerinfo->srcLayer, layer->map) == -1)
    return MS_FAILURE;

#ifdef USE_CLUSTER_EXTERNAL
  if (!layer->map)
    return MS_FAILURE;

  layerIndex = msGetLayerIndex(layer->map, layer->connection);

  if (layerIndex < 0 && layerIndex >= layer->map->numlayers) {
    msSetError(MS_MISCERR, "No source layers specified in layer: %s",
               "msClusterLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  if (layer->map->layers[layerIndex]->type != MS_LAYER_POINT) {
    msSetError(MS_MISCERR,
               "Only point layers are supported for cluster data source: %s",
               "msClusterLayerOpen()", layer->name);
    return MS_FAILURE;
  }

  if (msCopyLayer(&layerinfo->srcLayer, layer->map->layers[layerIndex]) !=
      MS_SUCCESS)
    return (MS_FAILURE);
#else
  /* hook the vtable to this driver, will be restored in LayerClose*/
  if (!layer->vtable) {
    if (msInitializeVirtualTable(layer) != MS_SUCCESS)
      return MS_FAILURE;
  }
  assert(layer->vtable);
  msClusterLayerCopyVirtualTable(layer->vtable);

  if (msCopyLayer(&layerinfo->srcLayer, layer) != MS_SUCCESS)
    return (MS_FAILURE);
#endif

  /* disable the connection pool for this layer */
  msLayerSetProcessingKey(&layerinfo->srcLayer, "CLOSE_CONNECTION", "ALWAYS");

  /* open the source layer */
  if (!layerinfo->srcLayer.vtable) {
    if (msInitializeVirtualTable(&layerinfo->srcLayer) != MS_SUCCESS)
      return MS_FAILURE;
  }

  if (layerinfo->srcLayer.vtable->LayerOpen(&layerinfo->srcLayer) !=
      MS_SUCCESS) {
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

int msClusterLayerTranslateFilter(layerObj *layer, expressionObj *filter,
                                  char *filteritem) {
  (void)filter;
  msClusterLayerInfo *layerinfo = layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer is not open: %s",
               "msClusterLayerTranslateFilter()", layer->name);
    return MS_FAILURE;
  }

  if (layerinfo->srcLayer.filter.type == MS_EXPRESSION &&
      layerinfo->srcLayer.filter.tokens == NULL)
    msTokenizeExpression(&(layerinfo->srcLayer.filter), layer->items,
                         &(layer->numitems));

  return layerinfo->srcLayer.vtable->LayerTranslateFilter(
      &layerinfo->srcLayer, &layerinfo->srcLayer.filter, filteritem);
}

char *msClusterLayerEscapeSQLParam(layerObj *layer, const char *pszString) {
  msClusterLayerInfo *layerinfo = layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer is not open: %s",
               "msClusterLayerEscapeSQLParam()", layer->name);
    return msStrdup("");
  }

  return layerinfo->srcLayer.vtable->LayerEscapeSQLParam(&layerinfo->srcLayer,
                                                         pszString);
}

int msClusterLayerGetAutoProjection(layerObj *layer,
                                    projectionObj *projection) {
  msClusterLayerInfo *layerinfo = layer->layerinfo;

  if (!layerinfo) {
    msSetError(MS_MISCERR, "Layer is not open: %s",
               "msClusterLayerGetAutoProjection()", layer->name);
    return MS_FAILURE;
  }

  return layerinfo->srcLayer.vtable->LayerGetAutoProjection(
      &layerinfo->srcLayer, projection);
}

int msClusterLayerGetPaging(layerObj *layer) {
  (void)layer;
  return MS_FALSE;
}

void msClusterLayerEnablePaging(layerObj *layer, int value) {
  (void)layer;
  (void)value;
}

void msClusterLayerCopyVirtualTable(layerVTableObj *vtable) {
  vtable->LayerInitItemInfo = msClusterLayerInitItemInfo;
  vtable->LayerFreeItemInfo = msClusterLayerFreeItemInfo;
  vtable->LayerOpen = msClusterLayerOpen;
  vtable->LayerIsOpen = msClusterLayerIsOpen;
  vtable->LayerWhichShapes = msClusterLayerWhichShapes;
  vtable->LayerNextShape = msClusterLayerNextShape;
  vtable->LayerGetShape = msClusterLayerGetShape;
  /* layer->vtable->LayerGetShapeCount, use default */

  vtable->LayerClose = msClusterLayerClose;

  vtable->LayerGetItems = msClusterLayerGetItems;
  vtable->LayerCloseConnection = msClusterLayerClose;

  vtable->LayerGetNumFeatures = msClusterLayerGetNumFeatures;
  vtable->LayerGetAutoStyle = msClusterLayerGetAutoStyle;
  vtable->LayerTranslateFilter = msClusterLayerTranslateFilter;
  /* vtable->LayerSupportsCommonFilters, use driver implementation */
  vtable->LayerEscapeSQLParam = msClusterLayerEscapeSQLParam;
  /* vtable->LayerEscapePropertyName, use driver implementation */

  vtable->LayerEnablePaging = msClusterLayerEnablePaging;
  vtable->LayerGetPaging = msClusterLayerGetPaging;
  vtable->LayerGetAutoProjection = msClusterLayerGetAutoProjection;
}

#ifdef USE_CLUSTER_PLUGIN

MS_DLL_EXPORT int PluginInitializeVirtualTable(layerVTableObj *vtable,
                                               layerObj *layer) {
  assert(layer != NULL);
  assert(vtable != NULL);

  msClusterLayerCopyVirtualTable(vtable);

  return MS_SUCCESS;
}

#endif

int msClusterLayerInitializeVirtualTable(layerObj *layer) {
  assert(layer != NULL);
  assert(layer->vtable != NULL);

  msClusterLayerCopyVirtualTable(layer->vtable);

  return MS_SUCCESS;
}
