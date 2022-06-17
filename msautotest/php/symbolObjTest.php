<?php

class symbolObjTest extends \PHPUnit\Framework\TestCase
{
    protected $symbol;
    protected $map;
    protected $outputFrmt;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/labels.map');
        $this->symbol = $this->map->symbolset->getSymbolByName("plant");
        $this->outputFrmt = new outputFormatObj('AGG/PNG', 'theName');
    }

    public function testSetGetImage()
    {
        //$this->assertInstanceOf('imageObj', $image = $this->symbol->getImage($outputFrmt));
        $image = $this->symbol->getImage($this->outputFrmt);
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
        $this->assertEquals(5.0, $this->symbol->miny = 5.0);
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($this->map);
        unset($this->symbol);
        unset($this->outputFrmt);
    }    
    
}

?>
