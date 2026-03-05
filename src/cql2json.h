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

#ifndef CQL2JSON_H_INCLUDED
#define CQL2JSON_H_INCLUDED

#include "cql2.h"

std::unique_ptr<cql2_expr_node> CQL2JSONParse(const char *pszInput,
                                              std::string &osError);

#endif
