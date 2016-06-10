/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to draw the map directly onto a WPF image.
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
using System.IO;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using OSGeo.MapServer;	

/// <summary>
/// A C# based mapscript example to draw the map directly onto a WPF image.
/// </summary>
class DrawMap
{
  public static void usage() 
  { 
	Console.WriteLine("usage: DrawMapWPF {mapfile} {outfile}");
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
        WriteableBitmap mapImage = new WriteableBitmap(map.width, map.height, 96, 96, PixelFormats.Bgr32, null);
        Stopwatch stopwatch = new Stopwatch();
        stopwatch.Start();
        using (imageObj image = map.draw())
        {
            // Reserve the back buffer for updates.
            mapImage.Lock();
            try
            {
                if (image.getRawPixels(mapImage.BackBuffer) == (int)MS_RETURN_VALUE.MS_FAILURE)
                {
                    Console.WriteLine("Unable to get image contents");
                }
                // Specify the area of the bitmap that changed.
                mapImage.AddDirtyRect(new Int32Rect(0, 0, map.width, map.height));
            }
            finally
            {
                // Release the back buffer and make it available for display.
                mapImage.Unlock();
            }

            Console.WriteLine("Rendering time: " + stopwatch.ElapsedMilliseconds + "ms");

            // Save the bitmap into a file.
            using (FileStream stream = new FileStream(args[1], FileMode.Create))
            {
                PngBitmapEncoder encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(mapImage));
                encoder.Save(stream);
            }
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

