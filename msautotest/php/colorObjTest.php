<?php

class ColorObjTest extends PHPUnit_Framework_TestCase
{
    protected $color;

    public function setUp()
    {
        $map_file = 'maps/labels.map';
        $map = new mapObj($map_file);
        $this->color = $map->getLayer(0)->getClass(0)->getLabel(0)->color;
    }

    public function test__getsetAlpha()
    {
        $this->assertEquals(50, $this->color->alpha=50);
    }

}
?>
