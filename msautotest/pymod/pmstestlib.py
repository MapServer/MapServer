# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test harnass for testing via Python MapScript.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
#  Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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

import sys
import os
import string
from testlib import *

keep_pass = 0

success_counter = 0
failure_counter = 0
blow_counter = 0
skip_counter = 0

reason = None

post_test_msg = None

def setup_run( name ):
    global success_counter, failure_counter, blow_counter, skip_counter
    global cur_name
    
    cur_name = name

def run_tests( test_list ):
    global success_counter, failure_counter, blow_counter, skip_counter
    global cur_name, reason, post_test_msg

    for test_item in test_list:
        if test_item is None:
            continue

        try:
            (func, name) = test_item
	    sys.stdout.write( '  TEST: ' + func.__name__[4:] + ': ' + name + ' ... ' )
	except:
            func = test_item
            name = func.__name__
	    sys.stdout.write( '  TEST: ' + name + ' ... ' )

        sys.stdout.flush()
            
        reason = None
        try:
            result = func()
            print result
        except:
            result = 'blowup'
            print result
            
            import traceback
            traceback.print_exc()


        if reason is not None:
            print '    ' + reason

        if post_test_msg is not None:
            print post_test_msg
            post_test_msg = None

        if result == 'success':
            success_counter = success_counter + 1
        elif result == 'fail':
            failure_counter = failure_counter + 1
        elif result == 'skip':
            skip_counter = skip_counter + 1
        else:
            blow_counter = blow_counter + 1

def post_reason( msg ):
    global reason

    reason = msg

def summarize():
    global success_counter, failure_counter, blow_counter, skip_counter
    global cur_name
    
    print
    print 'Test Script: %s' % cur_name
    print 'Succeeded: %d' % success_counter
    print 'Failed:    %d (%d blew exceptions)' \
          % (failure_counter+blow_counter, blow_counter)
    print 'Skipped:   %d' % skip_counter
    print

    return failure_counter + blow_counter

def get_item_value( layer, shape_obj, name ):
    for i in range(layer.numitems):
        if layer.getItem(i) == name:
            return shape_obj.getValue(i)
    return None
    
def check_items( layer, shape_obj, nv_list ):
    for nv in nv_list:
        name = nv[0]
        expected_value = nv[1]

        actual_value = get_item_value( layer, shape_obj, name )
        if actual_value is None:
            post_reason( 'missing expected attribute %s' % name )
            return 0
            
        if str(expected_value) != actual_value:
            post_reason( 'attribute %s is "%s" instead of expected "%s"' % \
                         (name, actual_value, str(expected_value)) )
            return 0

    return 1

          
###############################################################################

def run_all( dirlist, option_list = [] ):

    for dir_name in dirlist:
        files = os.listdir(dir_name)

        old_path = sys.path
        sys.path.append('.')
        
        for file in files:
            if not file[-3:] == '.py':
                continue

            module = file[:-3]
            try:
                wd = os.getcwd()
                os.chdir( dir_name )
                
                xtest_list = None
                exec "import " + module

                try:
                    exec "xtest_list = " + module + ".test_list"
                except:
                    pass

                if xtest_list is not None:
                    try:
                        print 'Running tests from %s/%s' % (dir_name,file)
                        run_tests( xtest_list )
                    except:
                        import traceback
                        traceback.print_exc()
                        pass
                
                os.chdir( wd )

            except:
                os.chdir( wd )
                print '... failed to load %s ... skipping.' % file

                import traceback
                traceback.print_exc()


        # We only add the tool directory to the python path long enough
        # to load the tool files.
        sys.path = old_path


###############################################################################
#

def compare_and_report( out_file ):

    global post_test_msg
    
    cmp = compare_result( out_file )
    
    if cmp == 'match':
        if keep_pass == 0:
            os.remove( 'result/' + out_file )
        post_test_msg = '     results match.'
        return 'success'
        
    elif cmp ==  'files_differ_image_match':
        if keep_pass == 0:
            os.remove( 'result/' + out_file )
        post_test_msg = '     result images match, though files differ.'
        return 'success'
        
    elif cmp ==  'files_differ_image_nearly_match':
        if keep_pass == 0:
            os.remove( 'result/' + out_file )
        post_test_msg = '     result images perceptually match, though files differ.'
        return 'success'

    elif cmp ==  'nomatch':
        post_test_msg = '*    results dont match, TEST FAILED.'
        return 'fail'
        
    elif cmp == 'noresult':
        post_test_msg = '*    no result file generated, TEST FAILED.'
        return 'fail'
    
    elif cmp == 'noexpected':
        post_test_msg = '     no expected file exists, accepting result as expected.'
        os.rename( 'result/' + out_file, 'expected/' + out_file )
        return 'skip'

    else:
        post_test_msg = 'unexpected cmp value: ' + cmp
        return 'fail'
    
    
