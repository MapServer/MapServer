/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to create an image given a mapfile.
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
/// A C# based mapscript mapscript example to create an image given a mapfile.
/// </summary>
class DrawMap
{
  public static void usage() 
  { 
	Console.WriteLine("usage: DrawMap {mapfile} {outfile} {imagetype optional}");
	System.Environment.Exit(-1);
  }
		  
  public static void Main(string[] args)
  {
    Console.WriteLine("");
	if (args.Length < 2) usage();
    
	mapObj m_obj = new mapObj(args[0]);

	if (args.Length >= 3) 
	{
      Console.WriteLine("Setting the imagetype to " + args[2]);
	  m_obj.setImageType(args[2]);
	}

	Console.WriteLine ("# Map layers " + m_obj.numlayers + "; Map name = " + m_obj.name);	
	for (int i=0; i<m_obj.numlayers; i++) 
	{
	  Console.WriteLine("Layer [" + i + "] name: " + m_obj.getLayer(i).name);
	}
	
    imageObj i_obj = m_obj.draw();
	Console.WriteLine("Image URL = " + i_obj.imageurl + "; Image path = " + i_obj.imagepath);    
	Console.WriteLine("Image height = " + i_obj.height + "; width = " + i_obj.width); 
	try 
	{
	  i_obj.save(args[1],m_obj);
    } 
	catch (Exception ex) 
	{
                Console.WriteLine( "\nMessage ---\n{0}", ex.Message );
                Console.WriteLine( 
                    "\nHelpLink ---\n{0}", ex.HelpLink );
                Console.WriteLine( "\nSource ---\n{0}", ex.Source );
                Console.WriteLine( 
                    "\nStackTrace ---\n{0}", ex.StackTrace );
                Console.WriteLine( 
                    "\nTargetSite ---\n{0}", ex.TargetSite );	}	
  }
}

