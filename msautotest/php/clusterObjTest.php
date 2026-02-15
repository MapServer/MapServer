<?php

class clusterObjTest extends \PHPUnit\Framework\TestCase
{
    protected $cluster;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
        $layer = $this->map->getLayer(0);
        $this->cluster = $layer->cluster;
    }

    public function testConvertToString()
    {
        $str = $this->cluster->convertToString();
        $this->assertIsString($str);
    }

    public function testSetGetFilter()
    {
        $this->assertEquals(MS_SUCCESS, $this->cluster->setFilter("('[population]' > '1000')"));
        $filter = $this->cluster->getFilterString();
        $this->assertIsString($filter);
    }

    public function testSetGetGroup()
    {
        $this->assertEquals(MS_SUCCESS, $this->cluster->setGroup("('[region]')"));
        $group = $this->cluster->getGroupString();
        $this->assertIsString($group);
    }

    public function testClearFilter()
    {
        $this->assertEquals(MS_SUCCESS, $this->cluster->setFilter(''));
    }

    public function testProperties()
    {
        $this->assertIsFloat($this->cluster->maxdistance + 0.0);
        $this->assertIsFloat($this->cluster->buffer + 0.0);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->cluster, $this->map);
    }
}

?>
