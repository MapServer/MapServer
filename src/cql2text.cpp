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

#include "cql2text.h"
#include "cql2textparser.hpp"
#include "mapserver.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

/************************************************************************/
/*                            CQL2TextParse()                           */
/************************************************************************/

std::unique_ptr<cql2_expr_node> CQL2TextParse(const char *pszInput,
                                              std::string &osError) {
  cql2text_parse_context context;

  context.pszInput = pszInput;
  context.pszNext = pszInput;
  context.pszLastValid = pszInput;
  context.nStartToken = CQL2_TOK_VALUE_START;

  if (cql2textparse(&context) == 0) {
    context.poRoot->RebalanceAndOr();
    return std::unique_ptr<cql2_expr_node>(context.poRoot);
  } else {
    delete context.poRoot;
    osError = std::move(context.osError);
    return nullptr;
  }
}

/************************************************************************/
/*                           cql2texterror()                            */
/************************************************************************/

void cql2texterror(cql2text_parse_context *context, const char *msg) {
  std::string osMsg("CQL2Text Expression Parsing Error: ");
  osMsg += msg;
  osMsg += ". Occurred around :\n";

  int n = static_cast<int>(context->pszLastValid - context->pszInput);

  for (int i = std::max(0, n - 40); i < n + 40 && context->pszInput[i] != '\0';
       i++)
    osMsg += context->pszInput[i];
  osMsg += "\n";
  for (int i = 0; i < std::min(n, 40); i++)
    osMsg += " ";
  osMsg += "^";

  context->osError = std::move(osMsg);
}

/************************************************************************/
/*                               swqlex()                               */
/*                                                                      */
/*      Read back a token from the input.                               */
/************************************************************************/

