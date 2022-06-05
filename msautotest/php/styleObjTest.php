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

    public function testsClone()
    {
        $newStyle = clone $this->style;
    }

}

?>
