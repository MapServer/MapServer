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

#include "cql2.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <queue>
#include <string>

#include "mapogcfilter.h"
#include "mapows.h"

/************************************************************************/
/*                             to_string()                              */
/************************************************************************/

static std::string to_string(double x) {
  char szBuf[32];
  snprintf(szBuf, sizeof(szBuf), "%.17g", x);
  return szBuf;
}

/************************************************************************/
/*                       cql2_expr_node(cql2_op)                        */
/************************************************************************/

cql2_expr_node::cql2_expr_node(cql2_op op)
    : m_op(op), m_node_type(CQL2_NT_OPERATION) {
  switch (op) {
  case CQL2_OR:
  case CQL2_AND:
  case CQL2_NOT:
  case CQL2_EQ:
  case CQL2_IEQ:
  case CQL2_NE:
  case CQL2_INE:
  case CQL2_GE:
  case CQL2_LE:
  case CQL2_LT:
  case CQL2_GT:
  case CQL2_IN:
  case CQL2_LIKE:
  case CQL2_ILIKE:
  case CQL2_BETWEEN:
  case CQL2_IS_NULL:
  case CQL2_S_INTERSECTS:
  case CQL2_S_EQUALS:
  case CQL2_S_DISJOINT:
  case CQL2_S_TOUCHES:
  case CQL2_S_WITHIN:
  case CQL2_S_OVERLAPS:
  case CQL2_S_CROSSES:
  case CQL2_S_CONTAINS:
    m_field_type = CQL2_BOOLEAN;
    break;

  case CQL2_ADD:
  case CQL2_SUBTRACT:
  case CQL2_MULTIPLY:
  case CQL2_DIV:
  case CQL2_MOD:
  case CQL2_ARGUMENT_LIST:
  case CQL2_CUSTOM_FUNC:
    break;

  case CQL2_NONE:
    assert(false);
    break;
  }
}

/************************************************************************/
/*                          ~cql2_expr_node()                           */
/************************************************************************/

cql2_expr_node::~cql2_expr_node() = default;

/************************************************************************/
/*                         PushSubExpression()                          */
/************************************************************************/

void cql2_expr_node::PushSubExpression(cql2_expr_node *child) {
  m_depth = std::max(m_depth, 1 + child->m_depth);
  m_apoChildren.push_back(std::unique_ptr<cql2_expr_node>(child));
}

/************************************************************************/
/*                       ReverseSubExpressions()                        */
/************************************************************************/

void cql2_expr_node::ReverseSubExpressions() {
  for (size_t i = 0; i < m_apoChildren.size() / 2; i++) {
    std::swap(m_apoChildren[i], m_apoChildren[m_apoChildren.size() - i - 1]);
  }
}

/************************************************************************/
/*                           RebalanceAndOr()                           */
/************************************************************************/

void cql2_expr_node::RebalanceAndOr() {
  std::queue<cql2_expr_node *> nodes;
  nodes.push(this);
  while (!nodes.empty()) {
    cql2_expr_node *node = nodes.front();
    nodes.pop();
    if (node->m_node_type == CQL2_NT_OPERATION) {
      const auto op = node->m_op;
      if ((op == CQL2_OR || op == CQL2_AND) && node->m_apoChildren.size() > 2) {
        std::vector<std::unique_ptr<cql2_expr_node>> exprs;
        for (auto &subexpr : node->m_apoChildren) {
          subexpr->RebalanceAndOr();
          exprs.push_back(std::move(subexpr));
        }
        node->m_apoChildren.clear();

        while (exprs.size() > 2) {
          std::vector<std::unique_ptr<cql2_expr_node>> new_exprs;
          for (size_t i = 0; i < exprs.size(); i++) {
            if (i + 1 < exprs.size()) {
              auto cur_expr = std::make_unique<cql2_expr_node>(op);
              cur_expr->PushSubExpression(exprs[i].release());
              cur_expr->PushSubExpression(exprs[i + 1].release());
              i++;
              new_exprs.push_back(std::move(cur_expr));
            } else {
              new_exprs.push_back(std::move(exprs[i]));
            }
          }
          exprs = std::move(new_exprs);
        }
        assert(exprs.size() == 2);
        node->m_apoChildren = std::move(exprs);
      } else {
        for (auto &subexpr : node->m_apoChildren) {
          nodes.push(subexpr.get());
        }
      }
    }
  }
}

