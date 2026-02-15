<?php

class symbolSetObjTest extends \PHPUnit\Framework\TestCase
{
    protected $symbolset;
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/labels.map');
        $this->symbolset = $this->map->symbolset;
    }

    public function testGetSymbolByName()
    {
        $symbol = $this->symbolset->getSymbolByName('plant');
        $this->assertInstanceOf('symbolObj', $symbol);
    }

    public function testGetSymbol()
    {
        $symbol = $this->symbolset->getSymbol(1);
        $this->assertInstanceOf('symbolObj', $symbol);
    }

    public function testIndex()
    {
        $idx = $this->symbolset->index('plant');
        $this->assertGreaterThanOrEqual(0, $idx);
    }

    public function testAppendAndRemoveSymbol()
    {
        $symbol = new symbolObj('testSymbol');
        $idx = $this->symbolset->appendSymbol($symbol);
        $this->assertGreaterThan(0, $idx);

        $removed = $this->symbolset->removeSymbol($idx);
        $this->assertInstanceOf('symbolObj', $removed);
    }

    public function testNumsymbols()
    {
        $this->assertGreaterThan(0, $this->symbolset->numsymbols);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->symbolset, $this->map);
    }
}

?>
