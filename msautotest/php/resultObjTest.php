<?php

class resultObjTest extends \PHPUnit\Framework\TestCase
{
    protected $map;
    protected $layer;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
        $this->layer = $this->map->getLayer(0);
        $this->layer->status = MS_ON;
        $this->map->queryByRect($this->map->extent);
    }

    public function testResultCache()
    {
        $resultCache = $this->layer->getResults();
        $this->assertInstanceOf('resultCacheObj', $resultCache);
        $this->assertGreaterThan(0, $resultCache->numresults);

        $result = $resultCache->getResult(0);
        $this->assertInstanceOf('resultObj', $result);
    }

    public function testResultObj()
    {
        $result = $this->layer->getResult(0);
        $this->assertInstanceOf('resultObj', $result);
        $this->assertGreaterThanOrEqual(0, $result->shapeindex);
        $this->assertGreaterThanOrEqual(0, $result->tileindex);
        $this->assertGreaterThanOrEqual(0, $result->resultindex);
    }

    public function tearDown(): void
    {
        unset($this->layer);
        unset($this->map);
    }
}
