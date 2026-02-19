<?php

class labelCacheObjTest extends \PHPUnit\Framework\TestCase
{
    public function testLabelCacheInstance()
    {
        $map_file = 'maps/labels-leader.map';
        $map = new mapObj($map_file);
        $map->draw(); // Populates label cache

        $labelCache = $map->labelcache;
        $this->assertInstanceOf('labelCacheObj', $labelCache);
    }
}
