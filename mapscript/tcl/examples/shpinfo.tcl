#!/usr/local/bin/tclsh8.3

# if running a test before install, include directory to find package

lappend auto_path [pwd]/..

package require Mapscript

array set types {
   1 point
   3 arc
   5 polygon
   8 multipoint
}


set file $argv

if {! [file isfile $file.shp]} {
    puts "shapefile \"$file\" not found"
    puts "usage: $argv0 <shapefile>"
    exit
}

mapscript::shapefileObj shp1 $file -1

puts "\nShapefile $file:\n"
puts "\ttype: $types([shp1 cget -type])"
puts "\tnumber of features: [shp1 cget -numshapes]"
set rectPtr [shp1 cget -bounds]
puts "\tbounds: ([mapscript::rectObjRef $rectPtr cget -minx],[mapscript::rectObjRef $rectPtr cget -miny]) ([mapscript::rectObjRef $rectPtr cget -maxx],[mapscript::rectObjRef $rectPtr cget -maxy])"


