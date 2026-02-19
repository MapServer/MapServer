<?php

class dbfInfoTest extends \PHPUnit\Framework\TestCase
{
    protected $shp_file = '../../tests/line.shp';
    protected $shp;
    protected $dbf;

    public function setUp(): void
    {
        $this->shp = new shapefileObj($this->shp_file);
        $this->dbf = $this->shp->getDBF();
    }

    public function testGetFieldName()
    {
        $this->assertEquals('FID', $this->dbf->getFieldName(0));
    }

    public function testGetFieldWidth()
    {
        $this->assertGreaterThan(0, $this->dbf->getFieldWidth(0));
    }

    public function testGetFieldDecimals()
    {
        $this->assertEquals(0, $this->dbf->getFieldDecimals(0));
    }

    public function testGetFieldType()
    {
        // FTInteger is 0, FTDouble is 1, FTString is 2
        $this->assertEquals(1, $this->dbf->getFieldType(0));
    }

    public function tearDown(): void
    {
        unset($this->dbf);
        unset($this->shp);
    }
}
