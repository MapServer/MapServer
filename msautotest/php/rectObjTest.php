<?php

class rectObjTest extends \PHPUnit\Framework\TestCase
{
    protected $rect;

    public function setUp(): void
    {
        $this->rect = new rectObj(0,0,5,5);
    }

    public function testgetCenter()
    {
        $this->assertEquals('5.0', $this->rect->getCenter()->x + $this->rect->getCenter()->y);
    }
}

?>
