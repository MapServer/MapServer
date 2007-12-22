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
		try { testlegendObj(); }
		catch (Exception e) { Console.WriteLine("\t- testlegendObj exception:" + e.Message); }
		try { testreferenceMapObj(); } 
		catch (Exception e) { Console.WriteLine("\t- testreferenceMapObj exception:" + e.Message); }
		try { testwebObj(); }
		catch (Exception e) { Console.WriteLine("\t- testwebObj exception:" + e.Message); }
		try { testqueryMapObj(); }
		catch (Exception e) { Console.WriteLine("\t- testqueryMapObj exception:" + e.Message); }
		try { testmapObjHashTable(); }
		catch (Exception e) { Console.WriteLine("\t- testmapObjHashTable exception:" + e.Message); }
		try { testsymbolSetObj(); }
		catch (Exception e) { Console.WriteLine("\t- testsymbolSetObj exception:" + e.Message); }
		try { testimageObj(); }
		catch (Exception e) { Console.WriteLine("\t- testimageObj exception:" + e.Message); }
		try { testStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testStyleObj exception:" + e.Message); }
		try { testInsertStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testInsertStyleObj exception:" + e.Message); }
		try { testGetStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testGetStyleObj exception:" + e.Message); }
		Console.WriteLine("Finished RFC24");
	}

	public void testlegendObj() 
	{
		mapObj map=new mapObj(mapfile);
		legendObj legend = map.legend;
		legend.template = "This is a sample!";

		map=null;
		gc();	
		assert(legend.template == "This is a sample!", "testlegendObj");
	}

	public void testmapObjHashTable() 
	{
		mapObj map=new mapObj(mapfile);
		hashTableObj configoptions = map.configoptions;
		configoptions.set("key", "test value");

		map=null;
		gc();
	
		assert(configoptions.get("key", "") == "test value", "testmapObjHashTable");
	}

	public void testsymbolSetObj() 
	{
		mapObj map=new mapObj(mapfile);
		symbolSetObj symbolset = map.symbolset;
		symbolset.filename = "filename";

		map=null;
		gc();
	
		assert(symbolset.filename == "filename", "testsymbolSetObj");
	}

	public void testreferenceMapObj() 
	{
		mapObj map=new mapObj(mapfile);
		referenceMapObj refmap = map.reference;
		refmap.markername = "This is a sample!";

		map=null;
		gc();
	    assert(refmap.markername == "This is a sample!", "testreferenceMapObj");
	}

	public void testwebObj() 
	{
		mapObj map=new mapObj(mapfile);
		webObj web = map.web;
		web.template = "This is a sample!";

		map=null;
		gc();
		assert(web.template == "This is a sample!", "testwebObj");
	}

	public void testqueryMapObj() 
	{
		mapObj map=new mapObj(mapfile);
		queryMapObj querymap = map.querymap;
		querymap.color.setHex( "#13ba88" );

		map=null;
		gc();
		assert(querymap.color.toHex() == "#13ba88", "testqueryMapObj");
	}

	public void testimageObj() 
	{
		mapObj map=new mapObj(mapfile);
		imageObj image = map.draw();
		outputFormatObj format = image.format;
		format.setOption( "INTERLACE", "OFF");

		map=null;
		gc();
		assert(format.getOption("INTERLACE", "") == "OFF", "testimageObj");
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

	public void testStyleObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj newStyle=new styleObj(classobj);
		
		map=null; layer=null; classobj=null;
		gc();
		assert(newStyle.refcount == 2, "testStyleObj");
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

	public void testInsertStyleObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj newStyle = new styleObj(null);
		classobj.insertStyle(newStyle,-1);
		
		assert(newStyle.refcount == 2, "testInsertStyleObj precondition");
		map=null; layer=null; classobj=null;
		gc();
		assert(newStyle.refcount == 2, "testInsertStyleObj");
	}

	public void testGetClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=layer.getClass(0);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testGetClassObj");
	}

	public void testGetStyleObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj style=classobj.getStyle(0);
		
		map=null; layer=null; classobj=null;
		gc();
		assert(style.refcount == 2, "testGetStyleObj");
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

	public void assert(bool val, string test) 
	{
		if ( val )
			Console.WriteLine("\t- "+test+" PASSED");
		else
			Console.WriteLine("\t- "+test+" FAILED");
	}
}
