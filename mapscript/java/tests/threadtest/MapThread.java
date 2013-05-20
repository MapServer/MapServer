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

	/*
		Uncomment this if you need to reschedule threads
	if (id>=5) {
		try {
			sleep(10000);
		} catch(InterruptedException ie) {
			ie.printStackTrace();
		}
	}
	*/

        for( int i = 0; i < iterations; i++ ) {
            mapObj  map = new mapObj(mapfile);
	    long path=Math.round(Math.random()*10);

	    if ( path > 5  ) {
	       System.out.println("Thread "+id+"-"+i+" querying...");
	       query(map);
	    } else {
	       System.out.println("Thread "+id+"-"+i+" using geos to create a buffer...");
	       try {
               	  createBuffer(map);
	       } catch(Exception e) {
		   System.out.println("have you enabled GEOS support? "+e.getMessage());
	       }
	    }
   	    // We use this to test swig's memory management code
	    //System.gc();
            //map.draw().save("/tmp/mapthread"+id+"-"+i+".png", map);
            map.draw();
	    
        }
	mapscript.msConnPoolCloseUnreferenced();
	System.out.println("Thread "+id+" done.");
    }

    public void createBuffer(mapObj map) {

	layerObj layer = map.getLayer(3);
	if (layer!=null) {
		layer.open();
		layer.queryByIndex(map,0,-1,mapscriptConstants.MS_FALSE);
		shapeObj shape=layer.getShape(layer.getResults().getResult(0));
		if (shape!=null) {
			shapeObj buffer=shape.buffer(0.1);
			if (buffer != null) {
				layerObj bufferLayer=new layerObj(map);
				bufferLayer.setStatus(mapscriptConstants.MS_DEFAULT);
				bufferLayer.setDebug(mapscriptConstants.MS_ON);
				bufferLayer.setName("BUFFER");
				//bufferLayer.setType(mapscriptConstants.MS_LAYER_POLYGON);
				bufferLayer.setProjection("init=epsg:4326");
				bufferLayer.setType(MS_LAYER_TYPE.MS_LAYER_POINT);
				bufferLayer.setOpacity(50);
				classObj clazz=new classObj(bufferLayer);
				clazz.setName("Buffer class");
				styleObj style=new styleObj(clazz);
				colorObj green=new colorObj(0,254,0,255);
				//green.setRGB(0,254,0);
				style.setColor(green);
				bufferLayer.addFeature(buffer);

			} else {
				System.out.println("Buffer shape is NULL!");
			}
		}
	}	
    }

    public void query(mapObj map) {

	layerObj layer = map.getLayer(3);
	if (layer!=null) {
	   layer.setTemplate("template.html");
	   String filter="A Point";

	   layer.queryByAttributes(map,"FNAME", filter, mapscriptConstants.MS_MULTIPLE);
	   layer.open();
	   System.out.println( " numresults: " +layer.getNumResults() );
	   layer.close();
	}
    }

    String      mapfile;
    int         iterations;
    int 	id;
}
