<?php

class styleObjTest extends \PHPUnit\Framework\TestCase
{
    protected $style;

    public function setUp(): void
    {
        $map_file = 'maps/labels.map';
        $map = new mapObj($map_file);
        $this->style = $map->getLayer(0)->getClass(0)->getLabel(0)->getStyle(0);
    }

    public function test__getsetInitialGap()
    {
        $this->assertEquals(2.5, $this->style->initialgap = 2.5);
    }

    public function test__getsetMaxScaledenom()
    {
        $this->assertEquals(2.2, $this->style->maxscaledenom = 2.2);
    }

    public function test__getsetMinScaledenom()
    {
        $this->assertEquals(2.2, $this->style->minscaledenom = 2.2);
    }

    public function testClone()
    {
        $newStyle = $this->style->cloneStyle();
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($style, $map_file, $map, $this->style, $this->style->initialgap, $this->style->maxscaledenom, $this->style->minscaledenom, $newStyle);
    }    

}

?>