int cql2textlex(cql2_expr_node **ppNode, cql2text_parse_context *context) {
  const char *pszInput = context->pszNext;

  *ppNode = nullptr;

  /* -------------------------------------------------------------------- */
  /*      Do we have a start symbol to return?                            */
  /* -------------------------------------------------------------------- */
  if (context->nStartToken != 0) {
    int nRet = context->nStartToken;
    context->nStartToken = 0;
    return nRet;
  }

  /* -------------------------------------------------------------------- */
  /*      Skip white space.                                               */
  /* -------------------------------------------------------------------- */
  while (*pszInput == ' ' || *pszInput == '\t' || *pszInput == 10 ||
         *pszInput == 13)
    pszInput++;

  context->pszLastValid = pszInput;

  if (*pszInput == '\0') {
    context->pszNext = pszInput;
    return EOF;
  }

  /* -------------------------------------------------------------------- */
  /*      Handle string constants.                                        */
  /* -------------------------------------------------------------------- */
  if (*pszInput == '"' || *pszInput == '\'') {
    char chQuote = *pszInput;
    bool bFoundEndQuote = false;

    int nRet = *pszInput == '"' ? CQL2_TOK_IDENTIFIER : CQL2_TOK_STRING;

    pszInput++;

    std::string token;

    while (*pszInput != '\0') {
      if (*pszInput == '\\' && pszInput[1] != 0)
        pszInput++;
      else if (*pszInput == '\'' && pszInput[1] == chQuote)
        pszInput++;
      else if (*pszInput == chQuote) {
        pszInput++;
        bFoundEndQuote = true;
        break;
      }

      token += *(pszInput++);
    }

    if (!bFoundEndQuote) {
      context->osError = "Did not find end-of-string character";
      return YYerror;
    }

    *ppNode = new cql2_expr_node(token);

    context->pszNext = pszInput;

    return nRet;
  }

  /* -------------------------------------------------------------------- */
  /*      Handle numbers.                                                 */
  /* -------------------------------------------------------------------- */
  else if (*pszInput >= '0' && *pszInput <= '9') {
    std::string osToken;

    osToken += *pszInput;

    // collect non-decimal part of number
    const char *pszNext = pszInput + 1;
    while (*pszNext >= '0' && *pszNext <= '9')
      osToken += *(pszNext++);

    // collect decimal places.
    if (*pszNext == '.') {
      osToken += *(pszNext++);
      while (*pszNext >= '0' && *pszNext <= '9')
        osToken += *(pszNext++);
    }

    // collect exponent
    if (*pszNext == 'e' || *pszNext == 'E') {
      osToken += *(pszNext++);
      if (*pszNext == '-' || *pszNext == '+')
        osToken += *(pszNext++);
      while (*pszNext >= '0' && *pszNext <= '9')
        osToken += *(pszNext++);
    }

    context->pszNext = pszNext;

    if (strstr(osToken.c_str(), ".") || strstr(osToken.c_str(), "e") ||
        strstr(osToken.c_str(), "E")) {
      *ppNode = new cql2_expr_node(atof(osToken.c_str()));
      return CQL2_TOK_DOUBLE_NUMBER;
    } else {
      *ppNode = new cql2_expr_node(atoi(osToken.c_str()));
      return CQL2_TOK_INTEGER_NUMBER;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Handle alpha-numerics.                                          */
  /* -------------------------------------------------------------------- */
  else if (isalnum(static_cast<unsigned char>(*pszInput))) {
    int nReturn = CQL2_TOK_IDENTIFIER;
    std::string osToken;
    const char *pszNext = pszInput + 1;

    osToken += *pszInput;

    // collect text characters
    while (isalnum(static_cast<unsigned char>(*pszNext)) || *pszNext == '_' ||
           static_cast<unsigned char>(*pszNext) > 127)
      osToken += *(pszNext++);

    context->pszNext = pszNext;

    if (osToken == "true") {
      *ppNode = new cql2_expr_node(true);
      nReturn = CQL2_TOK_BOOLEAN_LITERAL;
    } else if (osToken == "false") {
      *ppNode = new cql2_expr_node(false);
      nReturn = CQL2_TOK_BOOLEAN_LITERAL;
    } else if (EQUAL(osToken.c_str(), "OR"))
      nReturn = CQL2_TOK_OR;
    else if (EQUAL(osToken.c_str(), "AND"))
      nReturn = CQL2_TOK_AND;
    else if (EQUAL(osToken.c_str(), "NOT"))
      nReturn = CQL2_TOK_NOT;
    else if (EQUAL(osToken.c_str(), "LIKE"))
      nReturn = CQL2_TOK_LIKE;
    else if (EQUAL(osToken.c_str(), "IN"))
      nReturn = CQL2_TOK_IN;
    else if (EQUAL(osToken.c_str(), "BETWEEN"))
      nReturn = CQL2_TOK_BETWEEN;
    else if (EQUAL(osToken.c_str(), "IS"))
      nReturn = CQL2_TOK_IS;
    else if (EQUAL(osToken.c_str(), "NULL"))
      nReturn = CQL2_TOK_NULL;
    else if (EQUAL(osToken.c_str(), "CASEI"))
      nReturn = CQL2_TOK_CASEI;
    else if (EQUAL(osToken.c_str(), "DIV"))
      nReturn = CQL2_TOK_DIV;
    else if (EQUAL(osToken.c_str(), "S_INTERSECTS"))
      nReturn = CQL2_TOK_S_INTERSECTS;
    else if (EQUAL(osToken.c_str(), "S_EQUALS"))
      nReturn = CQL2_TOK_S_EQUALS;
    else if (EQUAL(osToken.c_str(), "S_DISJOINT"))
      nReturn = CQL2_TOK_S_DISJOINT;
    else if (EQUAL(osToken.c_str(), "S_TOUCHES"))
      nReturn = CQL2_TOK_S_TOUCHES;
    else if (EQUAL(osToken.c_str(), "S_WITHIN"))
      nReturn = CQL2_TOK_S_WITHIN;
    else if (EQUAL(osToken.c_str(), "S_OVERLAPS"))
      nReturn = CQL2_TOK_S_OVERLAPS;
    else if (EQUAL(osToken.c_str(), "S_CROSSES"))
      nReturn = CQL2_TOK_S_CROSSES;
    else if (EQUAL(osToken.c_str(), "S_CONTAINS"))
      nReturn = CQL2_TOK_S_CONTAINS;
    else if (EQUAL(osToken.c_str(), "POINT") ||
             EQUAL(osToken.c_str(), "LINESTRING") ||
             EQUAL(osToken.c_str(), "POLYGON") ||
             EQUAL(osToken.c_str(), "MULTIPOINT") ||
             EQUAL(osToken.c_str(), "MULTILINESTRING") ||
             EQUAL(osToken.c_str(), "MULTIPOLYGON") ||
             EQUAL(osToken.c_str(), "GEOMETRYCOLLECTION")) {
      while (*pszNext == ' ')
        ++pszNext;
      if (*pszNext == '(') {
        int nLevel = 0;
        while (*pszNext) {
          osToken += *pszNext;
          if (*pszNext == '(') {
            ++nLevel;
          } else if (*pszNext == ')') {
            --nLevel;
            if (nLevel <= 0)
              break;
          }
          ++pszNext;
        }
        if (nLevel != 0) {
          context->osError = "Unbalanced or missing parenthesis in WKT";
          return YYerror;
        }
        context->pszNext = pszNext + 1;
        *ppNode = new cql2_expr_node(osToken);
        (*ppNode)->m_field_type = CQL2_WKT;
        nReturn = CQL2_TOK_WKT;
      } else {
        *ppNode = new cql2_expr_node(osToken);
        nReturn = CQL2_TOK_IDENTIFIER;
      }
    } else {
      *ppNode = new cql2_expr_node(osToken);
      nReturn = CQL2_TOK_IDENTIFIER;
    }

    return nReturn;
  }

  /* -------------------------------------------------------------------- */
  /*      Handle special tokens.                                          */
  /* -------------------------------------------------------------------- */
  else {
    context->pszNext = pszInput + 1;
    return *pszInput;
  }
}
