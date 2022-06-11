<?php

class mapObjTest extends \PHPUnit\Framework\TestCase
{
    protected $map;
    protected $map_file = 'maps/ogr_query.map';

    public function setUp(): void
    {
        $this->map = new mapObj($this->map_file);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'numoutputformats' is read-only
     */
    public function test__setnumoutputformats()
    {
        $this->map->numoutputformats = 2;
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

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'imagetype' is read-only
     */
    public function test__setImageType()
    {
        $this->map->imagetype = 'jpg';
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

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'numlayers' is read-only
     */
    public function test__setNumlayers()
    {
        $this->map->numlayers = 2;
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($map, $map_file, $this->map_file, $this->map->numoutputformats, $this->map->numlayers, $this->map->imagetype, $this->map->queryByRect, $this->map->saveQueryAsGML);
    }     

}

?>
