using System;
using OSGeo.MapServer;

/**
 * <p>Title: Mapscript RFC24 tests.</p>
 * <p>Description: Tests for RFC24 implementation. (http://mapserver.gis.umn.edu/development/rfc/ms-rfc-24/)</p>
 * @author Umberto Nicoletti (umberto.nicoletti@gmail.com)
 */

class RFC24 {
	string mapfile;

	public static void Main(string[] args) {
		new RFC24(args[0]).run();
	}
	
	public RFC24(string mapfile) {
		this.mapfile=mapfile;
	}
	
	public void run() {
		Console.WriteLine("Running RFC24");
		testLayerObj();
		testClassObj();
		testInsertLayerObj();
		testInsertClassObj();
		testGetLayerObj();
		testGetLayerObjByName();
		testGetClassObj();
		Console.WriteLine("Finished RFC24");
	}
	
	public void testLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(map);
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testLayerObj");
	}
	
	public void testInsertLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(null);
		map.insertLayer(newLayer,-1);
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testInsertLayerObj");
	}
	
	public void testGetLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayer(1);
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testGetLayerObj");
	}

	public void testGetLayerObjByName() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayerByName("POLYGON");
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testGetLayerObjByName");
	}

	public void testClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(layer);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testClassObj");
	}

	public void testInsertClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(null);
		layer.insertClass(newClass,-1);
		
		assertNotNull(newClass.layer, "testInsertClassObj precondition");
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testInsertClassObj");
	}

	public void testGetClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=layer.getClass(0);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testGetClassObj");
	}

	public void gc() {
		for (int i=0; i<100; i++) {
			GC.Collect();
			GC.WaitForPendingFinalizers();
		}
	}
	
	public void assertNotNull(object o, string test) {
		if ( o != null )
			Console.WriteLine("\t- "+test+" PASSED");
		else
			Console.WriteLine("\t- "+test+" FAILED");
	}
}
