#  mapscriptsupp.tcl
# 
#  define procs to invoke object methods using an object pointer
#  tom poindexter, nov 2000, tpoindex@nyx.net
#  
#  e.g.  shapeObj method arg arg ...
#        can be invoked on a shapeObj pointer as:
#        shapeObjRef $shapeObjPointer method arg arg ...        
#
#  convenience procedures:
#   getDBFNames  $DBFHandle		       :returns list of field names
#   getDBFTypes  $DBFHandle 		       :returns list of field types 
#   getDBFValues $DBFHandle $rec	       :returns list of field values
#   getDBFQueryInfo $mapPtr $queryResultPtr    :return "field value" from query


# fix SWIG error
# some swig versions don't handle namespaces when generating shadow objects
# mapscript just has one - ms_error
#

if {![string length [info commands ::mapscript::ms_error]]} {
  ::mapscript::errorObj ::mapscript::ms_error -this [::mapscript::ms_error_get]
}

#
# now define object helper procs
#

namespace eval ::mapscript {

proc _mapscript_init_ {} {

    set code {
        proc OBJECTRef {ptr method args} {
	    # check ptr for correct object type
	    #if {! [regexp {_[^_]*_OBJECT_p} $ptr]} {
	    #	error "OBJECTRef pointer \"$ptr\" has wrong object class"
	    #}
	    set argc [llength $args]
	    # check for configure/cget commands 
	    set member ""
	    set command $method
	    switch -- $method {
		configure {
		    set command set
		    set member _[string range [lindex $args 0] 1 end]
		    set args [lrange $args 1 end]
		}
		cget {
		    set command get
		    set member _[string range [lindex $args 0] 1 end]
		    set args ""
		}
	    }
	    # check of existence of command
	    if {! [string length \
		      [info command ::mapscript::OBJECT${member}_$command]]} {
		error "OBJECT does not have method named \"$method\", or \
		       a member named \"$member\""
	    }
	    # invoke command 
	    return [uplevel "::mapscript::OBJECT${member}_$command $ptr $args"]
        }
    }

    foreach obj {featureObj colorObj queryObj shapeResultObj queryResultObj \
	         queryMapObj labelObj webObj classObj labelCacheMemberObj   \
	         labelCacheObj markerCacheMemberObj symbolSetObj \
		 referenceMapObj scalebarObj legendObj layerObj mapObj \
		 errorObj rectObj pointObj lineObj shapeObj shapefileObj \
		 DBFInfo} {
    
        regsub -all OBJECT $code $obj proc_code
        eval $proc_code
    }
}

_mapscript_init_
rename _mapscript_init_ {}

#
# now create convenience procs
#

proc getDBFNames {dbf} {
    set n [DBFGetFieldCount $dbf]
    set names ""
    for {set i 0} {$i < $n} {incr i} {
	lappend names [DBFInfoRef $dbf getFieldName $i]
    }
    return $names
}

proc getDBFTypes {dbf} {
    set n [DBFGetFieldCount $dbf]
    set types ""
    for {set i 0} {$i < $n} {incr i} {
	set t [DBFInfoRef $dbf getFieldType $i]
	switch $t \
	    $::mapscript::FTString  {set t string}  \
	    $::mapscript::FTInteger {set t integer} \
	    $::mapscript::FTDouble  {set t double}  \
	    default      {set t invalid}

	lappend t [DBFInfoRef $dbf getFieldWidth $i]
        lappend types $t
    }
    return $types
}

proc getDBFValues {dbf rec} {
    set n [DBFGetFieldCount $dbf]
    set r [DBFGetRecordCount $dbf]
    if {$rec >= $r || $rec < -1} {return ""}
    set values ""
    for {set i 0} {$i < $n} {incr i} {
	switch [DBFInfoRef $dbf getFieldType $i] \
	    $::mapscript::FTString  {
		set t [DBFReadStringAttribute  $dbf $rec $i]
	    } \
	    $::mapscript::FTInteger {
		set t [DBFReadIntegerAttribute $dbf $rec $i]
	    } \
	    $::mapscript::FTDouble  {
		set t [DBFReadDoubleAttribute  $dbf $rec $i]
	    } \
	    default      {set t ""}
	lappend values $t
    }
    return $values
}

proc getDBFQueryInfo {mapPtr queryResultPtr} {
    set shapePath [mapObjRef $mapPtr cget -shapepath]
    set numResults [queryResultObjRef $queryResultPtr cget -numresults]
    set result ""
    for {set i 0} {$i < $numResults} {incr i} {
        set shapeResultPtr [queryResultObjRef $queryResultPtr next]
        set layerNum [shapeResultObjRef $shapeResultPtr cget -layer]
        set shpNum   [shapeResultObjRef $shapeResultPtr cget -shape]
        set layerPtr [mapObjRef $mapPtr getLayer $layerNum]
        set layerName [layerObjRef $layerPtr cget -name]
        set layerFile [layerObjRef $layerPtr cget -data]
	set dbfile $shapePath/$layerFile.dbf
	set dbf [DBFOpen $dbfile r]
        set names  [getDBFNames  $dbf]
        set values [getDBFValues $dbf $shpNum]
	set pairs ""
	set j 0
	foreach n $names {
	    lappend pairs $n [lindex $values $j]
	    incr j
	}
	lappend result $layerName $pairs
	DBFClose $dbf
    }
    return $result
}



} ;#end of namespace command
