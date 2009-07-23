using System;

/**
 * <p>Title: Mapscript shape dump example.</p>
 * <p>Description: A Java based mapscript to dump information from a shapefile.</p>
 * @author Yew K Choo  (ykchoo@geozervice.com)
 * @version 1.0
 */

/// <summary>
/// A C# based mapscript mapscript to dump information from a shapefile.
/// </summary>  
class ShapeDump {
  public static void usage() 
  { 
	Console.WriteLine("usage: shpdump {shapefile}");
	System.Environment.Exit(-1);
  }
  	
  public static void Main(string[] args) {

	if (args.Length != 1) usage();
		  
 	shapefileObj sf_obj = new shapefileObj(args[0],-1); 
	shapeObj s_obj = new shapeObj(-1);
	for (int i=0; i<sf_obj.numshapes; i++) 
	{
	  sf_obj.get(i, s_obj);
	  Console.WriteLine("Shape " + i + " has " + s_obj.numlines + " part(s)");
	  Console.WriteLine("bounds (" + s_obj.bounds.minx + ", " +  s_obj.bounds.miny + ") (" + s_obj.bounds.maxx + ", " + s_obj.bounds.maxy + ")" );
	  for(int j=0; j<s_obj.numlines; j++) 
	  {
		lineObj l_obj = s_obj.get(i);
		Console.WriteLine("Part " + j + " has " + l_obj.numpoints + " points.");
                    
		for(int k=0; k < l_obj.numpoints; k++) {
		  pointObj p_obj = l_obj.get(k);                     
          Console.WriteLine(k +": " + p_obj.x + ", " + p_obj.y);							
		}
	  }	  
    }
  }
}