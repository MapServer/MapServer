# -*- coding: utf-8 -*-
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
import pytest
import string
from testlib import compare_result
try:
    import xmlvalidate
    xmlvalidate_ok = True
except:
    print('Cannot import xmlvalidate. Likely lxml missing')
    xmlvalidate_ok = False
    pass

have_pdiff = None
shp2img = 'shp2img'
keep_pass = False
quiet = False
validate_xml = True
ogc_schemas_location = None

###############################################################################
# get_mapfile_list()

def get_mapfile_list( dirname ):

    # scan the current directory for mapfiles. 
    files = os.listdir(dirname)

    map_files = []
    for file in files:
        if file[-4:] == '.map':
            map_files.append( file )

    return map_files


###############################################################################
# compare_version()
# Returns 1 if a > b, 0 if a == b, -1 if a < b

def compare_version(version_a, version_b):

    a = version_a.split('.')
    b = version_b.split('.')
    while len(a) < 3:
        a += [ '0' ]
    while len(b) < 3:
        b += [ '0' ]
    a_x, a_y, a_z = int(a[0]), int(a[1]), int(a[2])
    b_x, b_y, b_z = int(b[0]), int(b[1]), int(b[2])
    if a_x > b_x:
        return 1
    if a_x < b_x:
        return -1
    if a_y > b_y:
        return 1
    if a_y < b_y:
        return -1
    if a_z > b_z:
        return 1
    if a_z < b_z:
        return -1
    return 0

###############################################################################
# has_requires()

def has_requires( version_info, gdal_version, requires_list ):

    for item in requires_list:
        if item.startswith('GDAL>='):
            if gdal_version is None:
                return 0
            if compare_version(gdal_version, item[len('GDAL>='):]) < 0:
                return 0
        elif item.startswith('GDAL=='):
            if gdal_version is None:
                return 0
            if compare_version(gdal_version, item[len('GDAL=='):]) != 0:
                return 0
        elif version_info.find( item ) == -1:
            return 0

    return 1

###############################################################################
# read_test_directives()

def read_test_directives( mapfile_name ):

    runparms_list = []
    require_list = []
    
    from sys import version_info
    if version_info >= (3,0,0):
        lines = open(mapfile_name, encoding="utf8").readlines()
    else:
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

    # remove any double CR which will crash the following code
    extension = os.path.splitext(filename)[1]
    if extension in (".xml", ".json"):
        data = data.replace("\r\r", "\r")

    offset = -1
    for i in range(len(data)-1):
        if data[i] == '\r' and data[i+1] == '\n' and data[i+2] == '\r' and data[i+3] == '\n':
            offset = 4
        elif data[i] == '\n' and data[i+1] == '\n':
            offset = 2

        if offset != -1:
            if version_info >= (3,0,0):
                open(filename,'wb').write(bytes(data[i+offset:], 'iso-8859-1'))
            else:
                open(filename,'wb').write(data[i+offset:])
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
# Strip GDAL version from file.

def degdalversion_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    # Remove GDAL version from GPX file
    start = data.find('creator="GDAL ')
    if start == -1:
        return

    end = start + 14
    length = len(data)
    while end < length and data[end] != '"':
        end = end + 1
    if data[end] != '"':
        return

    data = data[:start-1] + data[end+1:]

    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(data, 'iso-8859-1'))
    else:
        open(filename,'wb').write(data)
    return

###############################################################################
# white out timestamp

def detimestamp_file( filename ):

    data = open(filename,'rb').read()
    has_found_timestamp = False

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    start_find_idx = 0
    while True:
        start = data[start_find_idx:].find( 'timeStamp="' )
        if start == -1:
            break
        start = start + start_find_idx
        has_found_timestamp = True

        start = start + 11
        end = start
        while data[end+1] != '"':
            end = end + 1

        data = data[:start] + data[end+1:]
        start_find_idx = start + 11

    if not has_found_timestamp:
        return

    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(data, 'iso-8859-1'))
    else:
        open(filename,'wb').write(data)
    return

###############################################################################
# Keep version="" string

