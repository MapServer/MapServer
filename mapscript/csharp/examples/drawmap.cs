using System;
	
/**
 * <p>Title: Mapscript shape dump example.</p>
 * <p>Description: A C# based mapscript example to create an image given a mapfile.</p>
 * @author Yew K Choo 	ykchoo@geozervice.com
 * @version 1.0  
*/

/// <summary>
/// A C# based mapscript mapscript example to create an image given a mapfile.
/// </summary>
class DrawMap
{
  public static void usage() 
  { 
	Console.WriteLine("usage: DrawMap {mapfile} {outfile}");
	System.Environment.Exit(-1);
  }
		  
  public static void Main(string[] args)
  {
    if (args.Length != 2) usage();
    
	mapObj m_obj = new mapObj(args[0]);	
	Console.WriteLine ("# Map layers " + m_obj.numlayers + "; Map name = " + m_obj.name);	
	for (int i=0; i<m_obj.numlayers; i++) 
	{
	  Console.WriteLine("Layer [" + i + "] name: " + m_obj.getLayer(i).name);
	}
	
    imageObj i_obj = m_obj.draw();
	Console.WriteLine("Image URL = " + i_obj.imageurl + "; Image path = " + i_obj.imagepath);    
	Console.WriteLine("Image height = " + i_obj.height + "; width = " + i_obj.width); 
	try 
	{
	  i_obj.save(args[1],m_obj);
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

