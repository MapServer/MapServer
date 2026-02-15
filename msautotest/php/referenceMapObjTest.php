<?php

class referenceMapObjTest extends \PHPUnit\Framework\TestCase
{
    protected $referencemap;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
        $this->referencemap = $this->map->reference;
    }

    public function testConvertToString()
    {
        $str = $this->referencemap->convertToString();
        $this->assertIsString($str);
    }

    public function testUpdateFromString()
    {
        $this->assertEquals(MS_SUCCESS, $this->referencemap->updateFromString('REFERENCE STATUS ON END'));
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->referencemap, $this->map);
    }
}

?>
