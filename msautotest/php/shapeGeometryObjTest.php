<?php

/**
 * Tests for shape geometry operations (GEOS-dependent).
 * Covers spatial predicates, geometry operations, and WKT conversion.
 */
class shapeGeometryObjTest extends \PHPUnit\Framework\TestCase
{
    protected $polygon;
    protected $innerPoint;
    protected $outerPoint;

    public function setUp(): void
    {
        // Create a polygon (square from 0,0 to 10,10)
        $this->polygon = new shapeObj(MS_SHAPE_POLYGON);
        $ring = new lineObj();
        $ring->add(new pointObj(0, 0));
        $ring->add(new pointObj(10, 0));
        $ring->add(new pointObj(10, 10));
        $ring->add(new pointObj(0, 10));
        $ring->add(new pointObj(0, 0));
        $this->polygon->add($ring);
        $this->polygon->setBounds();
    }

    public function testToWKT()
    {
        $wkt = $this->polygon->toWKT();
        $this->assertIsString($wkt);
        $this->assertStringContainsString('POLYGON', $wkt);
    }

    public function testFromWKT()
    {
        $shape = shapeObj::fromWKT('POINT (5 5)');
        $this->assertInstanceOf('shapeObj', $shape);
    }

    public function testContains()
    {
        $inner = shapeObj::fromWKT('POINT (5 5)');
        $this->assertEquals(MS_TRUE, $this->polygon->contains($inner));

        $outer = shapeObj::fromWKT('POINT (20 20)');
        $this->assertEquals(MS_FALSE, $this->polygon->contains($outer));
    }

    public function testWithin()
    {
        // Create a larger polygon
        $bigPoly = shapeObj::fromWKT('POLYGON ((-5 -5, 15 -5, 15 15, -5 15, -5 -5))');
        $this->assertEquals(MS_TRUE, $this->polygon->within($bigPoly));
    }

    public function testIntersects()
    {
        $overlapping = shapeObj::fromWKT('POLYGON ((5 5, 15 5, 15 15, 5 15, 5 5))');
        $this->assertEquals(MS_TRUE, $this->polygon->intersects($overlapping));
    }

    public function testDisjoint()
    {
        $farAway = shapeObj::fromWKT('POLYGON ((20 20, 30 20, 30 30, 20 30, 20 20))');
        $this->assertEquals(MS_TRUE, $this->polygon->disjoint($farAway));
    }

    public function testGetCentroid()
    {
        $centroid = $this->polygon->getCentroid();
        $this->assertInstanceOf('pointObj', $centroid);
        $this->assertEqualsWithDelta(5.0, $centroid->x, 0.001);
        $this->assertEqualsWithDelta(5.0, $centroid->y, 0.001);
    }

    public function testConvexHull()
    {
        $hull = $this->polygon->convexHull();
        $this->assertInstanceOf('shapeObj', $hull);
    }

    public function testBuffer()
    {
        $buffered = $this->polygon->buffer(1.0);
        $this->assertInstanceOf('shapeObj', $buffered);
    }

    public function testBoundary()
    {
        $boundary = $this->polygon->boundary();
        $this->assertInstanceOf('shapeObj', $boundary);
    }

    public function testUnion()
    {
        $other = shapeObj::fromWKT('POLYGON ((5 5, 15 5, 15 15, 5 15, 5 5))');
        $union = $this->polygon->Union($other);
        $this->assertInstanceOf('shapeObj', $union);
    }

    public function testIntersection()
    {
        $other = shapeObj::fromWKT('POLYGON ((5 5, 15 5, 15 15, 5 15, 5 5))');
        $result = $this->polygon->intersection($other);
        $this->assertInstanceOf('shapeObj', $result);
    }

    public function testDifference()
    {
        $other = shapeObj::fromWKT('POLYGON ((5 5, 15 5, 15 15, 5 15, 5 5))');
        $result = $this->polygon->difference($other);
        $this->assertInstanceOf('shapeObj', $result);
    }

    public function testGetArea()
    {
        $area = $this->polygon->getArea();
        $this->assertEqualsWithDelta(100.0, $area, 0.001);
    }

    public function testGetLength()
    {
        $length = $this->polygon->getLength();
        $this->assertEqualsWithDelta(40.0, $length, 0.001);
    }

    public function testSetBounds()
    {
        $this->polygon->setBounds();
        $bounds = $this->polygon->bounds;
        $this->assertInstanceOf('rectObj', $bounds);
        $this->assertEqualsWithDelta(0.0, $bounds->minx, 0.001);
        $this->assertEqualsWithDelta(10.0, $bounds->maxx, 0.001);
    }

    public function testCopy()
    {
        $dest = new shapeObj(MS_SHAPE_POLYGON);
        $this->assertEquals(MS_SUCCESS, $this->polygon->copy($dest));
        $this->assertEqualsWithDelta($this->polygon->getArea(), $dest->getArea(), 0.001);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->polygon, $this->innerPoint, $this->outerPoint);
    }
}

?>
