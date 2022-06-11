<?php

class labelleaderObjTest extends \PHPUnit\Framework\TestCase
{
    protected $labelleader;

    public function setUp(): void
    {
        $map_file = 'maps/labels-leader.map';
        $map = new mapObj($map_file);
        $layer = $map->getLayer(0);
        $class = $layer->getClass(0);
        $label = $class->getLabel(0);
        $this->labelleader = $class->leader;
    }

    public function test__getmaxdistance()
    {
        $this->assertEquals(30, $this->labelleader->maxdistance);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'maxdistance' is read-only
     */
    public function test__setmaxdistance()
    {
        $this->labelleader->maxdistance = 35;
    }

    public function test__getgridstep()
    {
        $this->assertEquals(5, $this->labelleader->gridstep);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'gridstep' is read-only
     */
    public function test__setgridstep()
    {
        $this->labelleader->gridstep = 35;
    }

    

}

?>
