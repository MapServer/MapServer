#!/bin/bash

ASTYLEOPTS="--style=kr --indent=spaces=2 -c --lineend=linux -S"
ASTYLEBIN=astyle

$ASTYLEBIN $ASTYLEOPTS *.c *.h *.cpp opengl/*.h
$ASTYLEBIN $ASTYLEOPTS -R 'mapscript/*.c'
$ASTYLEBIN $ASTYLEOPTS -R 'mapscript/*.h'

#find . -name '*.orig' -exec rm -f {} \;
