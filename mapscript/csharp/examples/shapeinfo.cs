using System;
using System.Collections;

/**
 * <p>Title: Mapscript shapeinfo example.</p>
 * <p>Description: A Java based mapscript example to dump information from a shapefile.</p>
 * @author Yew K Choo (ykchoo@geozervice.com)
 * @version 1.0
 */

/// <summary>
/// A C# based mapscript mapscript to dump information from a shapefile.
/// </summary> 
class ShapeInfo {
	
  public static void usage() 
  { 
	Console.WriteLine("usage: shapeinfo {shapefile}");
	System.Environment.Exit(-1);
  }
  
  	
  public static void Main(string[] args) {
	 
	if (args.Length != 1) usage();

	Hashtable ht = new Hashtable();
	ht.Add(1,"point");	
	ht.Add(3,"arc");	
	ht.Add(5,"polygon");
	ht.Add(8,"multipoint");
		
    shapefileObj shpObj = new shapefileObj(args[0],-1); 
    Console.WriteLine ("ShapeType = " + ht[shpObj.type]);
    Console.WriteLine ("Num shapes = " + shpObj.numshapes);
    Console.WriteLine ("(xmin, ymin) = (" + shpObj.bounds.minx + "," + shpObj.bounds.miny + ") (xmax, ymax) = (" + shpObj.bounds.maxx + "," + shpObj.bounds.maxy + ")");
  }
}