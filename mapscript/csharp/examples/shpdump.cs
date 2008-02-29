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
using OSGeo.MapServer;

/// <summary>
/// A C# based mapscript mapscript to dump information from a shapefile.
/// </summary>  
class ShapeDump {
  public static void usage() 
  { 
	Console.WriteLine("usage: shpdump {shapefile}");
	System.Environment.Exit(-1);
  }
  	
  public static void Main(string[] args) {

	if (args.Length != 1) usage();
		  
 	shapefileObj sf_obj = new shapefileObj(args[0],-1); 
	shapeObj s_obj = new shapeObj(-1);
	for (int i=0; i<sf_obj.numshapes; i++) 
	{
	  sf_obj.get(i, s_obj);
	  Console.WriteLine("Shape " + i + " has " + s_obj.numlines + " part(s)");
	  Console.WriteLine("bounds (" + s_obj.bounds.minx + ", " +  s_obj.bounds.miny + ") (" + s_obj.bounds.maxx + ", " + s_obj.bounds.maxy + ")" );
	  for(int j=0; j<s_obj.numlines; j++) 
	  {
		lineObj l_obj = s_obj.get(j);
		Console.WriteLine("Part " + j + " has " + l_obj.numpoints + " points.");
                    
		for(int k=0; k < l_obj.numpoints; k++) {
		  pointObj p_obj = l_obj.get(k);                     
          Console.WriteLine(k +": " + p_obj.x + ", " + p_obj.y);							
		}
	  }	  
    }
  }
}