def extract_service_version_file( filename ):

    data = open(filename,'rb').read()

    from sys import version_info
    if version_info >= (3,0,0):
        data = str(data, 'iso-8859-1')

    # Remove GDAL version from GPX file
    start = data.find('WFS_Capabilities')
    if start == -1:
        return
    data = data[start:]
    start = data.find('version="')
    if start == -1:
        return

    end = start + len('version="')
    length = len(data)
    result = ''
    while end < length and data[end] != '"':
        result = result + data[end]
        end = end + 1
    if data[end] != '"':
        return

    if version_info >= (3,0,0):
        open(filename,'wb').write(bytes(result, 'iso-8859-1'))
    else:
        open(filename,'wb').write(result)
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
        os.stat( filename )
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
def get_gdal_version():

    # First try with GDAL Python bindings, otherwise with gdalinfo binary
    try:
        from osgeo import gdal
        gdal_version = gdal.VersionInfo('VERSION_INFO')
    except:
        gdal_version = os.popen( 'gdalinfo --version').read()

    # Parse something like "GDAL x.y.zdev, released..." to extract "x.y.z"
    if gdal_version.startswith('GDAL '):
        gdal_version = gdal_version[len('GDAL '):]
        pos = gdal_version.find('dev')
        if pos >= 0:
            return gdal_version[0:pos]
        pos = gdal_version.find(',')
        if pos >= 0:
            return gdal_version[0:pos]
    return None

###############################################################################

def get_pytests(dirname):

    ###########################################################################
    # Create results directory if it does not already exist.
    if not os.path.exists(os.path.join(dirname, "result")):
         os.mkdir(os.path.join(dirname, "result"))

    ###########################################################################
    # Get version info.
    version_info = os.popen( shp2img + ' -v' ).read()
    #print('version = %s' % version_info)

    gdal_version = get_gdal_version()
    #print('GDAL version = %s' % gdal_version)

    ###########################################################################
    # Must we and can we validate XML stuff ?
    if validate_xml and xmlvalidate_ok:
        if xmlvalidate.has_local_ogc_schemas('SCHEMAS_OPENGIS_NET'):
            global ogc_schemas_location
            ogc_schemas_location = 'SCHEMAS_OPENGIS_NET'
        else:
            print('Cannot validate XML because SCHEMAS_OPENGIS_NET not found. Run "python ../pymod/xmlvalidate.py -download_ogc_schemas" from msautotest/wxs')

    tests = []
    for map in get_mapfile_list(dirname):

        runparms_list, requires_list = read_test_directives( os.path.join(dirname, map) )

        marks = []
        if not has_requires( version_info, gdal_version, requires_list ):
            print('%s: missing some or all of required components, skip.'%(map))
            marks.append(pytest.mark.skip)

        for out_file, command in runparms_list:
            out_file = os.path.basename(out_file)
            id = map[0:-4]+'_'+out_file.replace('.','_')
            param = pytest.param(os.path.join(dirname, map), out_file, command, id=id,marks=marks)
            tests.append(param)

    return tests


###############################################################################


