using System;
	
/**
 * <p>Title: Mapscript query map example.</p>
 * <p>Description: A C# based mapscript example to use the arribute query an highlight the results.</p>
 * @author Tamas Szekeres	szekerest@gmail.com
 * @version 1.0  
*/

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
        if (args.Length < 3 || args.Length > 4) usage();
        
        bool ZoomToResults = (args.Length == 4 && args[3] == "-zoom");

	    mapObj map = new mapObj(args[0]);	
	    Console.WriteLine ("# Map layers " + map.numlayers + "; Map name = " + map.name);
        
        QueryByAttribute(args[1], map, ZoomToResults);

        map.querymap.status = mapscript.MS_ON;
        map.querymap.color.setRGB(0,0,255);
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
                if (layer.connection != null)
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
            if (zoomToResults)
                map.setExtent(query_bounds.minx, query_bounds.miny, query_bounds.maxx, query_bounds.maxy);
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
            layer.open();
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
            layer.close();

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

