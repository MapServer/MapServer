###############################################################################
# $Id$
#
# Project:  MapServer
# Purpose:  Test harnass for MapServer autotest.
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

import sys
import os
import string
import time
from testlib import *

have_pdiff = None

###############################################################################
# get_mapfile_list()

def get_mapfile_list( argv ):

    map_files = []

    # use mapfile(s) passed in arg list if any.
    for arg in argv:
        if arg[-4:] == '.map':
            map_files.append( arg )

    if len(map_files) > 0:
        return map_files
    
    # scan the current directory for mapfiles. 
    files = os.listdir('.')

    for file in files:
        if file[-4:] == '.map':
            map_files.append( file )

    return map_files

###############################################################################
# has_requires()

def has_requires( version_info, requires_list ):

    for item in requires_list:
        if version_info.find( item ) == -1:
            return 0
        
    return 1

###############################################################################
# read_test_directives()

def read_test_directives( mapfile_name ):

    runparms_list = []
    require_list = []
    
    lines = open(mapfile_name).readlines()
    for line in lines:
        req_off = line.find( 'REQUIRES:' )
        if req_off != -1:
            items = line[req_off+9:].split(  )
            for item in items:
                require_list.append( item )
                
        run_off = line.find( 'RUN_PARMS:' )
        if run_off != -1:
            items = line[run_off+10:].split( None, 1 )
            if len(items) == 2:
                runparms_list.append( (items[0], items[1]) )
            elif len(items) == 1:
                runparms_list.append( (items[0],
                                       '[SHP2IMG] [RENDERER] -m [MAPFILE] -o [RESULT]') )
                                     
    if len(runparms_list) == 0:
        runparms_list.append( (mapfile_name[:-4] + '.png',
                               '[SHP2IMG] [RENDERER] -m [MAPFILE] -o [RESULT]') )

    return (runparms_list, require_list)
            

###############################################################################
# Strip Content-type and other http headers off this file.

def demime_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    for i in range(len(data)-1):
        if data[i] == '\r' and data[i+1] == '\n' and data[i+2] == '\r' and data[i+3] == '\n':
            if version_info >= (3,0,0):
                open(filename,'wb').write(bytes(data[i+4:], 'iso-8859-1'))
            else:
                open(filename,'wb').write(data[i+4:])
            return
    return

###############################################################################
# Strip MapServer version comment from file.

def deversion_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    start = data.find( '<!-- MapServer version' )
    if start == -1:
        start = data.find( '<!--MapServer version' )
    if start == -1:
        return

    end = start+10
    length = len(data)
    while end < length - 5 and data[end:end+3] != '-->':
        end = end+1

    if data[end:end+3] != '-->':
        return

    new_data = data[:start-1] + data[end+4:]
    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(new_data, 'iso-8859-1'))
    else:
        open(filename,'wb').write(new_data)
    return

###############################################################################
# white out timestamp

def detimestamp_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    start = data.find( 'timeStamp="' )
    if start == -1:
        return

    start = start + 11
    end = start
    while data[end+1] != '"':
        end = end + 1

    new_data = data[:start] + data[end+1:]
    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(new_data, 'iso-8859-1'))
    else:
        open(filename,'wb').write(new_data)
    return

###############################################################################
# Collect all the [STRIP:] directives from a command string and remove them
# from the command string.

def collect_strip_requests( command ):

    strip_items = []

    while command.find('[STRIP:') != -1:

        dir_start = command.find('[STRIP:')
        pat_start = dir_start + 7
        pat_end = pat_start+1

        while command[pat_end] != ']':
            pat_end = pat_end+1

        strip_items.append( command[pat_start:pat_end] )

        command = command[:dir_start] + command[pat_end+1:]

    return (command, strip_items)
        
###############################################################################
# Strip lines matching substrings.

def apply_strip_items_file( filename, strip_items ):

    if len(strip_items) == 0:
        return
    
    from sys import version_info

    data_lines = open(filename,'rb').readlines()
    out_data = ''
    for i in range(len(data_lines)):
        data_line = data_lines[i]
        if version_info >= (3,0,0):
            data_line = str(data_line, 'iso-8859-1')

        discard = 0
        for item in strip_items:
            if data_line.find( item ) != -1:
                discard = 1
        if discard == 0:
            out_data += data_line
        else:
            out_data += '[stripped line matching "%s"]\n' % item 

    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(out_data, 'iso-8859-1'))
    else:
        open(filename,'wb').write(out_data)

    return

###############################################################################
# Do windows exponential conversion on the file (e+0nn to e+nn).

def fixexponent_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    orig_data = data

    start = data.find( 'e+0' )
    while start != -1:
        if data[start+3] in string.digits and data[start+4] in string.digits \
            and data[start+5] == '"':
            data = data[:start+2] + data[start+3:]
        start = data.find( 'e+0', start+3 )

    if data != orig_data:
        if version_info >= (3,0,0):
            open(filename,'wb').write(bytes(data, 'iso-8859-1'))
        else:
            open(filename,'wb').write(data)

    return

###############################################################################
# Do windows number of decimal truncation.