def _run(map, out_file, command, extra_args):

    strict = extra_args.getoption("strict_mode")
    valgrind = extra_args.getoption("valgrind")
    run_under_asan = extra_args.getoption("run_under_asan")
    verbose = extra_args.getoption("verbose") >= 2
    renderer = extra_args.getoption("renderer")
    if renderer:
        (resultbase,resultext) = os.path.splitext(out_file)
        if renderer in ( 'pdf', 'svg', 'gif'):
            out_file = "%s.%s"%(resultbase,renderer)
        else:
            out_file = "%s.%s%s"%(resultbase,renderer,resultext)

    if command.find('[RESULT_DEMIME_DEVERSION]') != -1:
        demime = 1
        deversion = 1
    else:
        if command.find('[RESULT_DEMIME]') != -1:
            demime = 1
        else:
            demime = 0

        if command.find('[RESULT_DEVERSION]') != -1:
            deversion = 1
        else:
            deversion = 0

    if command.find('[EXTRACT_SERVICE_VERSION]') != -1:
        extractserviceversion = 1
    else:
        extractserviceversion = 0

    command = command.replace('[RESULT]', 'result/'+out_file )
    command = command.replace('[RESULT_DEMIME]', 'result/'+out_file )
    command = command.replace('[RESULT_DEVERSION]', 'result/'+out_file )
    command = command.replace('[RESULT_DEMIME_DEVERSION]', 'result/'+out_file )
    command = command.replace('[EXTRACT_SERVICE_VERSION]', 'result/'+out_file )
    command = command.replace('[MAPFILE]', os.path.basename(map) )
    command = command.replace('[SHP2IMG]', shp2img )
    if renderer is not None:
        command = command.replace('[RENDERER]', '-i '+renderer )
    else:
        command = command.replace('[RENDERER]', '' )

    os.environ['MS_PDF_CREATION_DATE'] = 'dummy date'

    #support for environment variable of type [ENV foo=bar]
    begin = command.find('[ENV')
    envirkey = ''
    if begin != -1:
        end = command[begin:].find(']')
        equal = command[begin:].find('=')
        #print("equal is %d"%equal)
        envirkey = command[begin+len('[ENV '):begin+equal]
        envirval = command[equal+1:end]
        os.environ[envirkey] = envirval
        tmp = command
        command = tmp[:begin] + tmp[end+1:]
        #print('added environment variable (%s)=(%s); new command:%s' % (envirkey,envirval,command))

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
            if ogc_schemas_location is not None:
                xmlvalidate.validate(post, ogc_schemas_location = ogc_schemas_location)
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
    elif run_under_asan:
        asan_log = 'result/' + out_file + ".asan.txt"
        command = command.strip()
        command += " 2>%s" % asan_log

    if verbose:
        print('')
        print( command )
        
    os.system( command )

    if begin != -1 and end != -1 and begin < end:
        del os.environ['CONTENT_LENGTH']
        del os.environ['REQUEST_METHOD']
        del os.environ['MS_MAPFILE']

    if envirkey != '':
        del os.environ[envirkey]

    if demime:
        demime_file( 'result/'+out_file )
    if deversion:
        deversion_file( 'result/'+out_file )
        degdalversion_file( 'result/'+out_file )
        fixexponent_file( 'result/'+out_file )
        truncate_one_decimal( 'result/'+out_file )
        detimestamp_file( 'result/'+out_file )
    if extractserviceversion:
        extract_service_version_file( 'result/'+out_file )
    if valgrind:
        if os.path.getsize(valgrind_log) == 0:
            os.remove( valgrind_log )
        else:
            if not quiet:
                print('     Valgrind log non empty.')

    elif run_under_asan:
        if os.path.getsize(asan_log) == 0:
            os.remove( asan_log )
        else:
            asan_log_content = open(asan_log, 'rt').read()
            # We get some unexplained crashes on Travis-CI
            if "Assertion `unscaled->face" in asan_log_content:
                os.remove( asan_log )
            elif 'AddressSanitizer' in asan_log_content or 'LeakSanitizer' in asan_log_content:
                if not quiet:
                    print('     ASAN log non empty.')
            else:
                # some other standard error output
                os.remove( asan_log )

    apply_strip_items_file( 'result/'+out_file, strip_items )

    crlf('result/'+out_file)

    cmp = compare_result( out_file )

    if cmp == 'match':
        if not keep_pass:
            os.remove( 'result/' + out_file )
        if not quiet:
            print('     results match.')
        else:
            sys.stdout.write('.')
            sys.stdout.flush()
    elif cmp ==  'files_differ_image_match':
        if strict:
            return False, ('results dont match (though images match)', map, out_file)

        else:
            if not keep_pass:
                os.remove( 'result/' + out_file )
            if not quiet:
                print('     result images match, though files differ.')
            else:
                sys.stdout.write('.')
                sys.stdout.flush()
    elif cmp ==  'files_differ_image_nearly_match':
        if strict:
            return False, ('results dont match (though images perceptually match)', map, out_file)

        else:
            if not keep_pass:
                os.remove( 'result/' + out_file )
            if not quiet:
                print('     result images perceptually match, though files differ.')
            else:
                print('%s: result images perceptually match, though files differ.'%(out_file))

    elif cmp ==  'nomatch':
        return False, ('results dont match', map, out_file)

    elif cmp == 'noresult':
        with open('result/' + out_file, 'w') as fh:
            fh.write("Segmentation fault or other serious error\n")

        return False, ('no result file generated', map, out_file)

    elif cmp == 'noexpected':
        if strict:
            return False, ('no existing file expected/' + out_file)

        if not quiet:
            print('     no expected file exists, accepting result as expected.')
        else:
            print('%s: no expected file exists, accepting result as expected.'%(out_file))

        os.rename( 'result/' + out_file, 'expected/' + out_file )

    return True, None

###############################################################################


def run_pytest(map, out_file, command, extra_args):

    os.chdir(os.path.dirname(map))

    ret, msg = _run(map, out_file, command, extra_args)
    assert ret, msg

###############################################################################


def pytest_main():
    maps = []
    new_argv = []
    help = False
    for arg in sys.argv:
        if arg.endswith('.map'):
            maps.append(arg[0:-4])
        else:
            if arg == '--help':
                help = True
            new_argv.append(arg)
    sys.argv = new_argv
    if maps:
        sys.argv.append('-k')
        sys.argv.append(' or '.join(maps))
    ret = pytest.main()
    if help:
        print('\nMapServer note: you can also specify one or several .map filenames')
    return ret
