// $Id$
//
// See README_THREADTEST.TXT for usage details.
//


import edu.umn.gis.mapscript.*;

public class MapThread extends Thread {
    MapThread(String mapfile, int iterations, int id) {
        this.mapfile = mapfile;
        this.iterations = iterations;
	this.id=id;
    }

    public void run() {
	System.out.println("Thread "+id+" running...");

        for( int i = 0; i < iterations; i++ ) {
            mapObj  map = new mapObj(mapfile);
	    long path=Math.round(Math.random()*10);

	    if ( path > 5  ) {
	       System.out.println("Thread "+id+"-"+i+" querying...");
	       query(map);
	    } else {
	       System.out.println("Thread "+id+"-"+i+" using geos to create a buffer...");
               createBuffer(map);
	    }
   	    // We use this to test swig's memory management code
	    System.gc();
            //map.draw().save("/tmp/mapthread"+id+"-"+i+".png", map);
            map.draw();
	    
        }
	System.out.println("Thread "+id+" done.");
    }

    public void createBuffer(mapObj map) {

	layerObj layer = map.getLayerByName("POINT");
	if (layer!=null) {
		layer.open();
		shapeObj shape=layer.getFeature(0,-1);
		if (shape!=null) {
			shapeObj buffer=shape.buffer(0.1);
			if (buffer != null) {
				layerObj bufferLayer=new layerObj(map);
				bufferLayer.setStatus(mapscriptConstants.MS_DEFAULT);
				bufferLayer.setDebug(mapscriptConstants.MS_ON);
				bufferLayer.setName("BUFFER");
				//bufferLayer.setType(mapscriptConstants.MS_LAYER_POLYGON);
				bufferLayer.setProjection("init=epsg:4326");
				bufferLayer.setType(mapscriptConstants.MS_LAYER_POINT);
				bufferLayer.setTransparency(50);
				classObj clazz=new classObj(bufferLayer);
				clazz.setName("Buffer class");
				styleObj style=new styleObj(clazz);
				colorObj green=new colorObj(0,254,0,-4);
				//green.setRGB(0,254,0);
				style.setColor(green);
				bufferLayer.addFeature(buffer);

				// just for safety
				//layer.addFeature(buffer);
			} else {
				System.out.println("Buffer shape is NULL!");
			}
		}
	}	
    }

    public void query(mapObj map) {

	layerObj layer = map.getLayerByName("POINT");
	layer.setTemplate("template.html");
	String filter="A Point";

	layer.queryByAttributes(map,"FNAME", filter, mapscriptConstants.MS_MULTIPLE);
	layer.open();
	System.out.println( " numresults: " +layer.getNumResults() );
	layer.close();
    }

    String      mapfile;
    int         iterations;
    int 	id;
}
