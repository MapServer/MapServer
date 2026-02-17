<?php

/**
 * Tests convertToString/updateFromString roundtrips for all objects
 * that support serialization. Verifies string handling across PHP versions.
 */
class serializationTest extends \PHPUnit\Framework\TestCase
{
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/labels.map');
    }

    public function testLayerConvertAndUpdate()
    {
        $layer = $this->map->getLayer(0);
        $str = $layer->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('LAYER', $str);

        // Update with a valid snippet
        $this->assertEquals(MS_SUCCESS, $layer->updateFromString('LAYER NAME "updated_layer" END'));
        $this->assertEquals('updated_layer', $layer->name);
    }

    public function testClassConvertAndUpdate()
    {
        $layer = $this->map->getLayer(0);
        $class = $layer->getClass(0);
        $str = $class->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('CLASS', $str);

        $this->assertEquals(MS_SUCCESS, $class->updateFromString('CLASS NAME "test_class" END'));
        $this->assertEquals('test_class', $class->name);
    }

    public function testStyleConvertAndUpdate()
    {
        $layer = $this->map->getLayer(0);
        $class = $layer->getClass(0);
        $label = $class->getLabel(0);
        $style = $label->getStyle(0);
        $str = $style->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('STYLE', $str);
    }

    public function testLabelConvertAndUpdate()
    {
        $layer = $this->map->getLayer(0);
        $class = $layer->getClass(0);
        $label = $class->getLabel(0);
        $str = $label->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('LABEL', $str);
    }

    public function testWebConvertAndUpdate()
    {
        $web = $this->map->web;
        $str = $web->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('WEB', $str);
    }

    public function testQueryMapConvertAndUpdate()
    {
        $qm = $this->map->querymap;
        $str = $qm->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('QUERYMAP', $str);
    }

    public function testScalebarRoundtrip()
    {
        $scalebar = $this->map->scalebar;
        $str = $scalebar->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('SCALEBAR', $str);
        $this->assertEquals(MS_SUCCESS, $scalebar->updateFromString($str));
    }

    public function testLegendRoundtrip()
    {
        $legend = $this->map->legend;
        $str = $legend->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('LEGEND', $str);
        $this->assertEquals(MS_SUCCESS, $legend->updateFromString($str));
    }

    public function testReferenceMapRoundtrip()
    {
        $ref = $this->map->reference;
        $str = "REFERENCE IMAGE 'foo.png' EXTENT 0 0 100 100 STATUS ON END";
        $this->assertEquals(MS_SUCCESS, $ref->updateFromString($str));
        $str = $ref->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString("REFERENCE", $str);
        $this->assertEquals(MS_SUCCESS, $ref->updateFromString($str));
    }

    public function testClusterRoundtrip()
    {
        $layer = $this->map->getLayer(0);
        $cluster = $layer->cluster;
        $str = "CLUSTER MAXDISTANCE 20 BUFFER 5 REGION 'ellipse' END";
        $this->assertEquals(MS_SUCCESS, $cluster->updateFromString($str));
        $str = $cluster->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString("CLUSTER", $str);
        $this->assertEquals(MS_SUCCESS, $cluster->updateFromString($str));
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->map);
    }
}
