# MapServer QIX File (aka ShpTree)

https://support.esri.com/en/white-paper/279

## Shape File Model

The shape file consists of three standard components.

* **shp** file contains the binary representations of the shapes (points, lines, polygons)
* **dbf** file contains the associated attribute information for each shape, one row per shape
* **shx** includes an ordered list of byte offsets of shapes in the **shp** file

The records in each component are inter-related by their **order of appearance**. The first entry in the **shx** file is the byte offset to the first shape in the shape file, and the first record of the dbf file contains the attributes of the first shape.

In addition to the standard three components, the QIX model adds a fourth component.

* **qix** file contains a binary quad-tree, in which every node in the three includes a bounds, a list of child nodes, and a list of "shape ids" that are contained in that node. The "shape ids" are just the record numbers. So "shape id" **0** corresponds to the first record in the shp/dbf/shx files, and so on.

## Header

There is an **8 byte** header.

* **byte 0, len 3** are file signature string, always "SQT"
* **byte 3, len 1** signals the byte-order of multi-byte types
  * [see code](https://github.com/MapServer/MapServer/blob/8af82513f8be8a380acbb5d92f9c5dec360ee5f1/maptree.h#L72-L76)
  * 1 = LSB (little-endian)
  * 2 = MSB (big-endian)
  * 0 = native (the endian of the system that wrote the file (probably LSB))
* **byte 4, len 1** is the version number
  * 1 = current version
* **byte 5, len 3** is reserved for future use
* **byte 8, len 4** is an **integer** count of number of shapes in the tree
* **byte 12, len 4** is an **integer** count of depth of the three

All **integer** values may require a byte swap, if the system byte order differs from the file byte order.

## Nodes

Nodes contain an 

* **byte 0, len 4** is an **integer** offset from the just after the numShapes of this node to the start of the next sibling node
* **byte 4, len 8** is a the **double** minx of the node
* **byte 12, len 8** is a the **double** miny of the node
* **byte 20, len 8** is a the **double** maxx of the node
* **byte 28, len 8** is a the **double** maxy of the node
* **byte 36, len 4** is an **integer** numShapes, the count of shape ids stored in this node
* **byte 40, len 4\*numShapes** is an **array of integer** shape ids
* **btye 40+4\*numShapes, 40+4\*numShapes+4** is an **integer** numChildren number of child nodes under this node

All **integer** and **double** values may require a byte swap, if the system byte order differs from the file byte order.

Note that the **offset** is the distance in bytes from just after the numShapes entry, 40 bytes from the start of the node. Or put another way, "offset + 40" is the distance from the start of **this** node to the start of the **next sibling** node.

## Arrangement of Components

* The file starts with a header.
* A node and all its descendents always form a contiguous range of the file.
* A node has at most four children.
* Both internal and leaf nodes may contain shape ids.

```
Header
Node A
Node AA
Node AAA
Node AAB
Node AAC
Node AB
Node ABA
Node ABB
Node AC
Node B
Node BA
Node BAA
Node BAB
Node BB
Node C
```

## Searching

### The Tree

The spatial search will involve a "query box" and will return a list of "shape id".

* Recursively search from the top node.
* Check that the node box and the query box intersect.
* If they do not, return.
* If they do, then add the "shape ids" in the node to the result list.
* Call the search function on each child node.

You can use the "offset" of each child to find the location of the next child in the set.

Implementation of a reader will probably want to opportunistically load portions of the whole tree into a memory structure, rather than hopping around with random reads. If "distance to next sibling" < some size, load whole subtree into memory and iterate on that.

### For Features

Once the tree search is complete, retrieving the features involves pulling the data from the **shp** and **dbf** files. If the shape file has been "cloud optimized", a query rectangle should involve a minimized number of range requests on the data files, since the order of records should match the order of nodes in the tree.

Analyze the "shape id" list coming out of the tree search, and find contiguous or almost-contiguous ranges of ids. Lookup the [byte offsets](https://www.esri.com/content/dam/esrisites/sitecore-archive/Files/Pdfs/library/whitepapers/pdfs/shapefile.pdf#page=28) from the **shx** file, then retrieve those ranges of data from the **shp** file. The **dbf** file is based on a fixed-length record, so once the header is read, finding the byte offset of a record just involving multiplying the record length by the shape id number.

