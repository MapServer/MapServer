#!/usr/bin/env ruby 

require "mapscript"
include Mapscript

filename = ARGV[0]

shapefile = ShapefileObj.new(filename, -1)

shapepath = File.dirname(filename)
shapename = File.basename(filename, "....")

map = MapObj.new('')
map.shapepath = shapepath
map.height = 200
map.width = 200
map.extent = shapefile.bounds

$shapetypes =
{
   MS_SHAPEFILE_POINT      => MS_LAYER_POINT,
   MS_SHAPEFILE_ARC        => MS_LAYER_LINE,
   MS_SHAPEFILE_POLYGON    => MS_LAYER_POLYGON,
   MS_SHAPEFILE_MULTIPOINT => MS_LAYER_LINE
}

layer = LayerObj.new(map)
layer.name = shapename
layer.type = $shapetypes[shapefile.type]
layer.status = MS_ON
layer.data = shapename

cls = ClassObj.new(layer)
style = StyleObj.new()
color = ColorObj.new()
color.red = 107
color.green = 158
color.blue = 160
style.color = color
cls.insertStyle(style)

#map.save(shapename+'.map')
img = map.draw
img.save(shapename+'.png')
