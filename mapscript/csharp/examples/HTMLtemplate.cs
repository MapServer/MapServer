using System;
using OSGeo.MapServer;
	
/**
 * <p>Title: Mapscript HTMLTemplate example.</p>
 * <p>Description: A C# based mapscript example to show the usage of HTML templates.</p>
 * @author Tamas Szekeres	szekerest@gmail.com
 * @version 1.0  
*/

/// <summary>
/// A C# based mapscript example to show the usage of HTML templates.
/// </summary>
class HTMLTemplate
{
    public static void usage() 
    { 
	    Console.WriteLine("usage: HTMLTemplate {mapfile} {templatefile} {outdir}");
	    System.Environment.Exit(-1);
    }
    		  
    public static void Main(string[] args)
    {
        if (args.Length < 3) usage();
        
        mapObj map = new mapObj(args[0]);
		map.legend.template = args[1];
		map.web.imagepath = args[2];
		map.web.imageurl = "";
		string str = null;
		string[] names = null, values = null;
		names = new string[] {"map"};
		values = new string[] { args[0] };
		str = map.processLegendTemplate(names, values, names.Length);
		Console.Write(str);
    }
}

