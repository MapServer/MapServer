<?php
/* 
PURPOSE:     test reprojectionObj through MapScript
SOURCE DATA: OSM places for Nova Scotia, 3843 points
             in EPSG:4326
DATA FORMAT: SpatiaLite database
*/
class reprojectionObjTest extends \PHPUnit\Framework\TestCase
{
    protected $reprojector;
    protected $map;
    protected $layer;
    protected $shape;
    protected $sourceProjection;
    protected $outputProjection;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/reprojection.map');
        $this->layer = $this->map->getLayer(0);
        $this->shape = new shapeObj(MS_SHAPE_POINT);
        //$this->sourceProjection = new projectionObj("proj=latlong");
        $this->sourceProjection = new projectionObj("EPSG:4326");
        $this->outputProjection = new projectionObj("EPSG:3857");        
    }

    public function testReprojectionInstance()
    {

        $this->assertInstanceOf("reprojectionObj", new reprojectionObj( $this->sourceProjection,  $this->outputProjection ));
    }

    public function testProjectMethodSpeed()
    {
        $reprojector = new reprojectionObj( $this->sourceProjection,  $this->outputProjection );        
        
        $this->layer->open();
        $i = 0;
        
        $start = microtime(true);
        while (($shape = $this->layer->nextShape()) != null) {
          $point = $shape->getCentroid();
          $point->project($reprojector);
          //check that the first shape reprojects correctly, to EPSG:3857
          if ($i == 0) {
            $this->assertStringContainsString("'x': -7078355.430891216, 'y': 5566372.173407509", $point->toString());
          }
          $i++;          
        }
        $time_elapsed_secs = microtime(true) - $start;
        //should take ~0.04 seconds long
        echo "    ".$i." points reprojected in ".round($time_elapsed_secs, 4)." seconds\n";
        $this->layer->close();
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($reprojector, $this->map, $this->layer, $this->sourceProjection, $this->outputProjection, $point);
    }    
    
}

?>
