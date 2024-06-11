#!/usr/bin/perl
############################################################################
# $Id: $
#
# Project:  MapServer
# Purpose:  MapScript WxS example
# Author:   Tom Kralidis (tomkralidis@gmail.com)
#
##############################################################################
# Copyright (c) 2007, Tom Kralidis
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies of this Software or works derived from this Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
############################################################################/

use CGI::Carp qw(fatalsToBrowser);
use mapscript;
use strict;
use warnings;
use XML::LibXSLT;
use XML::LibXML;

my $dispatch;

# uber-trivial XSLT document, as a file
my $xsltfile = "/tmp/foo.xslt";

# here's the actual document inline for
# testing save and alter $xsltFile above

=comment
my $xsltstring = <<END;
<?xml version="1.0" encoding="UTF-8"?> 
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:wfs="http://www.opengis.net/wfs">
 <xsl:output method="xml" indent="yes"/> 
 <xsl:template match="/"> 
<WFSLayers> 
<xsl:for-each select="//wfs:FeatureType"> 
<wfs_layer>
<name><xsl:value-of select="wfs:Name"/></name> 
<title><xsl:value-of select="wfs:Title"/></title> 
</wfs_layer>
</xsl:for-each> 
</WFSLayers> 
</xsl:template> 
</xsl:stylesheet>
=cut

my $mapfile  = "/tmp/config.map";

# init OWSRequest object
my $req = new mapscript::OWSRequest();

# pick up CGI parameters passed
$req->loadParams();

# init mapfile 
my $map = new mapscript::mapObj($mapfile);

# if this is a WFS GetCapabilities request, then intercept
# what is normally returned, process with an XSLT document
# and then return that to the client
if ($req->getValueByName('REQUEST') eq "GetCapabilities" && $req->getValueByName('SERVICE') eq "WFS") {

  # push STDOUT to a buffer and run the incoming request
  my $io = mapscript::msIO_installStdoutToBuffer();
  $dispatch = $map->OWSDispatch($req);

  # at this point, the client's request is sent

  # pull out the HTTP headers
  my $ct = mapscript::msIO_stripStdoutBufferContentType();

  # and then pick up the actual content of the response
  my $content = mapscript::msIO_getStdoutBufferString();

  my $xml  = XML::LibXML->new();
  my $xslt = XML::LibXSLT->new();

  # load XML content
  my $source = $xml->parse_string($content);

  # load XSLT document
  my $style_doc = $xml->parse_file($xsltfile);
  my $stylesheet = $xslt->parse_stylesheet($style_doc);

  # invoke the XSLT transformation
  my $results = $stylesheet->transform($source);

  # print out the result (header + content)
  print "Content-type: $ct\n\n";
  print $stylesheet->output_string($results);
}

# else process as normal
else {
  $dispatch = $map->OWSDispatch($req);
}
