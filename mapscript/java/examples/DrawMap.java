import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript shape dump example.</p>
 * <p>Description: A Java based mapscript example to create an image given a mapfile.</p>
 * @author Yew K Choo 	ykchoo@geozervice.com
 * @version 1.0
 */

public class DrawMap {

  public static void usage() {
    System.err.println("Usage: DrawMap {mapfile} {outfile}");
    System.exit(-1);
  }

  public static void main(String[] args) {
    if (args.length != 2) usage();
   
    mapObj map = new mapObj(args[0]);
    //map.getImagecolor().setRGB(153, 153, 204);
    //styleObj st = map.getLayer(1).getClass(0).getStyle(0);
    //st.getColor().setHex("#000000");
    if( map.getLayer(1).getMetadata().get("hidden") != null ) {
    	System.out.println("Layer 1 is hidden? "+map.getLayer(1).getMetadata().get("hidden"));
    }
    int i=0;
    //for (i=0; i<100; i++) {
    	imageObj img = map.draw();
    	System.out.println("Image size is: "+img.getSize());
    	System.out.println("Image size from getBytes is: "+img.getBytes().length);
    	System.out.println(i+") the map will be drawn to:"+args[1]);
    	img.save(args[1], map);
    //}
    img.delete();
    map.delete();
    //mapscript.msCleanup();
  }
}
