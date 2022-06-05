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

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'validation' is an object
     */
    public function test__setValidation()
    {
        $this->web->validation = 'this is an object, I swear';
    }
}

?>
