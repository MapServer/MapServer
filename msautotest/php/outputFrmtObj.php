<?php

class outputFormatObjTest extends PHPUnit_Framework_TestCase
{
    protected $outputFrmt;

    public function setUp()
    {
        $this->outputFrmt = new outputFormatObj('AGG/PNG', 'theName');
    }

    /**
     * @expectedException           MapScriptException
     * @exceptedExceptionMessage    Property 'bands' is read-only
     */
    public function test__setBands()
    {
        $this->outputFrmt->bands = 5;
    }

    public function test__getBands()
    {
        $this->assertEquals(1, $this->outputFrmt->bands);
    }

    /**
     * @expectedException           MapScriptException
     * @exceptedExceptionMessage    Property 'numformatoptions' is read-only
     */
    public function test__setNumFormatOptions()
    {
        $this->outputFrmt->numformatoptions = 5;
    }

    public function test__getNumFormatOptions()
    {
        $this->assertEquals(0, $this->outputFrmt->numformatoptions);
        $this->outputFrmt->setOption('OUTPUT_TYPE', 'RASTER');
        $this->outputFrmt->setOption('QUANTIZE_FORCE', 'on');
        $this->assertEquals(2, $this->outputFrmt->numformatoptions);
    }

    public function testgetOptionByIndex()
    {
        $this->outputFrmt->setOption('OUTPUT_TYPE', 'RASTER');
        $this->assertEquals('OUTPUT_TYPE=RASTER', $this->outputFrmt->getOptionByIndex(0));
    }
    
    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void {
        unset($outputFrmt, $this->outputFrmt, $this->outputFrmt->bands, $this->outputFrmt->numformatoptions, $this->outputFrmt->setOption);
    }    
    
}

?>
