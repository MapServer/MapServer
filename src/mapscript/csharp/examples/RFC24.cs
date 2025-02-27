/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Tests for RFC24 implementation. 
 *           (http://mapserver.gis.umn.edu/development/rfc/ms-rfc-24/).
 * Author:   Tamas Szekeres, szekerest@gmail.com
 *           Umberto Nicoletti, umberto.nicoletti@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 1996-2008 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

using System;
using OSGeo.MapServer;

/**
 * <p>Title: Mapscript RFC24 tests.</p>
 * <p>Description: Tests for RFC24 implementation. (http://mapserver.gis.umn.edu/development/rfc/ms-rfc-24/)</p>
 * @author Umberto Nicoletti (umberto.nicoletti@gmail.com)
 */

class RFC24 {
	string mapfile;
	int fails = 0;

	public static void Main(string[] args) {
		new RFC24(args[0]).run();
	}
	
	public RFC24(string mapfile) {
		this.mapfile=mapfile;
	}
	
	public void run() {
		Console.WriteLine("Running RFC24");
		testLayerObj();
		testLayerObjDestroy();
		testClassObj();
		testClassObjDestroy();
		testInsertLayerObj();
		testInsertLayerObjDestroy();
		testRemoveLayerObj();
		testInsertClassObj();
		testInsertClassObjDestroy();
		testRemoveClassObj();
		testGetLayerObj();
		testGetLayerObjDestroy();
		testGetLayerObjByName();
		testGetLayerObjByNameDestroy();
		testGetClassObj();
		testGetClassObjDestroy();
		try { testStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testStyleObj exception:" + e.Message); }
		testStyleObjDestroy();
		try { testInsertStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testInsertStyleObj exception:" + e.Message); }
		testInsertStyleObjDestroy();
		testRemoveStyleObj();
		try { testGetStyleObj(); }
		catch (Exception e) { Console.WriteLine("\t- testGetStyleObj exception:" + e.Message); }
		testGetStyleObjDestroy();

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

		if (fails > 0)
			Console.WriteLine("\n     " + fails + " tests were FAILED!!!\n");

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
		assert(newLayer.refcount == 2, "testLayerObj refcount");
	}

	public void testLayerObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(map);
		layerObj reference = map.getLayer(map.numlayers-1);

		assert(newLayer.refcount == 3, "testLayerObjDestroy precondition");
		newLayer.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		newLayer = null;
		gc();
		assert(reference.refcount == 2, "testLayerObjDestroy");
	}
	
	public void testInsertLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(null);
		map.insertLayer(newLayer,-1);
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testInsertLayerObj");
		assert(newLayer.refcount == 2, "testInsertLayerObj refcount");
	}

	public void testInsertLayerObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(null);
		map.insertLayer(newLayer,0);
		layerObj reference = map.getLayer(0);

		assert(newLayer.refcount == 3, "testInsertLayerObjDestroy precondition");
		newLayer.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		newLayer=null;
		gc();
		assert(reference.refcount == 2, "testInsertLayerObjDestroy");
	}

	public void testRemoveLayerObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=new layerObj(null);
		map.insertLayer(newLayer,0);
		map.removeLayer(0);

		map=null;
		gc();
		assert(newLayer.refcount == 1, "testRemoveLayerObj");
	}
	
	public void testGetLayerObj() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayer(1);
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testGetLayerObj");
		assert(newLayer.refcount == 2, "testGetLayerObj refcount");
	}

	public void testGetLayerObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayer(1);
		layerObj reference = map.getLayer(1);
		
		assert(newLayer.refcount == 3, "testGetLayerObjDestroy precondition");
		newLayer.Dispose(); // force the destruction needed for Mono on Windows
		newLayer=null;
		gc();
		assert(reference.refcount == 2, "testGetLayerObjDestroy");
	}

	public void testGetLayerObjByName() {
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayerByName("POLYGON");
		
		map=null;
		gc();
		assertNotNull(newLayer.map, "testGetLayerObjByName");
		assert(newLayer.refcount == 2, "testGetLayerObjByName refcount");
	}

	public void testGetLayerObjByNameDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj newLayer=map.getLayerByName("POLYGON");
		layerObj reference=map.getLayerByName("POLYGON");
		
		assert(newLayer.refcount == 3, "testGetLayerObjByNameDestroy precondition");
		newLayer.Dispose(); // force the destruction needed for Mono on Windows
		newLayer=null;
		gc();
		assert(reference.refcount == 2, "testGetLayerObjByNameDestroy");
	}

	public void testClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(layer);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testClassObj");
		assert(newClass.refcount == 2, "testClassObj refcount");
	}

	public void testClassObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(layer);
		classObj reference=layer.getClass(layer.numclasses-1);
		
		assert(newClass.refcount == 3, "testClassObjDestroy precondition");
		newClass.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		map=null; layer=null; newClass=null;
		gc();
		assert(reference.refcount == 2, "testClassObjDestroy");
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

	public void testStyleObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj newStyle=new styleObj(classobj);
		styleObj reference=classobj.getStyle(classobj.numstyles-1);
		
		assert(newStyle.refcount == 3, "testStyleObjDestroy");
		newStyle.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		map=null; layer=null; classobj=null; newStyle=null;
		gc();
		assert(reference.refcount == 2, "testStyleObjDestroy");
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
		assert(newClass.refcount == 2, "testInsertClassObj refcount");
	}

	public void testRemoveClassObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(null);
		layer.insertClass(newClass,0);
		layer.removeClass(0);
		
		map=null; layer=null;
		gc();
		assert(newClass.refcount == 1, "testRemoveClassObj");
	}

	public void testInsertClassObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=new classObj(null);
		layer.insertClass(newClass,0);
		classObj reference = layer.getClass(0);

		assert(newClass.refcount == 3, "testInsertClassObjDestroy precondition");
		newClass.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		map=null; layer=null; newClass=null;
		gc();
		assert(reference.refcount == 2, "testInsertClassObjDestroy");
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

	public void testRemoveStyleObj() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj newStyle = new styleObj(null);
		classobj.insertStyle(newStyle,0);
		classobj.removeStyle(0);
		
		map=null; layer=null; classobj=null;
		gc();
		assert(newStyle.refcount == 1, "testRemoveStyleObj");
	}

	public void testInsertStyleObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj newStyle = new styleObj(null);
		classobj.insertStyle(newStyle,0);
		styleObj reference = classobj.getStyle(0);
		
		assert(newStyle.refcount == 3, "testInsertStyleObjDestroy precondition");
		newStyle.Dispose(); // force the destruction for Mono on Windows because of the constructor overload
		map=null; layer=null; classobj=null; newStyle=null;
		gc();
		assert(reference.refcount == 2, "testInsertStyleObjDestroy");
	}

	public void testGetClassObj() {
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=layer.getClass(0);
		
		map=null; layer=null;
		gc();
		assertNotNull(newClass.layer, "testGetClassObj");
		assert(newClass.refcount == 2, "testGetClassObj refcount");
	}

	public void testGetClassObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj newClass=layer.getClass(0);
		classObj reference = layer.getClass(0);
		
		assert(newClass.refcount == 3, "testGetClassObjDestroy precondition");
		newClass.Dispose();
		map=null; layer=null; newClass=null;
		gc();
		assert(reference.refcount == 2, "testGetClassObjDestroy");
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

	public void testGetStyleObjDestroy() 
	{
		mapObj map=new mapObj(mapfile);
		layerObj layer=map.getLayer(1);
		classObj classobj=layer.getClass(0);
		styleObj style=classobj.getStyle(0);
		styleObj reference=classobj.getStyle(0);
		
		assert(style.refcount == 3, "testGetStyleObjDestroy precondition");
		style.Dispose();
		map=null; layer=null; classobj=null; style=null;
		gc();
		assert(reference.refcount == 2, "testGetStyleObjDestroy");
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
		{
			Console.WriteLine("\t- "+test+" FAILED");
			++fails;
		}
	}

	public void assert(bool val, string test) 
	{
		if ( val )
			Console.WriteLine("\t- "+test+" PASSED");
		else
		{
			Console.WriteLine("\t- "+test+" FAILED");
			++fails;
		}
	}
}
