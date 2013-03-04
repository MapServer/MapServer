import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript RFC24 tests.</p>
 * <p>Description: Tests for RFC24 implementation. (http://mapserver.gis.umn.edu/development/rfc/ms-rfc-24/)</p>
 * @author Umberto Nicoletti (umberto.nicoletti@gmail.com)
 */

public class RFC24 {
	String mapfile;

	public static void main(String[] args) {
		new RFC24(args[0]).run();
	}
	
	public RFC24(String mapfile) {
		this.mapfile=mapfile;
	}
	
	public void run() {
		System.out.println("Running "+getClass().getName());
		testLayerObj();
		testClassObj();
		testInsertLayerObj();
		testInsertClassObj();
		testGetLayerObj();
		testGetLayerObjByName();
		testGetClassObj();
		testGetOutputformatObj();
		testInsertOutputformatObj();

		System.out.println("Finished "+getClass().getName());
	}
	
	public void testLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(map);
		
		map=null;
		gc();
		assertNotNull(newLayer.getMap(), "testLayerObj");
	}
	
	public void testInsertLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(null);
		map.insertLayer(newLayer,-1);
		
		map=null;
		gc();
		assertNotNull(newLayer.getMap(), "testInsertLayerObj");
	}
	
	public void testGetLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayer(1);
		
		map=null;
		gc();
		assertNotNull(newLayer.getMap(), "testGetLayerObj");
	}

	public void testGetLayerObjByName() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayerByName("POLYGON");
		
		map=null;
		gc();
		assertNotNull(newLayer.getMap(), "testGetLayerObjByName");
	}

	public void testClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(layer);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.getLayer(), "testClassObj");
	}

	public void testInsertClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(null);
		layer.insertClass(newClass,-1);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.getLayer(), "testInsertClassObj");
	}

	public void testGetClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=layer.getClass(0);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.getLayer(), "testGetClassObj");
	}

        public void testGetOutputformatObj() {
                mapObj map=new mapObj(mapfile);
                outputFormatObj format=null;
                for (int i=0; i<map.getNumoutputformats(); i++) {
                    format = map.getOutputFormat(i);
                    //System.out.println("["+i+"] Format name: "+format.getName());
                } 
                map=null;
                gc();
                assertNotNull(format.getDriver(), "testGetOutputformatObj");
        }

        public void testInsertOutputformatObj() {
                mapObj map=new mapObj(mapfile);
                outputFormatObj format=null;
                for (int i=0; i<map.getNumoutputformats(); i++) {
                    format = map.getOutputFormat(i);
		    // uncomment to see output
                    //System.out.println("["+i+"] Format name: "+format.getName() + " driver="+format.getDriver());
                } 
                outputFormatObj newFormat=new outputFormatObj("AGG/PNG","test/rfc24");
		map.appendOutputFormat(newFormat);
                for (int i=0; i<map.getNumoutputformats(); i++) {
                    format = map.getOutputFormat(i);
		    // uncomment to see output
                    //System.out.println("["+i+"] Format name: "+format.getName());
                } 
		map.draw();
                map=null; format=null;
                gc();
                assertNotNull(newFormat.getDriver(), "testInsertOutputformatObj");
        }

	public void gc() {
		for (int i=0; i<10; i++)
			System.gc();
	}
	
	public void assertNotNull(Object object, String test) {
		if ( object != null )
			System.out.println("\t- "+test+" PASSED");
		else
			System.out.println("\t- "+test+" FAILED");
	}
}
