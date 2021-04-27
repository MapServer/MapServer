<?php

class ProjectionObjTest extends \PHPUnit\Framework\TestCase
{
    protected $projection;

    public function setUp(): void
    {
        $this->projection = new projectionObj("proj=latlong");
    }

    public function testsetWKTProjection()
    {
        $this->assertEquals(0, $this->projection->setWKTProjection( 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]'));

        $this->assertEquals(1, $this->projection->setWKTProjection('WrongStringisWrong'));
    }

    public function testClone()
    {
        $this->assertInstanceOf('projectionObj', $newProj = clone $this->projection);
    }
}

?>
