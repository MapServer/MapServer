/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based based mapscript example to dump information from 
 *           a shapefile.
 * Author:   Tamas Szekeres, szekerest@gmail.com
 *           Yew K Choo, ykchoo@geozervice.com 
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

using System;
using System.Collections;
using OSGeo.MapServer;

/// <summary>
/// A C# based mapscript mapscript to dump information from a shapefile.
/// </summary> 
class ShapeInfo {
	
  public static void usage() 
  { 
	Console.WriteLine("usage: shapeinfo {shapefile}");
	System.Environment.Exit(-1);
  }
  
  	
  public static void Main(string[] args) {
	 
	if (args.Length != 1) usage();

	Hashtable ht = new Hashtable();
	ht.Add(1,"point");	
	ht.Add(3,"arc");	
	ht.Add(5,"polygon");
	ht.Add(8,"multipoint");
		
    shapefileObj shpObj = new shapefileObj(args[0],-1); 
    Console.WriteLine ("ShapeType = " + ht[shpObj.type]);
    Console.WriteLine ("Num shapes = " + shpObj.numshapes);
    Console.WriteLine ("(xmin, ymin) = (" + shpObj.bounds.minx + "," + shpObj.bounds.miny + ") (xmax, ymax) = (" + shpObj.bounds.maxx + "," + shpObj.bounds.maxy + ")");
  }
}