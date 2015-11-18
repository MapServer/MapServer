/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  A C# based mapscript example to use the atribute query and 
 *           highlight the results.
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
using OSGeo.MapServer;

/// <summary>
/// A C# based mapscript example to use the arribute query an highlight the results.
/// </summary>
class DrawQuery
{
    public static void usage() 
    { 
	    Console.WriteLine("usage: QueryMap {mapfile} {query string} {outfile} {-zoom}");
	    System.Environment.Exit(-1);
    }
    		  
    public static void Main(string[] args)
    {
        Console.WriteLine("");
		if (args.Length < 3 || args.Length > 4) usage();
        
        bool ZoomToResults = (args.Length == 4 && args[3] == "-zoom");

	    mapObj map = new mapObj(args[0]);	
	    Console.WriteLine ("# Map layers " + map.numlayers + "; Map name = " + map.name);
        
        QueryByAttribute(args[1], map, ZoomToResults);

        map.querymap.status = mapscript.MS_ON;
        map.querymap.color.setRGB(0,0,255,255);
        map.querymap.style = (int)MS_QUERYMAP_STYLES.MS_HILITE;

        try 
        {
            imageObj image = map.drawQuery();
            image.save(args[2],map);
        } 
        catch (Exception ex) 
        {
            Console.WriteLine( "QueryMap: ", ex.Message );
        }
    }

    private static bool IsLayerQueryable(layerObj layer)
    {
        if ( layer.type == MS_LAYER_TYPE.MS_LAYER_TILEINDEX )
            return false;

        if(layer.template != null && layer.template.Length > 0) return true;

        for(int i=0; i<layer.numclasses; i++) 
        {
            if(layer.getClass(i).template != null && layer.getClass(i).template.Length > 0)
                                                        return true;
        }
        return false;
    }

    public static void QueryByAttribute(string qstring, mapObj map, bool zoomToResults)
    {
        Console.WriteLine("\nPerforming QueryByAttribute:");
        try
        {
        	layerObj layer;
            rectObj query_bounds = null;
            for (int i = 0; i < map.numlayers; i++)
            {
                layer = map.getLayer(i);
                if (layer.connection != null && IsLayerQueryable(layer))
                {
                    Console.WriteLine("Layer [" + i + "] name: " + layer.name);
                    BuildQuery(layer, qstring);
                    // zoom to the query results
                    using (resultCacheObj results = layer.getResults())
                    {
                        if (results != null && results.numresults > 0)
                        {
                            // calculating the extent of the results
                            if (query_bounds == null)
                                query_bounds = new rectObj(results.bounds.minx, results.bounds.miny,
                                    results.bounds.maxx, results.bounds.maxy,0);
                            else
                            {
                                if (results.bounds.minx < query_bounds.minx) query_bounds.minx = results.bounds.minx;
                                if (results.bounds.miny < query_bounds.miny) query_bounds.miny = results.bounds.miny;
                                if (results.bounds.maxx > query_bounds.maxx) query_bounds.maxx = results.bounds.maxx;
                                if (results.bounds.maxy > query_bounds.maxy) query_bounds.maxy = results.bounds.maxy;
                            }
                        }
                    }
                }
            }
            // setting the map extent to the result bounds
			if (query_bounds != null) 
			{
				if (zoomToResults) 
				{
					map.setExtent(query_bounds.minx, query_bounds.miny, query_bounds.maxx, query_bounds.maxy);
					map.scaleExtent(1.2, 0, 0); // increasing the visible area
					Console.WriteLine("Current map scale: 1:" + (int)map.scaledenom);
				}
			}
			else
				Console.WriteLine("The query returned 0 results ...");
        }
        catch (Exception e)
        {
        	Console.WriteLine("QueryByAttribute: " + e.Message);
        }
    }

    private static void BuildQuery(layerObj layer, string qstring)
    {
        if (layer != null && layer.map != null)
        {
            /*layer.open();
            string qs = "";
            string att = "";
            for (int i=0; i < layer.numitems; i++)
            {
                if (qs == "")
                {
                    qs = "(";
                    att = layer.getItem(i);
                }
                else
                {
                    qs += " OR ";
                }
                qs += "'[" + layer.getItem(i) + "]'='" + qstring + "'";
            }
            qs += ")";
            layer.close();*/
            string qs = qstring;
            string att = null;

            Console.WriteLine("Query string: " + qs);
            
            try
            {
                layer.queryByAttributes(layer.map, att, qs, 1);
            }
            catch (Exception e)
            {
                Console.WriteLine("BuildQuery: " + e.Message);
            }
        }
    }
}

