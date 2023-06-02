<?php

class mapObjTest extends \PHPUnit\Framework\TestCase
{
    protected $map;
    protected $map_file = 'maps/ogr_query.map';

    public function setUp(): void
    {
        $this->map = new mapObj($this->map_file);
    }

    public function test__setnumoutputformats()
    {
        # exception not thrown with PHPNG
        #$this->map->numoutputformats = 2;
    }

    // Also testing __get numoutputformats
    public function testOutputFormat()
    {

        $outputFrmt = new outputFormatObj('AGG/PNG', 'theName');
        $this->assertEquals(2,$this->map->appendOutputFormat($outputFrmt));
        $this->assertEquals(2, $this->map->numoutputformats);
        $this->assertInstanceOf('OutputFormatObj', $this->map->getOutputFormat(0));
        $this->assertEquals(0, $this->map->removeOutputFormat('theName'));
        $this->assertEquals(1, $this->map->numoutputformats);
    }

    public function testQueryByFilter()
    {
        $map_file = 'maps/filtermap.map';
        $map = new mapObj($map_file);
        $this->assertEquals(0, $map->queryByFilter("'[CTY_NAME]' = 'Itasca'"));
    }

    public function testConfigOption()
    {
        $this->assertEquals(0, $this->map->setConfigOption('DEBUG', '5'));
        $this->assertEquals('5',$this->map->getConfigOption('DEBUG'));
    }

    public function testSaveQueryAsGML()
    {
        $this->map->queryByRect($this->map->extent);
        $this->map->saveQueryAsGML("gml.gml");
    }

    public function test__setImageType()
    {
        # exception not thrown with PHPNG
        #$this->map->imagetype = 'jpg';
    }

    public function test__getImageType()
    {
        $this->assertEquals('png', $this->map->imagetype);
    }

    public function testClone()
    {
        $this->assertInstanceOf('mapObj', $newMap = $this->map->cloneMap());
    }

    public function test__getNumlayers()
    {
        $this->assertEquals(1, $this->map->numlayers);
    }

    public function test__setNumlayers()
    {
        # exception not thrown with PHPNG
        #$this->map->numlayers = 2;
    }
    
    //also testing multiple setProjection cache issue #6896
    public function test__setProjection()
    {
        $map_file = 'maps/reprojection.map';
        $map = new mapObj($map_file);
        $this->assertEquals(0, $map->setProjection('init=epsg:3857'), 'Failed to set MAP projection to EPSG:3857');
        $map->setExtent(-7913606, 5346829, -6120700, 6522760);
        $layer = $map->getLayerByName('ns-places');
        $this->assertEquals(0, $layer->setProjection('init=epsg:4326'), 'Failed to set LAYER projection to EPSG:4326');
        $image4326 = $map->draw();
        $image4326->save('./result/setprojection-3857.png');
        //test multiple reprojections
        $this->assertEquals(0, $map->setProjection('init=epsg:3978'), 'Failed to set MAP projection to EPSG:3978');
        $map->setExtent(1394924, -279190, 3376084, 1020214);
        $image3978 = $map->draw();
        $image3978->save('./result/setprojection-3978.png');
        //compare the EPSG:3857 map image
        $expectedImage3857 = './expected/setprojection-3857.png';
        $resultImage3857 = './result/setprojection-3857.png';
        $this->assertFileEquals($expectedImage3857 , $resultImage3857, 'Result setProjection EPSG:3857 map image is not same as Expected');
        //compare the EPSG:3978 map image
        $expectedImage3978 = './expected/setprojection-3978.png';
        $resultImage3978 = './result/setprojection-3978.png';
        $this->assertFileEquals($expectedImage3978 , $resultImage3978, 'Result setProjection EPSG:3978 map image is not same as Expected');
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($map, $map_file);
        unset($image4326, $image3978);
        unset($expectedImage3857, $resultImage3857);
        unset($expectedImage3978, $resultImage3978);
        unset($this->map_file, $this->map->numoutputformats);
        unset($this->map->numlayers, $this->map->imagetype);
        unset($this->map->queryByRect, $this->map->saveQueryAsGML);
    }     

}

?>
