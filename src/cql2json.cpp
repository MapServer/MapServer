/******************************************************************************
 *
 * Project: MapServer
 * Purpose: CQL2 JSON Parser
 * Author: Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (C) 2026 Even Rouault <even dot rouault at spatialys dot com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "cql2json.h"

#include "cpl_conv.h"
#include "ogr_api.h"

#include "third-party/include_nlohmann_json.hpp"

using json = nlohmann::json;

static std::unique_ptr<cql2_expr_node> ParseOperator(const json &j,
                                                     std::string &errorMsg);

/************************************************************************/
/*                           sGeoJSONType()                             */
/************************************************************************/

static bool isGeoJSONType(const std::string &s) {
  return s == "Point" || s == "LineString" || s == "Polygon" ||
         s == "MultiPoint" || s == "MultiLineString" || s == "MultiPolygon" ||
         s == "GeometryCollection";
}

/************************************************************************/
/*                            ParseObject()                             */
/************************************************************************/

static std::unique_ptr<cql2_expr_node> ParseObject(const json &j,
                                                   std::string &errorMsg) {
  if (j.is_object()) {
    if (j.contains("op")) {
      return ParseOperator(j, errorMsg);
    } else if (j.contains("property")) {
      const auto property = j["property"];
      if (!property.is_string()) {
        errorMsg = "Value of 'property' is not a string";
        return nullptr;
      }
      auto expr = std::make_unique<cql2_expr_node>(property.get<std::string>());
      expr->m_node_type = CQL2_NT_COLUMN;
      return expr;
    } else if (j.contains("type") && j["type"].is_string() &&
               isGeoJSONType(j["type"].get<std::string>())) {
      OGRGeometryH hGeom = OGR_G_CreateGeometryFromJson(j.dump().c_str());
      if (!hGeom) {
        errorMsg = "Invalid GeoJSON geometry";
        return nullptr;
      }
      char *pszWkt = nullptr;
      OGR_G_ExportToIsoWkt(hGeom, &pszWkt);
      OGR_G_DestroyGeometry(hGeom);
      if (!pszWkt)
        return nullptr;
      auto expr = std::make_unique<cql2_expr_node>(pszWkt);
      CPLFree(pszWkt);
      expr->m_field_type = CQL2_WKT;
      return expr;
    } else if (j.contains("bbox")) {
      const auto bbox = j["bbox"];
      if (!bbox.is_array() || (bbox.size() != 4 && bbox.size() != 6) ||
          !bbox[0].is_number() || !bbox[1].is_number() ||
          !bbox[2].is_number() || !bbox[3].is_number() ||
          !(bbox.size() == 4 ||
            (bbox.size() == 6 && bbox[4].is_number() && bbox[5].is_number()))) {
        errorMsg = "Value of bbox is not array of 4 or 6 numeric values";
        return nullptr;
      }
      auto expr = std::make_unique<cql2_expr_node>(CQL2_CUSTOM_FUNC);
      expr->m_osVal = "BBOX";
      if (bbox.size() == 4) {
        for (int i = 0; i < 4; ++i) {
          expr->m_apoChildren.push_back(
              std::make_unique<cql2_expr_node>(bbox[i].get<double>()));
        }
      } else {
        expr->m_apoChildren.push_back(
            std::make_unique<cql2_expr_node>(bbox[0].get<double>()));
        expr->m_apoChildren.push_back(
            std::make_unique<cql2_expr_node>(bbox[1].get<double>()));
        // skip minz
        expr->m_apoChildren.push_back(
            std::make_unique<cql2_expr_node>(bbox[3].get<double>()));
        expr->m_apoChildren.push_back(
            std::make_unique<cql2_expr_node>(bbox[4].get<double>()));
      }
      return expr;
    } else if (j.contains("date")) {
      const auto date = j["date"];
      if (!date.is_string()) {
        errorMsg = "Value of 'date' is not a string";
        return nullptr;
      }
      auto expr = std::make_unique<cql2_expr_node>(CQL2_CUSTOM_FUNC);
      expr->m_osVal = "DATE";
      expr->m_apoChildren.push_back(
          std::make_unique<cql2_expr_node>(date.get<std::string>()));
      return expr;
    } else if (j.contains("timestamp")) {
      const auto timestamp = j["timestamp"];
      if (!timestamp.is_string()) {
        errorMsg = "Value of 'timestamp' is not a string";
        return nullptr;
      }
      auto expr = std::make_unique<cql2_expr_node>(CQL2_CUSTOM_FUNC);
      expr->m_osVal = "TIMESTAMP";
      expr->m_apoChildren.push_back(
          std::make_unique<cql2_expr_node>(timestamp.get<std::string>()));
      return expr;
    } else {
      errorMsg = "Object has no 'op', 'property', 'type', 'bbox', 'date' or "
                 "'timestamp' member";
    }
  } else if (j.is_string()) {
    return std::make_unique<cql2_expr_node>(j.get<std::string>());
  } else if (j.is_number_integer()) {
    return std::make_unique<cql2_expr_node>(j.get<int>());
  } else if (j.is_number()) {
    return std::make_unique<cql2_expr_node>(j.get<double>());
  } else if (j.is_boolean()) {
    return std::make_unique<cql2_expr_node>(j.get<bool>());
  } else {
    errorMsg = "Unexpected JSON type";
  }
  return nullptr;
}

/************************************************************************/
/*                           ParseOperator()                            */
/************************************************************************/

