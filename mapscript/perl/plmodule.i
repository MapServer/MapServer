/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Perl-specific enhancements to MapScript
 * Author:   SDL based on Sean's Python conventions
 *
 ******************************************************************************
 *
 * Perl-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGPERL is defined (via 'swig -perl5 ...').
 *
 *****************************************************************************/

/******************************************************************************
 * Simple Typemaps
 *****************************************************************************/

%init %{
  if(msSetup() != MS_SUCCESS) {
    msSetError(MS_MISCERR, "Error initializing MapServer/Mapscript.", "msSetup()");
  }
%}
