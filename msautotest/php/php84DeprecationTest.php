<?php

/**
 * Tests for PHP 8.4+ compatibility with MapServer mapscript extension.
 *
 * PHP 8.4 deprecated E_STRICT and implicit nullable parameter types.
 * PHP 8.5 further tightens type handling. These tests verify that
 * the SWIG-generated mapscript extension works without triggering
 * deprecation notices or errors on PHP 8.4+.
 *
 * @group php84
 */
class php84DeprecationTest extends \PHPUnit\Framework\TestCase
{
    protected $map;
    protected $map_file = 'maps/ogr_query.map';
    protected $previousErrorHandler;
    protected $deprecationNotices = [];

    public function setUp(): void
    {
        if (PHP_VERSION_ID < 80400) {
            $this->markTestSkipped('These tests target PHP 8.4+ deprecation behavior');
        }

        $this->deprecationNotices = [];

        // Capture any deprecation notices emitted by the extension
        $this->previousErrorHandler = set_error_handler(function ($errno, $errstr, $errfile, $errline) {
            if ($errno === E_DEPRECATED || $errno === E_USER_DEPRECATED) {
                $this->deprecationNotices[] = $errstr;
                return true;
            }
            return false;
        });

        $this->map = new mapObj($this->map_file);
    }

    /**
     * Verify that creating core mapscript objects emits no deprecation notices.
     */
    public function testObjectCreationNoDeprecations()
    {
        $point = new pointObj();
        $point->setXY(10, 20);

        $line = new lineObj();
        $line->add($point);

        $shape = new shapeObj(MS_SHAPE_POINT);
        $shape->add($line);

        $rect = new rectObj();
        $rect->minx = 0;
        $rect->miny = 0;
        $rect->maxx = 100;
        $rect->maxy = 100;

        $color = new colorObj(255, 0, 0);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Object creation triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that map operations emit no deprecation notices.
     */
    public function testMapOperationsNoDeprecations()
    {
        $numLayers = $this->map->numlayers;
        $this->assertGreaterThanOrEqual(1, $numLayers);

        $layer = $this->map->getLayer(0);
        $this->assertInstanceOf('layerObj', $layer);

        $extent = $this->map->extent;
        $this->assertInstanceOf('rectObj', $extent);

        $version = $this->map->web->metadata->get('wms_title');

        $this->assertEmpty(
            $this->deprecationNotices,
            'Map operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that layer querying emits no deprecation notices.
     */
    public function testQueryOperationsNoDeprecations()
    {
        $this->map->queryByRect($this->map->extent);

        $layer = $this->map->getLayer(0);
        $numResults = $layer->getNumResults();
        $this->assertGreaterThanOrEqual(0, $numResults);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Query operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that drawing operations emit no deprecation notices.
     */
    public function testDrawOperationsNoDeprecations()
    {
        $image = $this->map->draw();
        $this->assertInstanceOf('imageObj', $image);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Draw operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that output format handling emits no deprecation notices.
     */
    public function testOutputFormatNoDeprecations()
    {
        $format = new outputFormatObj('AGG/PNG', 'testFormat');
        $this->map->appendOutputFormat($format);
        $retrieved = $this->map->getOutputFormat(0);
        $this->assertInstanceOf('OutputFormatObj', $retrieved);
        $this->map->removeOutputFormat('testFormat');

        $this->assertEmpty(
            $this->deprecationNotices,
            'Output format operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that projection operations emit no deprecation notices.
     */
    public function testProjectionNoDeprecations()
    {
        $proj = new projectionObj('init=epsg:4326');
        $this->assertInstanceOf('projectionObj', $proj);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Projection operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that shape geometry operations emit no deprecation notices.
     */
    public function testShapeGeometryNoDeprecations()
    {
        $shape = new shapeObj(MS_SHAPE_POINT);
        $line = new lineObj();
        $line->add(new pointObj(0, 0));
        $line->add(new pointObj(10, 10));
        $shape->add($line);

        $bounds = $shape->bounds;
        $this->assertInstanceOf('rectObj', $bounds);

        $cloned = $shape->cloneShape();
        $this->assertInstanceOf('shapeObj', $cloned);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Shape geometry operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that hashtable operations emit no deprecation notices.
     */
    public function testHashtableNoDeprecations()
    {
        $layer = $this->map->getLayer(0);
        $layer->metadata->set('test_key', 'test_value');
        $val = $layer->metadata->get('test_key');
        $this->assertEquals('test_value', $val);
        $layer->metadata->remove('test_key');

        $this->assertEmpty(
            $this->deprecationNotices,
            'Hashtable operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that class/style operations emit no deprecation notices.
     */
    public function testClassStyleNoDeprecations()
    {
        $layer = $this->map->getLayer(0);
        $class = $layer->getClass(0);
        $this->assertInstanceOf('classObj', $class);

        $style = $class->getStyle(0);
        $this->assertInstanceOf('styleObj', $style);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Class/style operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify GEOS geometry operations emit no deprecation notices.
     * These exercise SWIG's zend_throw_exception code paths.
     */
    public function testGEOSOperationsNoDeprecations()
    {
        $poly = shapeObj::fromWKT('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))');
        if ($poly === null) {
            $this->markTestSkipped('GEOS support not available');
        }

        $point = shapeObj::fromWKT('POINT (5 5)');
        $poly->contains($point);
        $poly->intersects($point);
        $poly->getCentroid();
        $poly->buffer(1.0);
        $poly->convexHull();
        $poly->getArea();
        $poly->toWKT();

        $this->assertEmpty(
            $this->deprecationNotices,
            'GEOS operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify image rendering pipeline emits no deprecation notices.
     */
    public function testImagePipelineNoDeprecations()
    {
        $image = $this->map->draw();
        $bytes = $image->getBytes();
        $this->assertNotEmpty($bytes);
        $size = $image->getSize();
        $this->assertGreaterThan(0, $size);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Image pipeline triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify msGetVersion works without deprecations.
     */
    public function testVersionFunctionNoDeprecations()
    {
        $version = msGetVersion();
        $this->assertIsString($version);
        $this->assertStringContainsString('MapServer', $version);

        $this->assertEmpty(
            $this->deprecationNotices,
            'msGetVersion triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that error handling operations emit no deprecation notices.
     * Exercises zend_throw_exception code paths.
     */
    public function testErrorHandlingNoDeprecations()
    {
        $error = new errorObj();
        $this->assertInstanceOf('errorObj', $error);
        $error->code;
        $error->routine;
        $error->message;
        $error->next();

        msResetErrorList();

        $str = msGetErrorString("\n");
        $this->assertTrue(is_string($str) || is_null($str));

        $this->assertEmpty(
            $this->deprecationNotices,
            'Error handling triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that IO buffering operations emit no deprecation notices.
     * Exercises gdBuffer typemap using RETVAL_STRINGL.
     */
    public function testIOBufferingNoDeprecations()
    {
        msIO_installStdoutToBuffer();

        $request = new OWSRequest();
        $request->setParameter('SERVICE', 'WMS');
        $request->setParameter('VERSION', '1.1.1');
        $request->setParameter('REQUEST', 'GetCapabilities');

        $wmsMap = new mapObj('maps/ows_wms.map');
        @$wmsMap->OWSDispatch($request);

        $str = msIO_getStdoutBufferString();
        $this->assertIsString($str);

        msIO_resetHandlers();

        $this->assertEmpty(
            $this->deprecationNotices,
            'IO buffering triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that zoom operations emit no deprecation notices.
     * Exercises float type coercion through SWIG.
     */
    public function testZoomOperationsNoDeprecations()
    {
        $pixPos = new pointObj();
        $pixPos->setXY($this->map->width / 2, $this->map->height / 2);
        $geoExt = $this->map->extent;

        $this->map->zoomPoint(2, $pixPos, $this->map->width, $this->map->height, $geoExt, null);
        $this->map->zoomScale(50000000, $pixPos, $this->map->width, $this->map->height, $geoExt, null);

        $pixRect = new rectObj();
        $pixRect->minx = 50;
        $pixRect->miny = 150;
        $pixRect->maxx = 150;
        $pixRect->maxy = 50;
        $this->map->zoomRectangle($pixRect, $this->map->width, $this->map->height, $geoExt, null);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Zoom operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that serialization roundtrips emit no deprecation notices.
     * Exercises string handling throughout the SWIG layer.
     */
    public function testSerializationNoDeprecations()
    {
        $layer = $this->map->getLayer(0);
        $layerStr = $layer->convertToString();
        $this->assertIsString($layerStr);

        $class = $layer->getClass(0);
        $classStr = $class->convertToString();
        $this->assertIsString($classStr);

        $style = $class->getStyle(0);
        $styleStr = $style->convertToString();
        $this->assertIsString($styleStr);

        $web = $this->map->web;
        $webStr = $web->convertToString();
        $this->assertIsString($webStr);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Serialization triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that cluster operations emit no deprecation notices.
     */
    public function testClusterOperationsNoDeprecations()
    {
        $layer = $this->map->getLayer(0);
        $cluster = $layer->cluster;
        $this->assertInstanceOf('clusterObj', $cluster);

        $clusterStr = $cluster->convertToString();
        $this->assertIsString($clusterStr);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Cluster operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    /**
     * Verify that symbol set operations emit no deprecation notices.
     */
    public function testSymbolSetOperationsNoDeprecations()
    {
        $symbolSet = $this->map->symbolset;
        $this->assertInstanceOf('symbolSetObj', $symbolSet);

        $numSymbols = $symbolSet->numsymbols;
        $this->assertGreaterThan(0, $numSymbols);

        $symbol = $symbolSet->getSymbol(0);
        $this->assertInstanceOf('symbolObj', $symbol);

        $this->assertEmpty(
            $this->deprecationNotices,
            'Symbol set operations triggered deprecation notices: ' . implode('; ', $this->deprecationNotices)
        );
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        if ($this->previousErrorHandler !== null) {
            restore_error_handler();
        }
        unset($this->map, $this->deprecationNotices, $this->previousErrorHandler);
    }
}