def truncate_one_decimal( filename ):
    import re
    
    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    numbers_found = re.compile('[0-9]+\.[0-9]{6,24}', re.M)

    start = 0
    new_data = ''
    for number in numbers_found.finditer(data):
        end = number.end() - 1
        new_data = new_data + data[start:end] 
        start = number.end()

    if new_data != '':
        new_data = new_data + data[start:] 


    if new_data != '' and new_data != data:
        if version_info >= (3,0,0):
            open(filename,'wb').write(bytes(new_data, 'iso-8859-1'))
        else:
            open(filename,'wb').write(new_data)

    return
###############################################################################
# Replace CR+LF by CR

def crlf( filename ):
    
    try:
        file_stat = os.stat( filename )
    except OSError:
        return
    data = open(filename, "rb").read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    #This is a binary file
    if '\0' in data:
        return
      
    newdata = data.replace("\r\n", "\n")
    if newdata != data:
        f = open(filename, "wb")
        if version_info >= (3,0,0):
            f.write(bytes(newdata, 'iso-8859-1'))
        else:
            f.write(newdata)
        f.close()
###############################################################################
# run_tests()

def run_tests( argv ):

    skip_count = 0
    fail_count = 0
    succeed_count = 0
    init_count = 0
    noresult_count = 0
    keep_pass = 0
    valgrind = 0 
    valgrind_log = ''
    shp2img = 'shp2img'
    renderer = None
    verbose = 0
    strict = 0
    quiet = 0
    skiparg = False
    ###########################################################################
    # Process arguments.
    
    for i in range(len(argv)):
        if skiparg:
            skiparg = False
            continue
        if argv[i] == '-shp2img':
            shp2img = argv[i+1]
            skiparg = True
        elif argv[i] == '-keep':
            keep_pass = 1
        elif argv[i] == '-valgrind':
            valgrind = 1
        elif argv[i] == '-strict':
            strict = 1
        elif argv[i] == '-renderer':
            renderer = argv[i+1]
            skiparg = True
        elif argv[i] == '-v':
            verbose = 1
        elif argv[i] == '-q':
            quiet = 1
        elif argv[i][-4:] == '.map':
            pass
        else:
            print( 'Unrecognised argument: %s' % argv[i] )
            print( 'Usage: run_test.py [-v] [-keep] [-valgrind] [-strict]\n' + 
                   '                   [-shp2img <file>] [-renderer <name>]\n' +
                   '                   [mapfilename]*' )
            sys.exit( 1 )

    ###########################################################################
    # Create results directory if it does not already exist.
    if not os.path.exists("result"):
         os.mkdir("result")

    ###########################################################################
    # Get version info.
    version_info = os.popen( shp2img + ' -v' ).read()
    print('version = %s' % version_info)
    
    ###########################################################################
    # Check directory wide requirements.
    try:
        (runparms_list, requires_list) = read_test_directives( 'all_require.txt' )
        if not has_requires( version_info, requires_list ):
            print('Some or all of the following requirements for this directory of tests\nare not available:')
            print(requires_list)
            return
    except:
        pass
    
    ###########################################################################
    # Process all mapfiles.
    map_files = get_mapfile_list( argv )

    for map in map_files:

        if not quiet:
           print(' Processing: %s' % map)
        (runparms_list, requires_list) = read_test_directives( map )
        for i in range(len(runparms_list)):
            if renderer is not None:
                (resultbase,resultext) = os.path.splitext(runparms_list[i][0])
                if renderer in ( 'pdf', 'svg', 'gif'):
                   runparms_list[i] = ("%s.%s"%(resultbase,renderer),runparms_list[i][1])
                else:
                   runparms_list[i] = ("%s.%s%s"%(resultbase,renderer,resultext),runparms_list[i][1])

        if not has_requires( version_info, requires_list ):
            if not quiet:
                print('    missing some or all of required components, skip.')
            else:
                print('%s: missing some or all of required components, skip.'%(map))
            skip_count += len(runparms_list)
            continue
        
        #######################################################################
        # Handle each RUN_PARMS item in this file.
        for run_item in runparms_list:
            time.sleep(0.05)  #allow us to catch a ctrl-c
            out_file = run_item[0]
            command = run_item[1]

            if len(runparms_list) > 1 and not quiet:
                print('   test %s' % out_file)

            if command.find('[RESULT_DEMIME]') != -1:
                demime = 1
            else:
                demime = 0
                
            if command.find('[RESULT_DEVERSION]') != -1:
                deversion = 1
            else:
                deversion = 0

            command = command.replace('[RESULT]', 'result/'+out_file )
            command = command.replace('[RESULT_DEMIME]', 'result/'+out_file )
            command = command.replace('[RESULT_DEVERSION]', 'result/'+out_file )
            command = command.replace('[MAPFILE]', map )
            command = command.replace('[SHP2IMG]', shp2img )
            if renderer is not None:
                command = command.replace('[RENDERER]', '-i '+renderer )
            else:
                command = command.replace('[RENDERER]', '' )

            # support for POST request method
            begin = command.find('[POST]')
            end = command.find('[/POST]')
            post = ''
            if begin != -1 and end != -1 and begin < end:
                post = command[begin+len('[POST]'):end]
                tmp = command
                post = post.replace( '"', '\'')
                if valgrind:
                    command = tmp[:begin] + tmp[end+len('[/POST]'):]
                else:
                    command = 'echo "' + post + '" | ' + tmp[:begin] + tmp[end+len('[/POST]'):]
                os.environ['CONTENT_LENGTH'] = str(len(post))
                os.environ['REQUEST_METHOD'] = "POST"
                os.environ['MS_MAPFILE'] = map
                if post[0] == '<':
                  os.environ['CONTENT_TYPE'] = 'text/xml'
                else:
                  os.environ['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'

            command = command.replace('[MAPSERV]', 'mapserv' )
            command = command.replace('[LEGEND]', 'legend' )
            command = command.replace('[SCALEBAR]', 'scalebar' )

            (command, strip_items) = collect_strip_requests( command )
            
            if valgrind:
                valgrind_log = 'result/%s.txt'%(out_file+".vgrind.txt")
                command = command.strip()
                if post == '':
                  command = 'valgrind --tool=memcheck -q --suppressions=../valgrind-suppressions.txt --leak-check=full --show-reachable=yes %s 2>%s'%(command, valgrind_log)
                else:
                  command = 'echo "' + post + '" | valgrind --tool=memcheck -q --suppressions=../valgrind-suppressions.txt --leak-check=full --show-reachable=yes %s 2>%s'%(command, valgrind_log)

            if verbose:
                print('')
                print( command )
                
            os.system( command )

            if begin != -1 and end != -1 and begin < end:
                del os.environ['CONTENT_LENGTH']
                del os.environ['REQUEST_METHOD']
                del os.environ['MS_MAPFILE']

            if demime:
                demime_file( 'result/'+out_file )
            if deversion:
                deversion_file( 'result/'+out_file )
                fixexponent_file( 'result/'+out_file )
                truncate_one_decimal( 'result/'+out_file )
                detimestamp_file( 'result/'+out_file )
            if valgrind:
                if os.path.getsize(valgrind_log) == 0:
                   os.remove( valgrind_log )

            apply_strip_items_file( 'result/'+out_file, strip_items )
                
            crlf('result/'+out_file)
            cmp = compare_result( out_file )
            
            if cmp == 'match':
                succeed_count = succeed_count + 1
                if keep_pass == 0:
                   os.remove( 'result/' + out_file )
                if not quiet:
                   print('     results match.')
                else:
                   sys.stdout.write('.')
                   sys.stdout.flush()
            elif cmp ==  'files_differ_image_match':
                if strict:
                   fail_count = fail_count + 1
                   if not quiet:
                       print('*    results dont match (though images match), TEST FAILED.')
                   else:
                       print('%s: results dont match (though images match), TEST FAILED.'%(out_file))
                else:
                   succeed_count = succeed_count + 1
                   if keep_pass == 0:
                      os.remove( 'result/' + out_file )
                   if not quiet:
                      print('     result images match, though files differ.')
                   else:
                      sys.stdout.write('.')
                      sys.stdout.flush()
            elif cmp ==  'files_differ_image_nearly_match':
                if strict:
                   fail_count = fail_count + 1
                   if not quiet:
                       print('*    results dont match (though images perceptually match), TEST FAILED.')
                   else:
                       print('%s: results dont match (though images perceptually match), TEST FAILED.'%(out_file))
                else:
                   succeed_count = succeed_count + 1
                   if keep_pass == 0:
                      os.remove( 'result/' + out_file )
                   if not quiet:
                      print('     result images perceptually match, though files differ.')
                   else:
                      print('%s: result images perceptually match, though files differ.'%(out_file))
            elif cmp ==  'nomatch':
                fail_count = fail_count + 1
                if not quiet:
                    print('*    results dont match, TEST FAILED.')
                else:
                    print('%s: results dont match, TEST FAILED.'%(out_file))

            elif cmp == 'noresult':
                f = open('result/'+out_file,"w")
                print >>f, "Segmentation fault or other serious error"
                f.close()
                fail_count = fail_count + 1
                noresult_count += 1
                if not quiet:
                    print('*    no result file generated, TEST FAILED.')
                else:
                    print('%s: no result file generated, TEST FAILED.'%(out_file))
            elif cmp == 'noexpected':
                if not quiet:
                    print('     no expected file exists, accepting result as expected.')
                else:
                    print('%s: no expected file exists, accepting result as expected.'%(out_file))
                init_count = init_count + 1
                os.rename( 'result/' + out_file, 'expected/' + out_file )

    try:
        print('Test done (%.2f%% success):' % (float(succeed_count)/float(succeed_count+fail_count)*100))
    except:
        pass

    print('%d tested skipped' % skip_count)
    print('%d tests succeeded' % succeed_count)
    print('%d tests failed' %fail_count)
    print('%d test results initialized' % init_count)

    if noresult_count > 0:
        print('%d of failed tests produced *no* result! Serious Failure!' % noresult_count)
