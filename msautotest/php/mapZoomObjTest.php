<?php

class mapZoomObjTest extends \PHPUnit\Framework\TestCase
{
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
    }

    public function testZoomPointIn()
    {
        $pixPos = new pointObj();
        $pixPos->setXY($this->map->width / 2, $this->map->height / 2);
        $geoExt = $this->map->extent;
        $origMinx = $geoExt->minx;

        $result = $this->map->zoomPoint(2, $pixPos, $this->map->width, $this->map->height, $geoExt, null);
        $this->assertEquals(MS_SUCCESS, $result);
        // After zoom in, extent should be smaller
        $newDelta = $this->map->extent->maxx - $this->map->extent->minx;
        $origDelta = $geoExt->maxx - $origMinx;
        $this->assertLessThan($origDelta, $newDelta);
    }

    public function testZoomPointOut()
    {
        $pixPos = new pointObj();
        $pixPos->setXY($this->map->width / 2, $this->map->height / 2);
        $geoExt = $this->map->extent;
        $origDelta = $geoExt->maxx - $geoExt->minx;

        $result = $this->map->zoomPoint(-2, $pixPos, $this->map->width, $this->map->height, $geoExt, null);
        $this->assertEquals(MS_SUCCESS, $result);
        // After zoom out, extent should be larger
        $newDelta = $this->map->extent->maxx - $this->map->extent->minx;
        $this->assertGreaterThan($origDelta, $newDelta);
    }

    public function testZoomPointPan()
    {
        $pixPos = new pointObj();
        // Click off-center to pan
        $pixPos->setXY($this->map->width / 4, $this->map->height / 4);
        $this->map->setExtent($this->map->extent->minx, $this->map->extent->miny, $this->map->extent->maxx, $this->map->extent->maxy);
        $geoExt = $this->map->extent;
        $origDelta = $geoExt->maxx - $geoExt->minx;

        $result = $this->map->zoomPoint(1, $pixPos, $this->map->width, $this->map->height, $geoExt, null);
        $this->assertEquals(MS_SUCCESS, $result);
        // Pan should preserve extent size (approximately)
        $newDelta = $this->map->extent->maxx - $this->map->extent->minx;
        $this->assertEqualsWithDelta($origDelta, $newDelta, $origDelta * 0.01);
    }

    public function testZoomRectangle()
    {
        $geoExt = $this->map->extent;
        // Pixel rectangle (note: miny > maxy per mapzoom.i convention)
        $pixRect = new rectObj();
        // $pixRect->setExtent(50, 150, 150, 50);
        $pixRect->minx = 50;
        $pixRect->miny = 150;
        $pixRect->maxx = 150;
        $pixRect->maxy = 50;

        $result = $this->map->zoomRectangle($pixRect, $this->map->width, $this->map->height, $geoExt, null);
        $this->assertEquals(MS_SUCCESS, $result);
    }

    public function testZoomScale()
    {
        $pixPos = new pointObj();
        $pixPos->setXY($this->map->width / 2, $this->map->height / 2);
        $geoExt = $this->map->extent;

        $result = $this->map->zoomScale(50000000, $pixPos, $this->map->width, $this->map->height, $geoExt, null);
        $this->assertEquals(MS_SUCCESS, $result);
    }

    public function testZoomPointWithMaxExtent()
    {
        $pixPos = new pointObj();
        $pixPos->setXY($this->map->width / 2, $this->map->height / 2);
        $geoExt = $this->map->extent;
        $maxExt = new rectObj();
        // $maxExt->setExtent(...)
        $maxExt->minx = $geoExt->minx - 10;
        $maxExt->miny = $geoExt->miny - 10;
        $maxExt->maxx = $geoExt->maxx + 10;
        $maxExt->maxy = $geoExt->maxy + 10;

        $result = $this->map->zoomPoint(2, $pixPos, $this->map->width, $this->map->height, $geoExt, $maxExt);
        $this->assertEquals(MS_SUCCESS, $result);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->map);
    }
}
