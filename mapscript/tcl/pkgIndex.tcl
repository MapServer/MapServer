# pkgIndex.tcl for mapscript
#
# load shared library, and source supplementaly ref procs

package ifneeded Mapscript 1.0 " 
    load [file join $dir libMapscript[info sharedlibextension]]
    # source the object reference helps and convenience procs
    source [file join $dir mapscriptsupp.tcl]
    package provide Mapscript 1.0
"