static std::unique_ptr<cql2_expr_node> ParseOperator(const json &j,
                                                     std::string &errorMsg) {
  if (!j.is_object()) {
    errorMsg = "Not a dictionnary";
    return nullptr;
  }

  if (!j.contains("op")) {
    errorMsg = "Missing \"op\" member";
    return nullptr;
  }
  const auto jOp = j["op"];
  if (!jOp.is_string()) {
    errorMsg = "Value of \"op\" member is not a string";
    return nullptr;
  }
  const auto op = jOp.get<std::string>();

  if (!j.contains("args")) {
    errorMsg = "Missing \"args\" member";
    return nullptr;
  }
  const auto jArgs = j["args"];
  if (!jArgs.is_array()) {
    errorMsg = "Value of \"args\" member is not an array";
    return nullptr;
  }

  if (op == "in") {
    if (jArgs.size() != 2) {
      errorMsg = "'" + op + "' operator expects 2 arguments";
      return nullptr;
    }
    auto firstArg = ParseObject(jArgs[0], errorMsg);
    if (!firstArg)
      return nullptr;
    if (!jArgs[1].is_array()) {
      errorMsg = "Second argument of 'in' operator must be an array";
      return nullptr;
    }
    auto expr = std::make_unique<cql2_expr_node>(CQL2_IN);
    expr->m_apoChildren.push_back(std::move(firstArg));
    for (const auto &jArg : jArgs[1]) {
      auto arg = ParseObject(jArg, errorMsg);
      if (!arg)
        return nullptr;
      expr->m_apoChildren.push_back(std::move(arg));
    }
    return expr;
  }

  std::vector<std::unique_ptr<cql2_expr_node>> args;
  bool caseInsensitive = false;
  for (const auto &jArgIn : jArgs) {
    json jArg = jArgIn;
    if (jArg.is_object() && jArg.contains("op") && jArg["op"].is_string() &&
        jArg["op"].get<std::string>() == "casei" && jArg.contains("args") &&
        jArg["args"].is_array() && jArg["args"].size() == 1) {
      jArg = jArg["args"][0];
      caseInsensitive = true;
    }
    auto arg = ParseObject(jArg, errorMsg);
    if (!arg)
      return nullptr;
    args.push_back(std::move(arg));
  }

  static const struct {
    const char *op_name;
    cql2_op op;
  } binaryOperators[] = {
      {"and", CQL2_AND},
      {"or", CQL2_OR},

      {"=", CQL2_EQ},
      {"<>", CQL2_NE},
      {"<", CQL2_LT},
      {">", CQL2_GT},
      {"<=", CQL2_LE},
      {">=", CQL2_GE},

      {"+", CQL2_ADD},
      {"-", CQL2_SUBTRACT},
      {"*", CQL2_MULTIPLY},
      {"/", CQL2_DIV},
      {"div", CQL2_DIV},
      {"%", CQL2_MOD},

      {"like", CQL2_LIKE},

      {"s_intersects", CQL2_S_INTERSECTS},
      {"s_equals", CQL2_S_EQUALS},
      {"s_disjoint", CQL2_S_DISJOINT},
      {"s_touches", CQL2_S_TOUCHES},
      {"s_within", CQL2_S_WITHIN},
      {"s_overlaps", CQL2_S_OVERLAPS},
      {"s_crosses", CQL2_S_CROSSES},
      {"s_contains", CQL2_S_CONTAINS},
  };

  for (const auto &opIter : binaryOperators) {
    if (op == opIter.op_name) {
      if (args.size() != 2) {
        errorMsg = "'" + op + "' operator expects 2 arguments";
        return nullptr;
      }
      auto cql2Op = opIter.op;
      if (caseInsensitive) {
        if (cql2Op == CQL2_EQ) {
          cql2Op = CQL2_IEQ;
        } else if (cql2Op == CQL2_NE) {
          cql2Op = CQL2_INE;
        } else if (cql2Op == CQL2_LIKE) {
          cql2Op = CQL2_ILIKE;
        } else {
          errorMsg = "casei is only supported for '=', '<>' and 'like'";
          return nullptr;
        }
      }
      auto expr = std::make_unique<cql2_expr_node>(cql2Op);
      expr->m_apoChildren = std::move(args);
      return expr;
    }
  }

  if (op == "not") {
    if (args.size() != 1) {
      errorMsg = "'" + op + "' operator expects one argument";
      return nullptr;
    }
    auto expr = std::make_unique<cql2_expr_node>(CQL2_NOT);
    expr->m_apoChildren = std::move(args);
    return expr;
  }

  if (op == "between") {
    if (args.size() != 3) {
      errorMsg = "'" + op + "' operator expects 3 arguments";
      return nullptr;
    }
    auto expr = std::make_unique<cql2_expr_node>(CQL2_BETWEEN);
    expr->m_apoChildren = std::move(args);
    return expr;
  }

  errorMsg = "Unhandled operator: ";
  errorMsg += op;
  return nullptr;
}

/************************************************************************/
/*                           CQL2JSONParse()                            */
/************************************************************************/

std::unique_ptr<cql2_expr_node> CQL2JSONParse(const char *pszInput,
                                              std::string &errorMsg) {
  try {
    return ParseOperator(json::parse(pszInput), errorMsg);
  } catch (const std::exception &e) {
    errorMsg = "Exception while parsing CQL2 JSON: ";
    errorMsg += e.what();
  }
  return nullptr;
}
