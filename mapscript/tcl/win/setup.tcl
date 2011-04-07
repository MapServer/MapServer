# install mapscript/tcl

set tclhome [file dirname $tcl_library]
set mslib $tclhome/MapscriptTcl1.0

if {! [file isdirectory $mslib]} {
    file mkdir $mslib
}

file copy -force libMapscript.dll      $mslib
file copy -force ../pkgIndex.tcl       $mslib
file copy -force ../mapscriptsupp.tcl  $mslib
catch {file copy -force ../mapscriptsupp.html  $mslib}
catch {file copy -force ../mapscript_wrap.html  $mslib}

wm title . "Mapscript/Tcl Installer"
label .l -text "Mapscript/Tcl Installation is complete."
button .b -text Ok -command exit
pack .l .b -side top -padx 30 -pady 30

