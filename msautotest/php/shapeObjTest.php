<?php

class shapeObjTest extends \PHPUnit\Framework\TestCase
{
    protected $shape;

    public function setUp(): void
    {
        $this->shape = new shapeObj(MS_SHAPE_POINT);
    }

    public function testdistanceToPoint()
    {
        $point = new pointObj();
        $point->setXY(5, 8);
        $line = new lineObj();
        $line->add(new pointObj(0,0));
        $line->add(new pointObj(6,8));

        $this->assertTrue(is_nan($this->shape->distanceToPoint($point)));
        $this->shape->add($line);
        $this->assertEquals(1.0, $this->shape->distanceToPoint($point));
    }

    public function testdistanceToShape()
    {
        $shape2 = new shapeObj(MS_SHAPE_POINT);

        $this->assertEquals(-1, $this->shape->distanceToShape($shape2));

        $line = new lineObj();
        $line->add(new pointObj(0,0));
        $line->add(new pointObj(4,4));
        $this->shape->add($line);
        $line2 = new lineObj();
        $line2->add(new pointObj(2,2));
        $line2->add(new pointObj(3,5));
        $shape2->add($line2);

        $this->assertEquals(1.4142135623731, $this->shape->distanceToShape($shape2));
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'resultindex' is read-only
     */
    public function test__setresultindex()
    {
        # exception not thrown with PHPNG
        #$this->shape->resultindex = 18;
    }

    public function test__getresultindex()
    {
        $this->assertEquals(-1, $this->shape->resultindex);
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($shape, $this->shape, $point, $line, $line2, $shape2);
    }    

}

?>
