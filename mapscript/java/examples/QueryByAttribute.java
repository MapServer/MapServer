import edu.umn.gis.mapscript.imageObj;
import edu.umn.gis.mapscript.mapObj;
import edu.umn.gis.mapscript.layerObj;
import edu.umn.gis.mapscript.shapeObj;
import edu.umn.gis.mapscript.mapscriptConstants;



public class QueryByAttribute {
	public static void main(String[] args)  {
	  
	String filter="A Point";
	mapObj map = new mapObj(args[0]);
	if (args.length == 2) {
		filter=args[1];
	} else {
		filter="A Point";
	}

	layerObj layer = map.getLayerByName("POINT");
	layer.setTemplate("template.html");

	layer.queryByAttributes(map,"FNAME", filter, mapscriptConstants.MS_MULTIPLE);
	layer.open();
	System.out.println( "Searched for: " +filter );        
	int results=layer.getNumResults();
	System.out.println( "Results number (should be always 1): " +results );
	for(int i=0;i<results;i++) {
		shapeObj shape=layer.getShape(layer.getResult(i));
		System.out.println( "shape["+i+"]=" +shape );
	}
	layer.close();


	}
}

