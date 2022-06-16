<?php

class hashtableObjTest extends \PHPUnit\Framework\TestCase
{
    protected $hash;
    protected $map;

    public function setUp(): void
    {
        $map_file = 'maps/helloworld-gif.map';
        $this->map = new mapObj($map_file);
        $this->hash = $this->map->web->metadata;
    }

    public function test__setNumItems()
    {   # exception not thrown with PHPNG     
        #$this->hash->numitems = 5;
    }

    public function test__getNumItems()
    {
        $this->assertEquals(1, $this->hash->numitems);
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($hash, $map_file, $map, $this->hash);
    }
    
}

?>
