/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Functions for operating on a classObj that don't belong in a
 *           more specific file such as mapfile.c.
 *           Adapted from mapobject.c.
 * Author:   Sean Gillies, sgillies@frii.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Sean Gillies
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

/*
** Add a label to a classObj (order doesn't matter for labels like it does with
*styles)
*/
int msAddLabelToClass(classObj *class, labelObj *label) {
  if (!label) {
    msSetError(MS_CHILDERR, "Can't add a NULL label.", "msAddLabelToClass()");
    return MS_FAILURE;
  }
  if (msGrowClassLabels(class) == NULL)
    return MS_FAILURE;

  /* msGrowClassLabels will alloc the label, free it in this case */
  free(class->labels[class->numlabels]);
  class->labels[class->numlabels] = label;
  MS_REFCNT_INCR(label);
  class->numlabels++;
  return MS_SUCCESS;
}

/*
** Remove a label from a classObj.
*/
labelObj *msRemoveLabelFromClass(classObj *class, int nLabelIndex) {
  int i;
  labelObj *label;

  if (nLabelIndex < 0 || nLabelIndex >= class->numlabels) {
    msSetError(MS_CHILDERR, "Cannot remove label, invalid index %d",
               "msRemoveLabelFromClass()", nLabelIndex);
    return NULL;
  } else {
    label = class->labels[nLabelIndex];
    for (i = nLabelIndex; i < class->numlabels - 1; i++) {
      class->labels[i] = class->labels[i + 1];
    }
    class->labels[class->numlabels - 1] = NULL;
    class->numlabels--;
    MS_REFCNT_DECR(label);
    return label;
  }
}

/**
 * Move the style up inside the array of styles.
 */
int msMoveStyleUp(classObj *class, int nStyleIndex) {
  styleObj *psTmpStyle = NULL;
  if (class && nStyleIndex < class->numstyles && nStyleIndex > 0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, class->styles[nStyleIndex]);

    msCopyStyle(class->styles[nStyleIndex], class->styles[nStyleIndex - 1]);

    msCopyStyle(class->styles[nStyleIndex - 1], psTmpStyle);

    return (MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveStyleUp()", nStyleIndex);
  return (MS_FAILURE);
}

/**
 * Move the style down inside the array of styles.
 */
int msMoveStyleDown(classObj *class, int nStyleIndex) {
  styleObj *psTmpStyle = NULL;

  if (class && nStyleIndex < class->numstyles - 1 && nStyleIndex >= 0) {
    psTmpStyle = (styleObj *)malloc(sizeof(styleObj));
    initStyle(psTmpStyle);

    msCopyStyle(psTmpStyle, class->styles[nStyleIndex]);

    msCopyStyle(class->styles[nStyleIndex], class->styles[nStyleIndex + 1]);

    msCopyStyle(class->styles[nStyleIndex + 1], psTmpStyle);

    return (MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msMoveStyleDown()",
             nStyleIndex);
  return (MS_FAILURE);
}

/* Moved here from mapscript.i
 *
 * Returns the index at which the style was inserted
 *
 */
int msInsertStyle(classObj *class, styleObj *style, int nStyleIndex) {
  int i;

  if (!style) {
    msSetError(MS_CHILDERR, "Can't insert a NULL Style", "msInsertStyle()");
    return -1;
  }

  /* Ensure there is room for a new style */
  if (msGrowClassStyles(class) == NULL) {
    return -1;
  }
  /* Catch attempt to insert past end of styles array */
  else if (nStyleIndex >= class->numstyles) {
    msSetError(MS_CHILDERR, "Cannot insert style beyond index %d",
               "insertStyle()", class->numstyles - 1);
    return -1;
  } else if (nStyleIndex < 0) { /* Insert at the end by default */
    class->styles[class->numstyles] = style;
    MS_REFCNT_INCR(style);
    class->numstyles++;
    return class->numstyles - 1;
  } else {
    /* Move styles existing at the specified nStyleIndex or greater */
    /* to a higher nStyleIndex */
    for (i = class->numstyles - 1; i >= nStyleIndex; i--) {
      class->styles[i + 1] = class->styles[i];
    }
    class->styles[nStyleIndex] = style;
    MS_REFCNT_INCR(style);
    class->numstyles++;
    return nStyleIndex;
  }
}

styleObj *msRemoveStyle(classObj *class, int nStyleIndex) {
  int i;
  styleObj *style;
  if (nStyleIndex < 0 || nStyleIndex >= class->numstyles) {
    msSetError(MS_CHILDERR, "Cannot remove style, invalid nStyleIndex %d",
               "removeStyle()", nStyleIndex);
    return NULL;
  } else {
    style = class->styles[nStyleIndex];
    for (i = nStyleIndex; i < class->numstyles - 1; i++) {
      class->styles[i] = class->styles[i + 1];
    }
    class->styles[class->numstyles - 1] = NULL;
    class->numstyles--;
    MS_REFCNT_DECR(style);
    return style;
  }
}

/**
 * Delete the style identified by the index and shift
 * styles that follows the deleted style.
 */
int msDeleteStyle(classObj *class, int nStyleIndex) {
  if (class && nStyleIndex < class->numstyles && nStyleIndex >= 0) {
    if (freeStyle(class->styles[nStyleIndex]) == MS_SUCCESS)
      msFree(class->styles[nStyleIndex]);
    for (int i = nStyleIndex; i < class->numstyles - 1; i++) {
      class->styles[i] = class->styles[i + 1];
    }
    class->styles[class->numstyles - 1] = NULL;
    class->numstyles--;
    return (MS_SUCCESS);
  }
  msSetError(MS_CHILDERR, "Invalid index: %d", "msDeleteStyle()", nStyleIndex);
  return (MS_FAILURE);
}
