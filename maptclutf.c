/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implementation of msUTF8ToUniChar()
 * Author:   Daniel Morissette, Thomas Bonfort
 *
 * Note:
 * The source code of Tcl_UtfToUniChar() was borrowed from tclUtf.c
 * from the Tcl/Tk project.
 *
 * Website: http://www.tcl.tk/software/tcltk/
 * Source download: http://prdownloads.sourceforge.net/tcl/tcl8.4.15-src.tar.gz
 *
 * See copyright and license terms below the standard MapServer license.
 *
 ******************************************************************************
 * Copyright (c) 1996-2007 Regents of the University of Minnesota.
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
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

/*
 * tclUtf.c --
 *
 * Routines for manipulating UTF-8 strings.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState
 * Corporation and other parties.  The following terms apply to all files
 * associated with the software unless explicitly disclaimed in
 * individual files.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 * FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 * DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 * IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 * NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense, the
 * software shall be classified as "Commercial Computer Software" and the
 * Government shall have only "Restricted Rights" as defined in Clause
 * 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 * authors grant the U.S. Government and others acting in its behalf
 * permission to use and distribute the software in accordance with the
 * terms specified in this license.
 */


#include "mapserver.h"




/* The source code of Tcl_UtfToUniChar() was borrowed from tclUtf.c
 * from the Tcl/Tk project:
 * Website:
 *   http://www.tcl.tk/software/tcltk/
 * Source download:
 *   http://prdownloads.sourceforge.net/tcl/tcl8.4.15-src.tar.gz
 * Original License info follows below.
 */

/*
 * tclUtf.c --
 *
 *  Routines for manipulating UTF-8 strings.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Id: tclUtf.c,v 1.30.2.3 2005/09/07 14:35:56 dgp Exp
 */

/******************* Tcl license.terms *********************

This software is copyrighted by the Regents of the University of
California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState
Corporation and other parties.  The following terms apply to all files
associated with the software unless explicitly disclaimed in
individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights"
in the software and related documentation as defined in the Federal
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
are acquiring the software on behalf of the Department of Defense, the
software shall be classified as "Commercial Computer Software" and the
Government shall have only "Restricted Rights" as defined in Clause
252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license.

***********************************************************/


#define TCL_UTF_MAX 6


/*
 * The following structures are used when mapping between Unicode (UCS-2)
 * and UTF-8.
 */

static const unsigned char totalBytes[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
#if TCL_UTF_MAX > 3
  4,4,4,4,4,4,4,4,
#else
  1,1,1,1,1,1,1,1,
#endif
#if TCL_UTF_MAX > 4
  5,5,5,5,
#else
  1,1,1,1,
#endif
#if TCL_UTF_MAX > 5
  6,6,6,6
#else
  1,1,1,1
#endif
};

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfToUniChar --
 *
 *  Extract the Tcl_UniChar represented by the UTF-8 string.  Bad
 *  UTF-8 sequences are converted to valid Tcl_UniChars and processing
 *  continues.  Equivalent to Plan 9 chartorune().
 *
 *  The caller must ensure that the source buffer is long enough that
 *  this routine does not run off the end and dereference non-existent
 *  memory looking for trail bytes.  If the source buffer is known to
 *  be '\0' terminated, this cannot happen.  Otherwise, the caller
 *  should call Tcl_UtfCharComplete() before calling this routine to
 *  ensure that enough bytes remain in the string.
 *
 * Results:
 *  *chPtr is filled with the Tcl_UniChar, and the return value is the
 *  number of bytes from the UTF-8 string that were consumed.
 *
 * Side effects:
 *  None.
 *
 *---------------------------------------------------------------------------
 */

int ms_Tcl_UtfToUniChar(register const char *str, register unsigned int *chPtr)
{
  register int byte;


  /*
   * Unroll 1 to 3 byte UTF-8 sequences, use loop to handle longer ones.
   */

  byte = *((unsigned char *) str);
  if (byte < 0xC0) {
    /*
     * Handles properly formed UTF-8 characters between 0x01 and 0x7F.
     * Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
     * characters representing themselves.
     */

    *chPtr = byte;
    return 1;
  } else if (byte < 0xE0) {
    if ((str[1] & 0xC0) == 0x80) {
      /*
       * Two-byte-character lead-byte followed by a trail-byte.
       */

      *chPtr = (((byte & 0x1F) << 6) | (str[1] & 0x3F));
      return 2;
    }
    /*
     * A two-byte-character lead-byte not followed by trail-byte
     * represents itself.
     */

    *chPtr = byte;
    return 1;
  } else if (byte < 0xF0) {
    if (((str[1] & 0xC0) == 0x80) && ((str[2] & 0xC0) == 0x80)) {
      /*
       * Three-byte-character lead byte followed by two trail bytes.
       */

      *chPtr = (((byte & 0x0F) << 12)
                              | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
      return 3;
    }
    /*
     * A three-byte-character lead-byte not followed by two trail-bytes
     * represents itself.
     */

    *chPtr = byte;
    return 1;
  }
#if TCL_UTF_MAX > 3
  else {
    int ch, total, trail;

    total = totalBytes[byte];
    trail = total - 1;
    if (trail > 0) {
      ch = byte & (0x3F >> trail);
      do {
        str++;
        if ((*str & 0xC0) != 0x80) {
          *chPtr = byte;
          return 1;
        }
        ch <<= 6;
        ch |= (*str & 0x3F);
        trail--;
      } while (trail > 0);
      *chPtr = ch;
      return total;
    }
  }
#endif

  *chPtr = byte;
  return 1;
}


/* msUTF8ToUniChar()
 *
 *  Extract the Unicode Char represented by the UTF-8 string.  Bad
 *  UTF-8 sequences are converted to valid Unicode Chars and processing
 *  continues.
 *
 *  The caller must ensure that the source buffer is long enough that
 *  this routine does not run off the end and dereference non-existent
 *  memory looking for trail bytes.  If the source buffer is known to
 *  be '\0' terminated, this cannot happen.
 *
 * Results:
 *  *chPtr is filled with the Unicode Char value, and the return value
 *  is the number of bytes from the UTF-8 string that were consumed.
**
**/
int msUTF8ToUniChar(const char *str, /* The UTF-8 string. */
                    unsigned int *chPtr)      /* Filled with the Unicode Char represented
                                      * by the UTF-8 string. */
{
  /*check if the string is an html entity (eg &#123; or &#x12a;)*/
  int entitylgth;
  if(*str == '&' && (entitylgth=msGetUnicodeEntity(str, chPtr))>0)
    return entitylgth;
  return ms_Tcl_UtfToUniChar(str, chPtr);
}


