<?php

class layerObjTest extends \PHPUnit\Framework\TestCase
{
    protected $layer;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/filter.map');
        $this->layer = $this->map->getLayer(0);
    }

    public function testSetFilter()
    {
        $this->assertEquals(0, $this->layer->setFilter("('[CTY_NAME]' = 'Itasca')"));
        $this->assertEquals("('[CTY_NAME]' = 'Itasca')", $this->layer->getFilterString());
        $this->layer->queryByRect($this->map, $this->map->extent);
        $this->assertEquals(1,$this->layer->getNumResults());
        $this->assertEquals(0, $this->layer->setFilter("('[CTY_ABBR]' = 'BECK')"));
        $this->layer->queryByRect($this->map, $this->map->extent);
        $this->assertEquals(1,$this->layer->getNumResults());
        $this->assertEquals(0, $this->layer->setFilter("('[WRONG]' = 'wrong')"));
        //handle expected exception error
        $this->expectException(Exception::class);        
        @$this->layer->queryByRect($this->map, $this->map->extent);
        $this->assertEquals(0,$this->layer->getNumResults());
    }

    public function testqueryByFilter()
    {
        $this->assertEquals(0, $this->layer->queryByFilter($this->map, "'[CTY_NAME]' = 'Itasca'"));
        $this->assertEquals(1, $this->layer->getNumResults());
        $this->assertEquals(0, $this->layer->queryByFilter($this->map, "'[CTY_NAME]' = 'Inocity'"));
        $this->assertEquals(0, $this->layer->getNumResults());
    }

    public function testqueryByIndex()
    {
        $this->layer->queryByIndex($this->map, -1, 0, MS_FALSE);
        $this->assertEquals(1, $this->layer->getNumResults());
        $this->layer->queryByIndex($this->map, -1, 0, MS_TRUE);
        $this->assertEquals(2, $this->layer->getNumResults());
    }

    public function testsetExtent()
    {
        $this->assertEquals(0, $this->layer->setExtent(1,1,3,3));
    }

    public function testsetProcessingKey()
    {
        # setProcessing() is not available in PHPNG
        #$this->layer->setProcessing("LABEL_NO_CLIP=True");
        $this->layer->setProcessingKey("LABEL_NO_CLIP", "False");
        $this->assertEquals("False", $this->layer->getProcessingKey("LABEL_NO_CLIP"));
    }

    public function test__setNumJoins()
    {
        # exception not thrown with PHPNG
        #$this->layer->numjoins = 5;
    }

    public function test__getNumJoins()
    {
        $this->assertEquals(0, $this->layer->numjoins);
    }

    public function test__setExtent()
    {
        # exception not thrown with PHPNG
        #$this->layer->extent = array(124500, 4784000, 788500, 5488988); // it would make senses if it were supposed to work
    }

    /**
     * Test on the property and not the function
     */
    public function test__getExtent()
    {
        $this->assertInstanceOf('rectObj', $this->layer->extent);
    }

    public function test__getMaxClasses()
    {
        $this->assertEquals(8, $this->layer->maxclasses);
    }

    public function test__setMaxClasses()
    {
        # exception not thrown with PHPNG
        #$this->layer->maxclasses = 10;
    }

    public function test__setgetMaxGeoWidth()
    {
        $this->assertEquals(2.2, $this->layer->maxgeowidth = 2.2);
    }

    public function test__setgetMinGeoWidth()
    {
        $this->assertEquals(2.5, $this->layer->mingeowidth = 2.5);
    }

    public function test__setNumItems()
    {
        # exception not thrown with PHPNG
        #$this->layer->numitems = 10;
    }

    public function test__getNumItems()
    {
        $this->assertEquals(0, $this->layer->numitems);
    }

    public function test__setgetBandsItem()
    {
        $this->assertEquals("4,2,1", $this->layer->bandsitem = "4,2,1");
    }

    public function testClone()
    {
        $this->assertInstanceOf('layerObj', $newLayer = $this->layer->cloneLayer());
    }

}

?>
