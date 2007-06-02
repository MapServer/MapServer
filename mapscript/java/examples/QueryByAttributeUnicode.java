import edu.umn.gis.mapscript.*;

public class QueryByAttributeUnicode {
	public static void main(String[] args)  {

		String filter="/Südliche Weinstraße/";
		if (args.length == 2) {
			filter=args[1];
		}
		// Unsupported
		//String langSettings=System.getenv("LANG");
		// Only since jdk 1.5
		//String charsetSetting=Charset.defaultCharset().displayName();
		String charsetSetting = new java.io.OutputStreamWriter(System.out).getEncoding();
		System.out.println( "("+charsetSetting+") Searching for: " +filter );        

		if ( charsetSetting.indexOf("UTF") == -1 ) {
			mapObj map = new mapObj(args[0]);
			layerObj layer = map.getLayerByName("test-iso");
			layer.queryByAttributes(map,"KREIS_NAME", filter, mapscriptConstants.MS_MULTIPLE);
			layer.open();

			resultCacheMemberObj result = layer.getResult(0);
			if (result==null) {
				System.out.println("Error: no results found, resultCacheMemberObj is null!");
			} else {
				shapeObj shp = new shapeObj( layer.getType().swigValue() );
				layer.getShape(shp, result.getTileindex(), result.getShapeindex());
				for (int z = 0; z < shp.getNumvalues(); z++) {
					System.out.println("shp.value[" + z + "]=" + shp.getValue(z));
				}

				System.out.println( "Results number (should be always 1): " +layer.getNumResults() );
			}
			layer.close();
		} else {
			mapObj map = new mapObj(args[0]);
			layerObj layer = map.getLayerByName("test-utf");
			layer.queryByAttributes(map,"KREIS_NAME", filter, mapscriptConstants.MS_MULTIPLE);
			layer.open();

			System.out.println( "Results number (should be always 1): " +layer.getNumResults() );
			layer.close();     
		}
	}
}