/************************************************************************/
/*                        cql2_create_and_or_or()                        */
/************************************************************************/

cql2_expr_node *cql2_create_and_or_or(cql2_op op, cql2_expr_node *left,
                                      cql2_expr_node *right) {
  auto poNode = new cql2_expr_node(op);
  poNode->m_field_type = CQL2_BOOLEAN;

  if (left->m_node_type == CQL2_NT_OPERATION && left->m_op == op) {
    poNode->m_apoChildren = std::move(left->m_apoChildren);
    delete left;

    // Temporary non-binary formulation
    if (right->m_node_type == CQL2_NT_OPERATION && right->m_op == op) {
      poNode->m_apoChildren.insert(
          poNode->m_apoChildren.end(),
          std::make_move_iterator(right->m_apoChildren.begin()),
          std::make_move_iterator(right->m_apoChildren.end()));
      delete right;
    } else {
      poNode->m_apoChildren.push_back(std::unique_ptr<cql2_expr_node>(right));
    }
  } else if (right->m_node_type == CQL2_NT_OPERATION && right->m_op == op) {
    // Temporary non-binary formulation
    poNode->m_apoChildren = std::move(right->m_apoChildren);
    delete right;

    poNode->m_apoChildren.push_back(std::unique_ptr<cql2_expr_node>(left));
    delete left;
  } else {
    poNode->m_apoChildren.push_back(std::unique_ptr<cql2_expr_node>(left));
    poNode->m_apoChildren.push_back(std::unique_ptr<cql2_expr_node>(right));
  }

  return poNode;
}

/************************************************************************/
/*                         getItemAliasOrName()                         */
/************************************************************************/

static const char *getItemAliasOrName(const layerObj *layer, const char *item) {
  std::string key = std::string(item) + "_alias";
  if (const char *value =
          msOWSLookupMetadata(&(layer->metadata), "OGA", key.c_str())) {
    return value;
  }
  return item;
}

/************************************************************************/
/*                           ChangeBBOXToWKT()                          */
/************************************************************************/

bool cql2_expr_node::ChangeBBOXToWKT(const layerObj *layer,
                                     const std::string &filterCrs,
                                     bool axisInverted) {
  if (m_node_type == CQL2_NT_OPERATION && m_op == CQL2_CUSTOM_FUNC &&
      EQUAL(m_osVal.c_str(), "BBOX") && m_apoChildren.size() == 4 &&
      m_apoChildren[0]->IsNumeric() && m_apoChildren[1]->IsNumeric() &&
      m_apoChildren[2]->IsNumeric() && m_apoChildren[3]->IsNumeric()) {
    rectObj rect;
    rect.minx = m_apoChildren[0]->AsDouble();
    rect.miny = m_apoChildren[1]->AsDouble();
    rect.maxx = m_apoChildren[2]->AsDouble();
    rect.maxy = m_apoChildren[3]->AsDouble();

    if (axisInverted) {
      std::swap(rect.minx, rect.miny);
      std::swap(rect.maxx, rect.maxy);
    }

    projectionObj shapeProj;
    msInitProjection(&shapeProj);
    msProjectionInheritContextFrom(&shapeProj, &(layer->projection));
    if (msLoadProjectionString(&shapeProj, filterCrs.c_str()) != 0) {
      msFreeProjection(&shapeProj);
      return false;
    }

    int status = msProjectRect(
        &shapeProj, const_cast<projectionObj *>(&(layer->projection)), &rect);
    msFreeProjection(&shapeProj);
    if (status != MS_SUCCESS) {
      return false;
    }

    const std::string minx = to_string(rect.minx);
    const std::string miny = to_string(rect.miny);
    const std::string maxx = to_string(rect.maxx);
    const std::string maxy = to_string(rect.maxy);

    std::string s("POLYGON((");
    s += minx + " " + miny + ",";
    s += minx + " " + maxy + ",";
    s += maxx + " " + maxy + ",";
    s += maxx + " " + miny + ",";
    s += minx + " " + miny;
    s += "))";

    m_node_type = CQL2_NT_CONSTANT;
    m_field_type = CQL2_WKT;
    m_osVal = std::move(s);
    m_apoChildren.clear();
    m_alreadyReprojected = true;

    return true;

  } else {
    for (auto &child : m_apoChildren) {
      if (!child->ChangeBBOXToWKT(layer, filterCrs, axisInverted)) {
        return false;
      }
    }
  }
  return true;
}

