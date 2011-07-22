#!/bin/bash
INFILE="./mapaxisorder.csv"
OUTFILE="./mapaxisorder.h"

# create array of elements from $INFILE
unset ARRAY i
while read -r LINE; do ARRAY[i++]=$LINE; done < $INFILE

print_header ()
{

	echo '/******************************************************************************'
	echo ' * $Id: $'
	echo ' *'
	echo ' * Project:  MapServer'
	echo ' * Purpose:  Axis lookup table'
	echo ' *'
	echo ' ******************************************************************************'
	echo ' * Copyright (c) 1996-2005 Regents of the University of Minnesota.'
	echo ' *'
	echo ' * Permission is hereby granted, free of charge, to any person obtaining a'
	echo ' * copy of this software and associated documentation files (the "Software"),'
	echo ' * to deal in the Software without restriction, including without limitation'
	echo ' * the rights to use, copy, modify, merge, publish, distribute, sublicense,'
	echo ' * and/or sell copies of the Software, and to permit persons to whom the'
	echo ' * Software is furnished to do so, subject to the following conditions:'
	echo ' *'
	echo ' * The above copyright notice and this permission notice shall be included in '
	echo ' * all copies of this Software or works derived from this Software.'
	echo ' *'
	echo ' * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS'
	echo ' * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,'
	echo ' * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL'
	echo ' * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER'
	echo ' * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING'
	echo ' * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER'
	echo ' * DEALINGS IN THE SOFTWARE.'
	echo ' ****************************************************************************/'

}

print_comment ()
{
	echo ' '
	echo '/*'
	echo ' * Generated file'
	echo ' *'
	echo ' * This file was generated from by means of a script. Do not edit manually.'
	echo ' */'
	echo ' '

}

print_body ()
{
	
	echo '#ifdef __cplusplus'
	echo 'extern "C" '{
	echo '#endif'
	echo ' '
	echo 'static struct axisOrientationEpsgCodes_s {'
	echo '	int	code;'
	echo '} axisOrientationEpsgCodes[] = {'
   
	# unset first array element
   unset ARRAY[0]
   # traverse array and print out elements
        let count=0
	for x in "${ARRAY[@]}"; do
		echo " { ${x} },"
		((count++))
	done

	echo '};'
   echo ' '
	echo '#define AXIS_ORIENTATION_TABLE_SIZE '${count}
   echo ' '
	echo '#ifdef __cplusplus'
	echo '}'
	echo '#endif'

}

print_header > ./$OUTFILE
print_comment >> ./$OUTFILE
print_body >> ./$OUTFILE

exit 0
