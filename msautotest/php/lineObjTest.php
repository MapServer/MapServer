<?php

class LineObjTest extends \PHPUnit\Framework\TestCase
{
    protected $line;

    public function setUp(): void
    {
        $this->line = new lineObj();
        $start = new pointObj;
        $start->setXY(1,1);
        $middle = new pointObj();
        $middle->setXY(2,1);
        $end = new pointObj();
        $end->setXY(3,3);
        $this->line->add($start);
        $this->line->add($middle);
        $this->line->add($end);
    }

    /**
     * get() is an alias of point()
     */
    public function testSetGet()
    {
        $point = new pointObj();
        $point->setXY(5,5);
        $this->assertEquals(0, $this->line->set(1, $point));
        $this->assertInstanceOf('pointObj', $replacedPoint = $this->line->get(1));
        $this->assertEquals(5, $replacedPoint->x);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'numpoints' is read-only
     */
    public function test__setNumPoints()
    {
        $this->line->numpoints = 5;
    }

    public function test__getNumPoints()
    {
        $this->assertEquals(3, $this->line->numpoints);
    }

    public function testClone()
    {
        $newline = clone $this->line;
    }

}

?>
