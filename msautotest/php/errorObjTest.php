<?php

class errorObjTest extends \PHPUnit\Framework\TestCase
{
    protected $error;

    public function setUp(): void
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
