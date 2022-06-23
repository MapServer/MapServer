<?php

class labelObjTest extends \PHPUnit\Framework\TestCase
{
    protected $label;

    public function setUp(): void
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

    # label->clone() method not available in MapServer 8-dev
    #public function testClone()
    #{
        #$this->assertInstanceOf('labelObj', $newLabel = $this->label->clone());
    #}
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($label, $map_file, $map, $this->label);
    }

}

?>
