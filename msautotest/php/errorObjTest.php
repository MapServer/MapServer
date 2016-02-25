<?php

class ErrorObjTest extends PHPUnit_Framework_TestCase
{
    protected $error;

    public function setUp()
    {
        $this->error = ms_GetErrorObj();
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'isreported' is read-only
     */
    public function test__setIsReported()
    {
        $this->error->isreported = 5;
    }

    public function test__getIsReported()
    {
        $this->assertEquals(0, $this->error->isreported);
    }
}

?>
