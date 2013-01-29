import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript outputformat dump example.</p>
 * @author Umberto Nicoletti 	umberto.nicoletti@gmail.com
 * @version 1.0
 */

public class OutputFormatList {

  public static void usage() {
    System.err.println("Usage: OutputFormatList {mapfile}");
    System.exit(-1);
  }

  public static void main(String[] args) {
    if (args.length != 1) usage();
   
    mapObj map = new mapObj(args[0]);
    for (int i=0; i<map.getNumoutputformats(); i++) {
	outputFormatObj format = map.getOutputFormat(i);
	System.out.println("["+i+"] Format name: "+format.getName());
    }
  }
}
