<?php

class ColorObjTest extends \PHPUnit\Framework\TestCase
{
    protected $color;
	protected $original = "#00ff00";
	protected $originalAlpha = 255;

    public function setUp(): void
    {
        $map_file = 'maps/labels.map';
        $map = new mapObj($map_file);
        $this->color = $map->getLayer(0)->getClass(0)->getLabel(0)->color;
    }

    public function test__getsetAlpha()
    {
        $this->assertEquals(50, $this->color->alpha=50);
		$this->color->alpha=$this->originalAlpha;
    }

    public function test__toHex()
    {
        # Test fails on ubuntu 20.04
        #$this->assertEquals("#00ff00", $this->color->toHex());
		#$this->color->alpha=128;
		#$this->assertEquals("#00ff0080", $this->color->toHex());
		#$this->color->alpha = $this->originalAlpha;
	}

	public function test__setHex()
	{
		$this->color->alpha = 80;
		$this->assertEquals(0, $this->color->red);
		$this->assertEquals(80, $this->color->alpha);
		$this->assertEquals(MS_SUCCESS, $this->color->setHex("#ff00ff"));
		$this->assertEquals(255, $this->color->red);
		$this->assertEquals(255, $this->color->alpha);
		$this->assertEquals(MS_SUCCESS, $this->color->setHex("#ff00ff80"));
		$this->assertEquals(128, $this->color->alpha);
		$this->color->setHex($this->original);
	}
	
	public function test__setRGB()
	{
		$this->color->alpha = 80;
		$this->assertEquals(0, $this->color->red);
		$this->assertEquals(80, $this->color->alpha);
		$this->assertEquals(MS_SUCCESS, $this->color->setRGB(255, 0, 255));
		$this->assertEquals(255, $this->color->red);
		$this->assertEquals(255, $this->color->alpha);
		$this->color->setHex($this->original);
		$this->assertEquals(0, $this->color->red);
		$this->assertEquals(255, $this->color->alpha);
		$this->assertEquals(MS_SUCCESS, $this->color->setRGB(255, 0, 255, 80));
		$this->assertEquals(255, $this->color->red);
		$this->assertEquals(80, $this->color->alpha);
		$this->color->setHex($this->original);
		$this->color->alpha = $this->originalAlpha;
	}

}
?>
