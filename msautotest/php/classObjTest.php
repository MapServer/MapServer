<?php

class classObjTest extends \PHPUnit\Framework\TestCase
{
    protected $class;

    public function setUp(): void
    {
        $map_file = 'maps/labels-leader.map';
        $map = new mapObj($map_file);
        $layer = $map->getLayer(0);
        $this->class = $layer->getClass(0);
    }

    public function testinsertremoveStyle()
    {

        $map = new mapObj('maps/labels.map');
        $layer = $map->getLayer(1);
        $classtmp = $layer->getClass(0);
        $style = $classtmp->removeStyle(0);
        $this->assertEquals(0, $this->class->insertStyle($style));


        $this->assertInstanceOf('styleObj', $this->class->removeStyle(0));

    }

    public function testleader()
    {
        $this->assertInstanceOf('labelLeaderObj', $this->class->leader);
    }
    
    public function testcreateLegendIcon()
    {
        $map_file = 'maps/labels-leader.map';
        $map = new mapObj($map_file);
        $layer = $map->getLayer(0);       
        $this->assertInstanceOf('imageObj', $layer->getClass(0)->createLegendIcon( $map, $layer, 50, 50 ));
    }

    public function testClone()
    {
        $this->assertInstanceOf('classObj', $newClass = $this->class->cloneClass());
    }    
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($this->class, $map_file, $map, $layer, $classtmp, $style, $newClass);
    }
    
}

?>
