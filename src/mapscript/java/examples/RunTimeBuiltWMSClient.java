import edu.umn.gis.mapscript.*;

/**
	This example demoes a wms client almost entirely configured
	at run time and without the use of a map file.
	TODO: remove the need for even a basic map file and do everything at run time

	@author: Nicole Herman, Umberto Nicoletti
*/
public class RunTimeBuiltWMSClient {
	public static void main(String[] args)  {
	   System.out.println(mapscript.msGetVersion());
	   mapObj  map;
	   webObj web;
	   imageObj bild;
	   	   
	   map = new mapObj("data/emptymap.map");
	   /*
	   map.setWidth(400);
	   map.setHeight(400);
	   map.setDebug(1);
           // map.setExtent(3280364,5237512,3921499,6103271);
           map.setExtent(3400000,5700000,3700000,6000000);
	   // map.setExtent(3300000,5600000,3800000,6100000);
	   */
	   map.setProjection("init=epsg:31467");
	   map.setImageType("png");
	   
	   outputFormatObj output = new outputFormatObj("gd/png", "");
	   output.setName("png");
	   output.setDriver("gd/png");
	   output.setMimetype("image/png");
	   output.setExtension("png");
	   output.setImagemode(MS_IMAGEMODE.MS_IMAGEMODE_RGB.swigValue());
	   
	   /* This fixes bug #1870 and #1803 */
	   // Instanz des WebObjekts 
	   // web = new webObj();
	   web=map.getWeb();
	   web.setImagepath("/tmp/");
	   web.setImageurl("http://katrin/~nicol/mapserver/tmp/");
	   web.setLog("/tmp/wms.log");
	   web.setHeader("nh_header.html");
	   web.setTemplate("../html/form.html");
	   web.setEmpty("../themen/noFeature.html");
	   
	   // no longer necessary
	   //web.setMap(map);
	   //map.setWeb(web);
	   System.out.println("ImagePath="+web.getImagepath());


	   // Layer Object wird erzeugt
	   layerObj layer;
	   layer = new layerObj(map);
	   layer.setName("DUEKN5000");
           layer.setDebug(mapscriptConstants.MS_ON);
	   layer.setType(MS_LAYER_TYPE.MS_LAYER_RASTER);
	   layer.setConnectiontype(MS_CONNECTION_TYPE.MS_WMS);
	   // TODO: replace with a permanent url
	   layer.setConnection("http://www.mapserver.niedersachsen.de/freezoneogc/mapserverogc?");
	   layer.setMetaData("wms_srs", "EPSG:31467");
	   layer.setMetaData("wms_name", "DUEKN5000");
	   layer.setMetaData("wms_server_version", "1.1.1");
	   layer.setMetaData("wms_format","image/png");
	   layer.setProjection("init=epsg:31467");
	   layer.setStatus(mapscriptConstants.MS_ON);

	   bild = map.draw();
	   bild.save("test.png", map);
	   bild.delete();
	}
}