/************************************************************************/
/*                         ToMapServerFilter()                          */
/************************************************************************/

std::string cql2_expr_node::ToMapServerFilter(
    const layerObj *layer, const std::vector<std::string> &queryableItems,
    const std::string &geometryName, const std::string &filterCrs,
    bool axisInverted, std::string &errorMsg) const {
  if (!errorMsg.empty())
    return std::string();

  switch (m_node_type) {
  case CQL2_NT_CONSTANT: {
    switch (m_field_type) {
    case CQL2_INTEGER: {
      return std::to_string(m_nVal);
    }

    case CQL2_DOUBLE: {
      return to_string(m_dfVal);
    }

    case CQL2_STRING: {
      return std::string("'")
          .append(msStdStringEscape(m_osVal.c_str()))
          .append("'");
    }

    case CQL2_WKT: {
      if (m_alreadyReprojected)
        return m_osVal;

      shapeObj *shape = msShapeFromWKT(m_osVal.c_str());
      if (!shape) {
        errorMsg = m_osVal;
        errorMsg += " is invalid WKT";
        return std::string();
      }

      projectionObj shapeProj;
      msInitProjection(&shapeProj);
      msProjectionInheritContextFrom(&shapeProj, &(layer->projection));
      if (msLoadProjectionString(&shapeProj, filterCrs.c_str()) != 0) {
        msFreeProjection(&shapeProj);
        errorMsg = "Cannot process filter-crs";
        return std::string();
      }

      if (axisInverted)
        msAxisSwapShape(shape);

      if (msProjectShape(&shapeProj,
                         const_cast<projectionObj *>(&(layer->projection)),
                         shape) != MS_SUCCESS) {
        msFreeProjection(&shapeProj);
        errorMsg = "Cannot reproject " + m_osVal;
        return std::string();
      }

      msFreeProjection(&shapeProj);
      char *wkt = msShapeToWKT(shape);

      msFreeShape(shape);
      msFree(shape);

      if (!wkt) {
        errorMsg = "Cannot export back to WKT";
        return std::string();
      }
      std::string s(wkt);
      msFree(wkt);
      return s;
    }

    case CQL2_BOOLEAN: {
      return m_nVal ? "1" : "0";
    }
    }
    break;
  }

  case CQL2_NT_COLUMN: {
    if (std::find(queryableItems.begin(), queryableItems.end(),
                  m_osVal.c_str()) == queryableItems.end()) {
      errorMsg = m_osVal;
      errorMsg += " is not a queryable item";
      break;
    }

    // Find actual item name from alias
    const char *pszItem = nullptr;
    for (int j = 0; j < layer->numitems; ++j) {
      if (m_osVal == getItemAliasOrName(layer, layer->items[j])) {
        pszItem = layer->items[j];
        break;
      }
    }

    const char *pszType =
        msOWSLookupMetadata(&(layer->metadata), "OFG",
                            std::string(pszItem).append("_type").c_str());
    if (pszType && (EQUAL(pszType, "Integer") || EQUAL(pszType, "Long") ||
                    EQUAL(pszType, "Real")))
      return std::string("[").append(msStdStringEscape(pszItem)).append("]");
    else if (pszType && (EQUAL(pszType, "Date") || EQUAL(pszType, "DateTime")))
      return std::string("`[").append(msStdStringEscape(pszItem)).append("]`");
    else
      return std::string("\"[")
          .append(msStdStringEscape(pszItem))
          .append("]\"");
  }

  case CQL2_NT_OPERATION: {
    const auto ReturnBinaryOp = [this, layer, axisInverted, &queryableItems,
                                 &geometryName, &filterCrs,
                                 &errorMsg](const char *pszOp) {
      assert(m_apoChildren.size() == 2);
      return std::string("(")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(" ")
          .append(pszOp)
          .append(" ")
          .append(m_apoChildren[1]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(")");
    };

    switch (m_op) {
    case CQL2_OR:
      return ReturnBinaryOp("OR");

    case CQL2_AND:
      return ReturnBinaryOp("AND");

    case CQL2_NOT: {
      assert(m_apoChildren.size() == 1);
      return std::string("(NOT ")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(")");
    }

    case CQL2_EQ:
      return ReturnBinaryOp("=");

    case CQL2_NE:
      return ReturnBinaryOp("!=");

    case CQL2_GE:
      return ReturnBinaryOp(">=");

    case CQL2_LE:
      return ReturnBinaryOp("<=");

    case CQL2_LT:
      return ReturnBinaryOp("<");

    case CQL2_GT:
      return ReturnBinaryOp(">");

    case CQL2_IN: {
      assert(m_apoChildren.size() >= 2);
      std::string s = "((";
      for (size_t i = 1; i < m_apoChildren.size(); ++i) {
        if (i > 1)
          s.append(") OR (");
        s.append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                     geometryName, filterCrs,
                                                     axisInverted, errorMsg))
            .append(" ")
            .append("=")
            .append(" ")
            .append(m_apoChildren[i]->ToMapServerFilter(
                layer, queryableItems, geometryName, filterCrs, axisInverted,
                errorMsg));
      }
      s += "))";
      return s;
    }

    case CQL2_LIKE:
    case CQL2_ILIKE: {
      assert(m_apoChildren.size() == 2);
      FEPropertyIsLike propIsLike;
      propIsLike.pszWildCard = const_cast<char *>("%");
      propIsLike.pszSingleChar = const_cast<char *>("_");
      propIsLike.pszEscapeChar = const_cast<char *>("\\");
      propIsLike.bCaseInsensitive = 0;
      return std::string("(")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(" ")
          .append(m_op == CQL2_LIKE ? "~" : "~*")
          .append(" \"")
          .append(msGetLikePatternAsRegex(&propIsLike,
                                          m_apoChildren[1]->m_osVal.c_str()))
          .append("\")");
    }

    case CQL2_IEQ:
    case CQL2_INE: {
      assert(m_apoChildren.size() == 2);
      FEPropertyIsLike propIsLike;
      propIsLike.pszWildCard = const_cast<char *>("");
      propIsLike.pszSingleChar = const_cast<char *>("");
      propIsLike.pszEscapeChar = const_cast<char *>("\\");
      propIsLike.bCaseInsensitive = 0;
      std::string s;
      if (m_op == CQL2_INE)
        s = "( NOT";
      s.append("(")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(" ~* \"")
          .append(msGetLikePatternAsRegex(&propIsLike,
                                          m_apoChildren[1]->m_osVal.c_str()))
          .append("\")");
      if (m_op == CQL2_INE)
        s.append(")");
      return s;
    }

    case CQL2_BETWEEN: {
      assert(m_apoChildren.size() == 3);
      return std::string("((")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(" ")
          .append(">=")
          .append(" ")
          .append(m_apoChildren[1]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(") AND (")
          .append(m_apoChildren[0]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append(" ")
          .append("<=")
          .append(" ")
          .append(m_apoChildren[2]->ToMapServerFilter(layer, queryableItems,
                                                      geometryName, filterCrs,
                                                      axisInverted, errorMsg))
          .append("))");
    }

    case CQL2_IS_NULL: {
      errorMsg = "IS NULL not supported by MapServer";
      break;
    }

    case CQL2_S_INTERSECTS:
    case CQL2_S_EQUALS:
    case CQL2_S_DISJOINT:
    case CQL2_S_TOUCHES:
    case CQL2_S_WITHIN:
    case CQL2_S_OVERLAPS:
    case CQL2_S_CROSSES:
    case CQL2_S_CONTAINS: {
      const char *pszFunc = "";
      switch (m_op) {
      case CQL2_S_INTERSECTS:
        pszFunc = "intersects";
        break;
      case CQL2_S_EQUALS:
        pszFunc = "equals";
        break;
      case CQL2_S_DISJOINT:
        pszFunc = "disjoint";
        break;
      case CQL2_S_TOUCHES:
        pszFunc = "touches";
        break;
      case CQL2_S_WITHIN:
        pszFunc = "within";
        break;
      case CQL2_S_OVERLAPS:
        pszFunc = "overlaps";
        break;
      case CQL2_S_CROSSES:
        pszFunc = "crosses";
        break;
      case CQL2_S_CONTAINS:
        pszFunc = "contains";
        break;
      default:
        assert(false);
        break;
      }

      assert(m_apoChildren.size() == 2);
      if (m_apoChildren[0]->m_node_type != CQL2_NT_COLUMN ||
          m_apoChildren[0]->m_osVal != geometryName) {
        errorMsg = "First parameter of s_";
        errorMsg += pszFunc;
        errorMsg += "() must be the geometry "
                    "column name '" +
                    geometryName + "'";
        return std::string();
      }

      if (!m_apoChildren[1]->ChangeBBOXToWKT(layer, filterCrs, axisInverted)) {
        errorMsg = "Error while reprojecting BBOX";
        return std::string();
      }

      if (m_apoChildren[1]->m_field_type != CQL2_WKT) {
        errorMsg = "Second parameter of s_";
        errorMsg += pszFunc;
        errorMsg += "() must be "
                    "BBOX(minx,miny,maxx,maxy) or a WKT literal'";
        return std::string();
      }

      std::string s("(");
      s += pszFunc;
      s += "([shape],fromText('";
      s += m_apoChildren[1]->ToMapServerFilter(layer, queryableItems,
                                               geometryName, filterCrs,
                                               axisInverted, errorMsg);
      s += "'))=true)";
      return s;
    }

    case CQL2_CUSTOM_FUNC: {
      if (m_osVal == "TIMESTAMP" || m_osVal == "DATE") {
        if (m_apoChildren.size() == 1 &&
            m_apoChildren[0]->m_field_type == CQL2_STRING) {
          return std::string("`")
              .append(msStdStringEscape(m_apoChildren[0]->m_osVal.c_str()))
              .append("`");
        } else {
          errorMsg = "Unhandled syntax for function ";
          errorMsg += m_osVal;
        }
      } else {
        errorMsg = "Unhandled function ";
        errorMsg += m_osVal;
      }
      break;
    }

    case CQL2_ADD:
      return ReturnBinaryOp("+");

    case CQL2_SUBTRACT:
      return ReturnBinaryOp("-");

    case CQL2_MULTIPLY:
      return ReturnBinaryOp("*");

    case CQL2_DIV:
      return ReturnBinaryOp("/");

    case CQL2_MOD:
      return ReturnBinaryOp("%");

    case CQL2_NONE:
    case CQL2_ARGUMENT_LIST:
      assert(false);
      break;
    }
  }
  }
  return std::string();
}
