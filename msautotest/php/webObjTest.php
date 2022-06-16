<?php

class webObjTest extends \PHPUnit\Framework\TestCase
{
    protected $web;

    public function setUp(): void
    {
        $map = new mapObj('maps/helloworld-gif.map');
        $this->web = $map->web;
    }

    public function test__getValidation()
    {
        $this->assertInstanceOf('hashTableObj', $this->web->validation);
    }

    public function test__setValidation()
    {
        # exception not thrown with PHPNG
        #$this->web->validation = 'this is an object, I swear';
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($web, $map, $this->web, $this->web->validation);
    }    
    
}

?>
