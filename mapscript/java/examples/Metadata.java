import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript shape dump example.</p>
 * <p>Description: A Java based mapscript example to create an image given a mapfile.</p>
 * @author Yew K Choo 	ykchoo@geozervice.com
 * @version 1.0
 */

public class Metadata {

  public static void usage() {
    System.err.println("Usage: Metadata {mapfile}");
    System.exit(-1);
  }

  public static void main(String[] args) {
    if (args.length != 1) usage();
   
    mapObj map = new mapObj(args[0]);
    
    for (int i=0; i<100; i++) {
	    System.out.println("Web->key1 ? "+map.getWeb().getMetadata().get("key1",null));
	    System.out.println("Web->key2 ? "+map.getWeb().getMetadata().get("key2",null));
	    System.out.println("Web->key3 ? "+map.getWeb().getMetadata().get("key3",null));
	    System.out.println("Web->key4 ? "+map.getWeb().getMetadata().get("key4",null));
	    System.out.println("Web->key5 ? "+map.getWeb().getMetadata().get("key5",null));
	    
	    System.gc();
	    System.gc();
	    System.gc();
	    
	    System.out.println("Web->key1 ? "+map.getWeb().getMetadata().get("key1",null));
    }
    map.delete();
  }
}
