/******************************************************************************
 * $Id: shapeinfo.cs 7418 2008-02-29 00:02:49Z nsavard $
 *
 * Project:  MapServer
 * Purpose:  A C# based based mapscript example to dump information from 
 *           a shapefile.
 * Author:   Tamas Szekeres, szekerest@gmail.com 
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
using System.Collections;
using OSGeo.MapServer;

/// <summary>
/// A MapScript application for creating inline layers with annotations.
/// </summary> 
class Inline {

  public static void usage()
  {
      Console.WriteLine("usage: inline [outformat] [outfile]");
      System.Environment.Exit(-1);
  }
  	
  public static void Main(string[] args) 
  {

      if (args.Length < 2) usage();

      // creating a new map from scratch
      mapObj map = new mapObj(null);
      // adding a layer
      layerObj layer = new layerObj(map);
      layer.type = MS_LAYER_TYPE.MS_LAYER_POINT;
      layer.status = mapscript.MS_ON;
      layer.connectiontype = MS_CONNECTION_TYPE.MS_INLINE;
      // define the attribute names from the inline layer
      layer.addProcessing("ITEMS=attribute1,attribute2,attribute3");
      // define the class
      classObj classobj = new classObj(layer);
      classobj.template = "query";   // making the layer queryable
      // setting up the text based on multiple attributes
      classobj.setText("('Shape:' + '[attribute1]' + ' Color:' + '[attribute2]' + ' Size:' + '[attribute3]')");
      // define the label
      classobj.label.outlinecolor = new colorObj(255, 255, 255, 0);
      classobj.label.force = mapscript.MS_TRUE;
      classobj.label.size = (double)MS_BITMAP_FONT_SIZES.MS_MEDIUM;
      classobj.label.position = (int)MS_POSITIONS_ENUM.MS_LC;
      classobj.label.wrap = ' ';
      // set up attribute binding
      classobj.label.setBinding((int)MS_LABEL_BINDING_ENUM.MS_LABEL_BINDING_COLOR, "attribute2");
      // define the style
      styleObj style = new styleObj(classobj);
      style.color = new colorObj(0, 255, 255, 0);
      style.setBinding((int)MS_STYLE_BINDING_ENUM.MS_STYLE_BINDING_COLOR, "attribute2");
      style.setBinding((int)MS_STYLE_BINDING_ENUM.MS_STYLE_BINDING_SIZE, "attribute3");

      Random rand = new Random((int)DateTime.Now.ToFileTime()); ;

      // creating the shapes
      for (int i = 0; i < 10; i++)
      {
          shapeObj shape = new shapeObj((int)MS_SHAPE_TYPE.MS_SHAPE_POINT);

          // setting the shape attributes
          shape.initValues(4);
          shape.setValue(0, Convert.ToString(i));
          shape.setValue(1, new colorObj(rand.Next(255), rand.Next(255), rand.Next(255), 0).toHex());
          shape.setValue(2, Convert.ToString(rand.Next(25) + 5));

          lineObj line = new lineObj();
          line.add(new pointObj(rand.Next(400) + 25, rand.Next(400) + 25, 0, 0));
          shape.add(line);
          layer.addFeature(shape);
      }

      map.width = 500;
      map.height = 500;
      map.setExtent(0,0,450,450);
      map.selectOutputFormat(args[0]);
      imageObj image = map.draw();
      image.save(args[1], map);

      //perform a query
      layer.queryByRect(map, new rectObj(0, 0, 450, 450, 0));

      resultObj res;
      shapeObj feature;
      using (resultCacheObj results = layer.getResults())
      {
          if (results != null && results.numresults > 0)
          {
              // extracting the features found
              layer.open();
              for (int j = 0; j < results.numresults; j++)
              {
                  res = results.getResult(j);
                  feature = layer.getShape(res);
                  if (feature != null)
                  {
                      Console.WriteLine("  Feature: shapeindex=" + res.shapeindex + " tileindex=" + res.tileindex);
                      for (int k = 0; k < layer.numitems; k++)
                      {
                          Console.Write("     " + layer.getItem(k));
                          Console.Write(" = ");
                          Console.Write(feature.getValue(k));
                          Console.WriteLine();
                      }
                  }
              }
              layer.close();
          }
      }
  }
}