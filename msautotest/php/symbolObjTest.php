<?php

class symbolObjTest extends \PHPUnit\Framework\TestCase
{
    protected $symbol;

    public function setUp(): void
    {
        $map = new mapObj('maps/labels.map');
        $this->symbol = $map->getSymbolObjectById($map->getSymbolByName("plant"));
    }

    public function testSetGetImage()
    {
        $outputFrmt = new outputFormatObj('AGG/PNG', 'theName');
        //$this->assertInstanceOf('imageObj', $image = $this->symbol->getImage($outputFrmt));
        $image = $this->symbol->getImage($outputFrmt);
        $this->symbol->setImage($image);
    }

    public function test__setgetAnchorpoint_x()
    {
        $this->assertEquals(5.5, $this->symbol->anchorpoint_x = 5.5);
    }

    public function test__setgetAnchorpooint_y()
    {
        $this->assertEquals(2.2, $this->symbol->anchorpoint_y = 2.2);
    }

    public function test_setgetMaxx()
    {
        $this->assertEquals(100.555, $this->symbol->maxx = 100.555);
    }

    public function test_setgetMaxy()
    {
        $this->assertEquals(555.100, $this->symbol->maxy = 555.100);
    }

    public function test_setgetMinx()
    {
        $this->assertEquals(0.5, $this->symbol->minx = 0.5);
    }

    public function test_setgetMiny()
    {
        $this->assertEquals(5.0, $this->symbol->minx = 5.0);
    }
}

?>
