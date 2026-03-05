/******************************************************************************
 *
 * Project: MapServer
 * Purpose: CQL2 Text Parser
 * Author: Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (C) 2026 Even Rouault <even dot rouault at spatialys dot com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef CQL2TEXT_H_INCLUDED
#define CQL2TEXT_H_INCLUDED

#include "cql2.h"

#include <string>

struct cql2text_parse_context {
  int nStartToken = 0;
  const char *pszInput = nullptr;
  const char *pszNext = nullptr;
  const char *pszLastValid = nullptr;

  cql2_expr_node *poRoot = nullptr;
  std::string osError{};
};

std::unique_ptr<cql2_expr_node> CQL2TextParse(const char *pszInput,
                                              std::string &osError);

int cql2textparse(cql2text_parse_context *context);
int cql2textlex(cql2_expr_node **ppNode, cql2text_parse_context *context);
void cql2texterror(cql2text_parse_context *context, const char *msg);

cql2_expr_node *cql2_create_and_or_or(cql2_op op, cql2_expr_node *left,
                                      cql2_expr_node *right);

#endif
