<?php

class pointObjTest extends \PHPUnit\Framework\TestCase
{
    protected $point;

    public function setUp(): void
    {
        $this->point = new pointObj();
        $this->point->setXY(10, 15);
    }

    public function testDistanceToLine()
    {
        $startLine = new pointObj();
        $startLine->setXY(11, 15);
        $endLine = new pointObj();
        $endLine->setXY(43, 67);
        $this->assertEquals(1.0, $this->point->distanceToSegment($startLine, $endLine));
        $startLine->setXY(10, 15);
        $this->assertEquals(0, $this->point->distanceToSegment($startLine, $endLine));
    }
}

?>
