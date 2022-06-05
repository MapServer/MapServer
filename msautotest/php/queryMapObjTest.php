<?php

class queryMapObjTest extends \PHPUnit\Framework\TestCase
{
    protected $queryMap;
    protected $map_file = 'maps/helloworld-gif.map';
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj($this->map_file);
        $this->queryMap = $this->map->querymap;
    }

    public function test__getsetStatus()
    {
        $this->assertEquals(5, $this->queryMap->status = 5);
    }

}

?>
