/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to draw the map directly onto a GDI
 *           printing device context.
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
using System.Drawing;
using System.Drawing.Printing;
using OSGeo.MapServer;	

/// <summary>
/// A C# based mapscript example to draw the map directly onto a GDI printing device context.
/// </summary>
class DrawMap
{
  public static void usage() 
  { 
	Console.WriteLine("usage: DrawMapDirectPrint {mapfile} {printername}");
	System.Environment.Exit(-1);
  }

  static mapObj map;
		  
  public static void Main(string[] args)
  {
    Console.WriteLine("");
	if (args.Length < 2) usage();
    
	map = new mapObj(args[0]);

    Console.WriteLine("# Map layers " + map.numlayers + "; Map name = " + map.name);
    for (int i = 0; i < map.numlayers; i++) 
    {
        Console.WriteLine("Layer [" + i + "] name: " + map.getLayer(i).name);
    }

    try
    {
        PrintDocument doc = new PrintDocument();

        doc.PrintPage += new PrintPageEventHandler(doc_PrintPage);

        // Specify the printer to use.
        doc.PrinterSettings.PrinterName = args[1];

        doc.Print();
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

    static void doc_PrintPage(object sender, PrintPageEventArgs e)
    {
        // Create the output format
        outputFormatObj of = new outputFormatObj("CAIRO/WINGDIPRINT", "cairowinGDIPrint");
        map.appendOutputFormat(of);
        map.selectOutputFormat("cairowinGDIPrint");
        map.resolution = e.Graphics.DpiX;
        Console.WriteLine("map resolution = " + map.resolution.ToString() + "DPI  defresolution = " + map.defresolution.ToString() + " DPI");
        // Calculating the desired image size to cover the entire area; 
        map.width = Convert.ToInt32(e.PageBounds.Width * e.Graphics.DpiX / 100);
        map.height = Convert.ToInt32(e.PageBounds.Height * e.Graphics.DpiY / 100);

        Console.WriteLine("map size = " + map.width.ToString() + " * " + map.height.ToString() + " pixels");

        IntPtr hdc = e.Graphics.GetHdc();
        try
        {
            // Attach the device to the outputformat for drawing
            of.attachDevice(hdc);
            // Drawing directly to the GDI context
            using (imageObj image = map.draw()) { };
        }
        finally
        {
            of.attachDevice(IntPtr.Zero);
            e.Graphics.ReleaseHdc(hdc);
        }

        e.HasMorePages = false;
    }
}

