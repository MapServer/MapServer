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
#   getQueryInfo $mapPtr $layerPtr             :return "field value" from query


# fix SWIG error
# some swig versions don't handle namespaces when generating shadow objects
# mapscript just has one - ms_error
#

if {![string length [info commands ::mapscript::ms_error]]} {
 catch {
  ::mapscript::errorObj ::mapscript::ms_error -this [::mapscript::ms_error_get]
 }
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
	    # check for configure/cget/delete commands 
	    set member ""
	    set delete ""
	    switch -- $method {
		configure {
		    set command _set
		    set member _[string range [lindex $args 0] 1 end]
		    set args [lrange $args 1 end]
		}
		cget {
		    set command _get
		    set member _[string range [lindex $args 0] 1 end]
		    set args ""
		}
		delete {
		    set delete delete_
		    set command ""
		    set args ""
		}
		default {
	    	    set command _$method
		}
	    }
	    # check of existence of command
	    if {! [string length \
	      [info command ::mapscript::${delete}OBJECT${member}${command}]]} {
		error "OBJECT does not have method named \"$method\", or \
		       a member named \"$member\""
	    }
	    # invoke command 
	    return [uplevel "::mapscript::${delete}OBJECT${member}${command} \
								$ptr $args"]
        }
    }

    foreach obj {classObj colorObj errorObj featureListNodeObj itemObj  \
		 labelCacheMemberObj labelCacheObj labelObj layerObj  \
		 legendObj lineObj mapObj markerCacheMemberObj pointObj \
		 projectionObj queryMapObj rectObj referenceMapObj  \
		 resultCacheMemberObj resultCacheObj scalebarObj shapeObj \
		 shapefileObj symbolSetObj webObj \
		 DBFInfo} {
    
        regsub -all OBJECT $code $obj proc_code
        eval $proc_code
    }
}


# run the help proc initialization, then get rid of it

_mapscript_init_
rename _mapscript_init_ {}


#
# now create convenience procs
#

proc getDBFNames {dbf} {
    set n [msDBFGetFieldCount $dbf]
    set names ""
    for {set i 0} {$i < $n} {incr i} {
	lappend names [DBFInfoRef $dbf getFieldName $i]
    }
    return $names
}

proc getDBFTypes {dbf} {
    set n [msDBFGetFieldCount $dbf]
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
    set n [msDBFGetFieldCount $dbf]
    set r [msDBFGetRecordCount $dbf]
    if {$rec >= $r || $rec < -1} {return ""}
    set values ""
    for {set i 0} {$i < $n} {incr i} {
	switch [DBFInfoRef $dbf getFieldType $i] \
	    $::mapscript::FTString  {
		set t [msDBFReadStringAttribute  $dbf $rec $i]
	    } \
	    $::mapscript::FTInteger {
		set t [msDBFReadIntegerAttribute $dbf $rec $i]
	    } \
	    $::mapscript::FTDouble  {
		set t [msDBFReadDoubleAttribute  $dbf $rec $i]
	    } \
	    default      {set t ""}
	lappend values $t
    }
    return $values
}

proc getQueryInfo {mapPtr layerPtr} {
    
    layerObjRef $layerPtr open [mapObjRef $mapPtr cget -shapepath]
    set rescachePtr [layerObjRef $layerPtr cget -resultcache]
    set numResults  [resultCacheObjRef $rescachePtr cget -numresults]

    for {set i 0} {$i < $numResults} {incr i} {
	set resmemPtr [layerObjRef $layerPtr getResult $i]
        set shpIdx    [resultCacheMemberObjRef $resmemPtr cget -shapeindex]
        set tileIdx   [resultCacheMemberObjRef $resmemPtr cget -tileindex]
	set shpPtr    [shapeObj -args $mapscript::MS_SHAPE_NULL]
	set rc        [layerObjRef $layerPtr getShape $shpPtr $tileIdx $shpIdx]
	set numValues [shapeObjRef $shpPtr cget -numvalues]

	set pairs ""
	for {set j 0} {$j < $numValues} {incr j} {
	    lappend pairs [layerObjRef $layerPtr getItem $j] \
			  [shapeObjRef $shpPtr getValue $j]
	}
        # delete the temporary shpPtr
	catch {rename $shpPtr {}}
    }
    layerObjRef $layerPtr close
    return $pairs
}



} ;#end of namespace command
