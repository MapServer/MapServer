/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to draw the map directly onto a GDI+ bitmap.
 * Author:   Tamas Szekeres, szekerest@gmail.com
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
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using OSGeo.MapServer;	

/// <summary>
/// A C# based mapscript example to draw the map directly onto a GDI+ bitmap.
/// </summary>
class DrawMap
{
  public static void usage() 
  { 
	Console.WriteLine("usage: DrawMapGDIPlus {mapfile} {outfile}");
	System.Environment.Exit(-1);
  }
		  
  public static void Main(string[] args)
  {
    Console.WriteLine("");
	if (args.Length < 2) usage();
    
	mapObj map = new mapObj(args[0]);

    Console.WriteLine("# Map layers " + map.numlayers + "; Map name = " + map.name);
    for (int i = 0; i < map.numlayers; i++) 
	{
        Console.WriteLine("Layer [" + i + "] name: " + map.getLayer(i).name);
	}

    try
    {
        Bitmap mapImage = new Bitmap(map.width, map.height, PixelFormat.Format32bppRgb);
        Stopwatch stopwatch = new Stopwatch();
        stopwatch.Start();
        using (imageObj image = map.draw())
        {
            BitmapData bitmapData = mapImage.LockBits(new Rectangle(0, 0, image.width, image.height), ImageLockMode.ReadWrite, PixelFormat.Format32bppRgb);
            try
            {
                if (image.getRawPixels(bitmapData.Scan0) == (int)MS_RETURN_VALUE.MS_FAILURE)
                {
                    Console.WriteLine("Unable to get image contents");
                }
            }
            finally
            {
                mapImage.UnlockBits(bitmapData);
            }

            Console.WriteLine("Rendering time: " + stopwatch.ElapsedMilliseconds + "ms");

            mapImage.Save(args[1]);
        }    
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

