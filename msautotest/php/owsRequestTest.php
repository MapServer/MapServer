<?php

class owsRequestTest extends \PHPUnit\Framework\TestCase
{
    protected $owsRequest;

    public function setUp(): void
    {
        $this->owsRequest = new OWSRequest();
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'contenttype' is read-only
     */
    public function test__setContentType()
    {
        $this->owsRequest->contenttype = 'TheType';
    }

    public function test__getContentType()
    {
        $this->assertEquals('', $this->owsRequest->contenttype);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'postrequest' is read-only
     */
    public function test__setPostRequest()
    {
        $this->owsRequest->postrequest = 'PostRequest';
    }

    public function test__getPostRequest()
    {
        $this->assertEquals('', $this->owsRequest->postrequest);
    }

    /**
     * @expectedException           MapScriptException
     * @expectedExceptionMessage    Property 'httpcookiedata' is read-only
     */
    public function test__setHttpCookieData()
    {
        $this->owsRequest->httpcookiedata = 'httpcookiedata';
    }

    public function test__getHttpCookieData()
    {
        $this->assertEquals('', $this->owsRequest->httpcookiedata);
    }
}
?>
