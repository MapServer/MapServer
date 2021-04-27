<?php

class ShapeFileObjTest extends \PHPUnit\Framework\TestCase
{
    protected $shapeFile;

    public function setUp(): void
    {
        $this->shapeFile = new shapeFileObj('maps/data/lakes2.shp', -2);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'isopen' is read-only
     */
    public function test__setIsOpen()
    {
        $this->shapeFile->isopen = 1;
    }

    public function test__getIsOpen()
    {
        $this->assertEquals(1, $this->shapeFile->isopen);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'lastshape' is read-only
     */
    public function test__setLastShape()
    {
        $this->shapeFile->lastshape = 5;
    }

    public function test__getLastShape()
    {
        //$shape = ms_shapeObjFromWkt('POINT(-126.4 45.32)');
        //$this->shapeFile->addShape($shape);
        //echo "\n"."numshapes: ".$this->shapeFile->numshapes;
        $this->shapeFile->getShape(5)->toWkt();
        $this->assertEquals(5, $this->shapeFile->lastshape);

    }
}

?>
