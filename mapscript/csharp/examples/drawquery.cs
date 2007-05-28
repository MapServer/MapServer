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
	    Console.WriteLine("usage: QueryMap {mapfile} {query string} {outfile}");
	    System.Environment.Exit(-1);
    }
    		  
    public static void Main(string[] args)
    {
        if (args.Length != 3) usage();
        
	    mapObj map = new mapObj(args[0]);	
	    Console.WriteLine ("# Map layers " + map.numlayers + "; Map name = " + map.name);	
    	
        QueryByAttribute(args[1], map);

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

    public static void QueryByAttribute(string qstring, mapObj map)
    {
        Console.WriteLine("\nPerforming QueryByAttribute:");
        try
        {
        	layerObj layer;
        	for (int i = 0; i < map.numlayers; i++)
        	{
        		layer = map.getLayer(i);
                if (layer.connection != null)
        		{
        			Console.WriteLine("Layer [" + i + "] name: " + layer.name);
                    BuildQuery(layer, qstring);
        		}
        	}
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

