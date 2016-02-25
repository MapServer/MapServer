<?php

class LabelObjTest extends PHPUnit_Framework_TestCase
{
    protected $label;

    public function setUp()
    {
        $map_file = 'maps/labels-leader.map';
        $map = new mapObj($map_file);
        $this->label = $map->getLayer(0)->getClass(0)->getLabel(0);
    }

    public function testInsertRemoveStyle()
    {
        $this->assertInstanceOf('styleObj', $style = $this->label->removeStyle(0));
        $this->assertEquals(1, $this->label->insertStyle($style));
    }

    public function test__setgetMinMaxScaledenom()
    {
        $this->assertEquals(5, $this->label->minscaledenom = 5);
        $this->assertEquals(5, $this->label->maxscaledenom = 5);

    }

    public function testClone()
    {
        $this->assertInstanceOf('labelObj', $newLabel = clone $this->label);
    }

}

?>
