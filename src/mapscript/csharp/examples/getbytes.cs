/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to show the usage of 
 *           imageObj.getBytes.
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
using System.IO;
using OSGeo.MapServer;

/// <summary>
/// A C# based mapscript example to show the usage of imageObj.getBytes.
/// </summary>
class GetBytes
{
    public static void usage() 
    { 
	    Console.WriteLine("usage: getbytes {mapfile} {outfile}");
	    System.Environment.Exit(-1);
    }
    		  
    public static void Main(string[] args)
    {
        if (args.Length < 2) usage();
        
        try 
        {
			mapObj map = new mapObj(args[0]);

			using(imageObj image = map.draw())
			{
				// solution 1
				Console.WriteLine ("Drawing map: '" + map.name + "' using imageObj.getBytes");
				
				byte[] img = image.getBytes();
				using (MemoryStream ms = new MemoryStream(img))
				{
					Image mapimage = Image.FromStream(ms);
					mapimage.Save(args[1]);
				}
				
				// solution 2
				Console.WriteLine ("Drawing map: '" + map.name + "' using imageObj.write");

				using (FileStream fs = File.Open("_" + args[1], FileMode.OpenOrCreate, FileAccess.ReadWrite))
				{
					image.write(fs);
				}
			}
        } 
        catch (Exception ex) 
        {
            Console.WriteLine( "GetBytes: ", ex.Message );
        }
    }

}

