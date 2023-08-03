import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript shapeinfo example.</p>
 * <p>Description: A Java based mapscript example to dump information from a shapefile.</p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: GeoZervice</p>
 * @author Yew K Choo (ykchoo@geozervice.com)
 * @version 1.0
 */

public class ShapeInfo {
  public  static String getShapeType(int type)
  {
    switch (type)
    {
      case 1: return "point";
      case 3: return "arc";
      case 5: return "polygon";
      case 8: return "multipoint";
      default: return "unknown";
    }
  }

  public static void usage() {
    System.err.println("Usage: ShapeInfo {shapefile.shp} {shapefile.dbf}");
    System.exit(-1);
  }

  public static void main(String[] args) {

    if (args.length != 2) usage();

    shapefileObj shapefile = new shapefileObj (args[0],-1);

    System.out.println ("Shapefile name = " + args[0]);
    System.out.println ("Type = "  + getShapeType(shapefile.getType()));
    System.out.println ("Number of features " + shapefile.getNumshapes());
    System.out.println("bounds (" + shapefile.getBounds().getMinx() + "," + shapefile.getBounds().getMiny() + ")" +
                        "(" + shapefile.getBounds().getMaxx() + "," + shapefile.getBounds().getMaxy() + ")");
    shapefile.delete();
    //mapscript.msCleanup();
  }

}
