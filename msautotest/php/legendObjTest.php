<?php

class legendObjTest extends \PHPUnit\Framework\TestCase
{
    protected $legend;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
        $this->legend = $this->map->legend;
    }

    public function testConvertToString()
    {
        $str = $this->legend->convertToString();
        $this->assertIsString($str);
        $this->assertStringContainsString('LEGEND', $str);
    }

    public function testUpdateFromString()
    {
        $this->assertEquals(MS_SUCCESS, $this->legend->updateFromString('LEGEND STATUS ON END'));
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->legend, $this->map);
    }
}

?>
