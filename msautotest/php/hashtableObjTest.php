<?php

class HashTableObjTest extends \PHPUnit\Framework\TestCase
{
    protected $hash;

    public function setUp(): void
    {
        $map_file = 'maps/helloworld-gif.map';
        $map = new mapObj($map_file);
        $this->hash = $map->web->metadata;
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'numitems' is read-only
     */
    public function test__setNumItems()
    {
        $this->hash->numitems = 5;
    }

    public function test__getNumItems()
    {
        $this->assertEquals(1, $this->hash->numitems);
    }
}

?>
