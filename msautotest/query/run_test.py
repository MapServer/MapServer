#!/usr/bin/env python
###############################################################################
# $Id: run_test.py 4851 2005-09-22 18:29:07Z frank $
#
# Project:  MapServer
# Purpose:  Test harnass script for MapServer autotest.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################
# 
# $Log$
# Revision 1.1  2005/09/22 18:28:43  frank
# New
#
# Revision 1.5  2003/03/05 15:28:50  frank
# use shared mstestlib.py
#
# Revision 1.4  2003/03/02 19:54:03  frank
# auto create result directory if missing
#
# Revision 1.3  2003/01/23 22:47:50  frank
# removed python2.2 use of st_size
#
# Revision 1.2  2002/12/21 21:47:20  frank
# preserved failed results
#
# Revision 1.1  2002/11/22 21:13:19  frank
# New
#

import sys

sys.path.append( '../pymod' )

import mstestlib


###############################################################################
# main()

if __name__ == '__main__':
    mstestlib.run_tests( sys.argv[1:] )
    


