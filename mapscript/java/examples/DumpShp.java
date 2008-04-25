import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript shape dump example.</p>
 * <p>Description: A Java based mapscript to dump information from a shapefile.</p>
 * @author Yew K Choo  (ykchoo@geozervice.com)
 * @version 1.0
 */

public class DumpShp {
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
    System.err.println("Usage: DumpShp {shapefile.shp}");
    System.exit(-1);
  }

  public static void main(String[] args) {
    if (args.length != 1) usage();

    shapefileObj shapefile = new shapefileObj (args[0],-1);
    System.out.println ("Shapefile opened (type = "  + getShapeType(shapefile.getType()) +
                        " with " + shapefile.getNumshapes() + " shapes).");


    shapeObj shape = new shapeObj(-1);

    for(int i=0; i<shapefile.getNumshapes(); i++) {
        shapefile.get(i, shape);

        System.out.println("Shape[" + i + "] has " + shape.getNumlines() + " part(s)");
        System.out.println("bounds (" + shape.getBounds().getMinx() + "," + shape.getBounds().getMiny() + ")" +
                           "(" + shape.getBounds().getMaxx() + "," + shape.getBounds().getMaxy() + ")");

        for(int j=0; j<shape.getNumlines(); j++) {
            lineObj part = shape.get(j);
            System.out.println("Part[" +j + "] has " + part.getNumpoints() + " point(s)");

            for(int k=0; k<part.getNumpoints(); k++) {
                pointObj point = part.get(k);
                System.out.println("Point[" + k + "] = " + point.getX() + ", "  + point.getY());
            }
        }
    }
    // shape.delete();
    // shapefile.delete();
    // mapscript.msCleanup();
  }

}
