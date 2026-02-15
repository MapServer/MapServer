<?php

class scalebarObjTest extends \PHPUnit\Framework\TestCase
{
    protected $scalebar;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
        $this->scalebar = $this->map->scalebar;
    }

    public function testConvertToString()
    {
        $str = $this->scalebar->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('SCALEBAR', $str);
    }

    public function testUpdateFromString()
    {
        $this->assertEquals(MS_SUCCESS, $this->scalebar->updateFromString('SCALEBAR STATUS ON END'));
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->scalebar, $this->map);
    }
}

?>
