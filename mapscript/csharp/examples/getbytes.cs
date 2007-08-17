using System;
using System.Drawing;
using System.IO;
using OSGeo.MapServer;
	
/**
 * <p>Title: Mapscript getBytes example.</p>
 * <p>Description: A C# based mapscript example to show the usage of imageObj.getBytes.</p>
 * @author Tamas Szekeres	szekerest@gmail.com
 * @version 1.0  
*/

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
			Console.WriteLine ("Drawing map: '" + map.name + "' using imageObj.getBytes");

			using(imageObj image = map.draw())
			{
				byte[] img = image.getBytes();
				using (MemoryStream ms = new MemoryStream(img))
				{
					Image mapimage = Image.FromStream(ms);
					mapimage.Save(args[1]);
				}
			}
        } 
        catch (Exception ex) 
        {
            Console.WriteLine( "GetBytes: ", ex.Message );
        }
    }

}

