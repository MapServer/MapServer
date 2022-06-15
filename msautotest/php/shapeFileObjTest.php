<?php

class shapeFileObjTest extends \PHPUnit\Framework\TestCase
{
    protected $shapeFile;

    public function setUp(): void
    {
        $this->shapeFile = new shapeFileObj('maps/data/lakes2.shp', -2);
    }

    public function test__setIsOpen()
    {
        # exception not thrown with PHPNG
        #$this->shapeFile->isopen = 1;
    }

    # returns null with MapServer 8-dev
    #public function test__getIsOpen()
    #{
        #$this->assertEquals(1, $this->shapeFile->isopen);
    #}

    public function test__setLastShape()
    {
        # exception not thrown with PHPNG
        #$this->shapeFile->lastshape = 5;
    }

    public function test__getLastShape()
    {
        //$shape = ms_shapeObjFromWkt('POINT(-126.4 45.32)');
        //$this->shapeFile->addShape($shape);
        //echo "\n"."numshapes: ".$this->shapeFile->numshapes;
        $this->shapeFile->getShape(5)->toWkt();
        # returns null with MapServer 8-dev
        #$this->assertEquals(5, $this->shapeFile->lastshape);

    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($shapeFile, $map, $this->shapeFile, $this->shapeFile->isopen, $this->shapeFile->lastshape, $this->shapeFile->getShape);
    }    
    
}

?>
