<?php

class owsRequestTest extends \PHPUnit\Framework\TestCase
{
    protected $owsRequest;

    public function setUp(): void
    {
        $this->owsRequest = new OWSRequest();
    }

    public function test__setContentType()
    {
        # exception not thrown with PHPNG
        #$this->owsRequest->contenttype = 'TheType';
    }

    public function test__getContentType()
    {
        $this->assertEquals('', $this->owsRequest->contenttype);
    }

    public function test__setPostRequest()
    {
        # exception not thrown with PHPNG
        #$this->owsRequest->postrequest = 'PostRequest';
    }

    public function test__getPostRequest()
    {
        $this->assertEquals('', $this->owsRequest->postrequest);
    }

    public function test__setHttpCookieData()
    {
        # exception not thrown with PHPNG
        #$this->owsRequest->httpcookiedata = 'httpcookiedata';
    }

    public function test__getHttpCookieData()
    {
        $this->assertEquals('', $this->owsRequest->httpcookiedata);
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($owsRequest, $this->owsRequest, $this->owsRequest->postrequest, $this->owsRequest->httpcookiedata);
    }
    
}
?>
