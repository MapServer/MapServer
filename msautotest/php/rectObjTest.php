<?php

class rectObjTest extends \PHPUnit\Framework\TestCase
{
    protected $rect;

    public function setUp(): void
    {
        $this->rect = new rectObj();
    }

    public function testgetCenter()
    {
        $this->rect->setextent(0,0,5,5);
        $this->assertEquals('5.0', $this->rect->getCenter()->x + $this->rect->getCenter()->y);
    }
}

?>
