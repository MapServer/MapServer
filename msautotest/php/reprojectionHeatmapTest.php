<?php
/*
PURPOSE:     test reprojection performance through MapScript heatmap
SOURCE DATA: OSM places for Nova Scotia, 3843 points in EPSG:4326
DATA FORMAT: SpatiaLite database
*/
class reprojectionHeatmapTest extends \PHPUnit\Framework\TestCase
{
    protected $map;

    protected $x1_4326 = -66.374741;
    protected $y1_4326 = 43.400270;
    protected $x2_4326 = -59.697577;
    protected $y2_4326 = 50.267922;

    protected $x1_3857 = -7388802.37;
    protected $y1_3857 = 5373096.839;
    protected $x2_3857 = -6645503.873;
    protected $y2_3857 = 6492805.151;

    protected $resultImage4326 = './result/heatmap-4326.png';
    protected $expectedImage4326 = './expected/heatmap-4326.png';
    protected $resultImage3857 = './result/heatmap-3857.png';
    protected $expectedImage3857 = './expected/heatmap-3857.png';

    public function setUp(): void
    {
        $this->map = new mapObj('maps/reproj-heatmap.map');
        $this->map->getLayerByName('heatmap')->setProjection('init=epsg:4326');
        $this->map->getLayerByName('points')->setProjection('init=epsg:4326');
    }

    public function testHeatmapReprojectSpeed()
    {
        # Check results first
        $this->map->setProjection('init=epsg:4326');
        $this->map->setExtent($this->x1_4326, $this->y1_4326, $this->x2_4326, $this->y2_4326);
        $this->map->units = MS_DD;
        $image4326 = $this->map->draw();
        $image4326->save($this->resultImage4326);
        $this->assertFileEquals(
            $this->expectedImage4326,
            $this->resultImage4326,
            'Result setProjection EPSG:4326 map image is not same as Expected',
        );

        $this->map->setProjection('init=epsg:3857');
        $this->map->setExtent($this->x1_3857, $this->y1_3857, $this->x2_3857, $this->y2_3857);
        $this->map->units = MS_METERS;
        $image3857 = $this->map->draw();
        $image3857->save($this->resultImage3857);
        $this->assertFileEquals(
            $this->expectedImage3857,
            $this->resultImage3857,
            'Result setProjection EPSG:3857 map image is not same as Expected',
        );

        # Check and print performance
        $it = 20;
        $this->map->setProjection('init=epsg:4326');
        $this->map->setExtent($this->x1_4326, $this->y1_4326, $this->x2_4326, $this->y2_4326);
        $this->map->units = MS_DD;
        $start = microtime(true);
        for ($i = 0; $i < $it; $i++)
        {
            $this->map->draw();
        }
        $time_elapsed_secs_noreproj = microtime(true) - $start;

        $this->map->setProjection('init=epsg:3857');
        $this->map->setExtent($this->x1_3857, $this->y1_3857, $this->x2_3857, $this->y2_3857);
        $this->map->units = MS_METERS;
        $start = microtime(true);
        for ($i = 0; $i < $it; $i++)
        {
            $this->map->draw();
        }
        $time_elapsed_secs_reproj = microtime(true) - $start;

        $time_diff = $time_elapsed_secs_reproj - $time_elapsed_secs_noreproj;
        $time_diff = $time_diff / $it;

        # expected to be < 0.01s, similar to reprojectionObjTest's performance
        echo "Heatmap reprojection time is ~".round($time_diff, 4)." seconds\n";
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->map, $image4326, $image3857);
    }
}

?>
