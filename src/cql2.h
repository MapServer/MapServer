/******************************************************************************
 *
 * Project: MapServer
 * Purpose: CQL2 expression
 * Author: Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (C) 2026 Even Rouault <even dot rouault at spatialys dot com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef CQL2_H_INCLUDED
#define CQL2_H_INCLUDED

#include <memory>
#include <string>
#include <vector>

#include "mapserver.h"

typedef enum {
  CQL2_NONE,

  /* temporary value only set during parsing and replaced by something else
   * at the end */
  CQL2_ARGUMENT_LIST,
  CQL2_CUSTOM_FUNC,

  CQL2_OR,
  CQL2_AND,

  CQL2_NOT,

  CQL2_EQ,
  CQL2_IEQ,
  CQL2_NE,
  CQL2_INE,
  CQL2_GE,
  CQL2_LE,
  CQL2_LT,
  CQL2_GT,

  CQL2_IS_NULL,

  CQL2_IN,
  CQL2_LIKE,
  CQL2_ILIKE,
  CQL2_BETWEEN,

  CQL2_S_INTERSECTS,
  CQL2_S_EQUALS,
  CQL2_S_DISJOINT,
  CQL2_S_TOUCHES,
  CQL2_S_WITHIN,
  CQL2_S_OVERLAPS,
  CQL2_S_CROSSES,
  CQL2_S_CONTAINS,

  CQL2_ADD,
  CQL2_SUBTRACT,
  CQL2_MULTIPLY,
  CQL2_DIV,
  CQL2_MOD,

} cql2_op;

typedef enum {
  CQL2_INTEGER,
  CQL2_DOUBLE,
  CQL2_STRING,
  CQL2_WKT,
  CQL2_BOOLEAN,
} cql2_field_type;

typedef enum {
  CQL2_NT_CONSTANT,
  CQL2_NT_COLUMN,
  CQL2_NT_OPERATION
} cql2_node_type;

struct cql2_expr_node {
  cql2_expr_node(cql2_op op);
  cql2_expr_node(bool v) : m_field_type(CQL2_BOOLEAN), m_nVal(v) {}
  cql2_expr_node(int v) : m_field_type(CQL2_INTEGER), m_nVal(v) {}
  cql2_expr_node(double v) : m_field_type(CQL2_DOUBLE), m_dfVal(v) {}
  cql2_expr_node(const std::string &v)
      : m_field_type(CQL2_STRING), m_osVal(v) {}
  cql2_expr_node(const char *v) : m_field_type(CQL2_STRING), m_osVal(v) {}
  ~cql2_expr_node();

  cql2_op m_op = CQL2_NONE;
  cql2_node_type m_node_type = CQL2_NT_CONSTANT;
  cql2_field_type m_field_type = CQL2_INTEGER;

  int m_nVal = 0;
  double m_dfVal = 0;
  std::string m_osVal{};
  std::vector<std::unique_ptr<cql2_expr_node>> m_apoChildren{};
  int m_depth = 0;
  bool m_alreadyReprojected = false;

  bool HasReachedMaxDepth() const { return m_depth > 32; }
  bool IsNumeric() const {
    return m_field_type == CQL2_INTEGER || m_field_type == CQL2_DOUBLE;
  }
  double AsDouble() const {
    if (m_field_type == CQL2_INTEGER)
      return m_nVal;
    else if (m_field_type == CQL2_DOUBLE)
      return m_dfVal;
    else
      return 0;
  }
  void PushSubExpression(cql2_expr_node *node);
  void ReverseSubExpressions();
  void RebalanceAndOr();
  bool ChangeBBOXToWKT(const layerObj *layer, const std::string &filterCrs,
                       bool axisInverted);
  std::string ToMapServerFilter(const layerObj *layer,
                                const std::vector<std::string> &queryableItems,
                                const std::string &geometryName,
                                const std::string &filterCrs, bool axisInverted,
                                std::string &errorMsg) const;
};

#endif
