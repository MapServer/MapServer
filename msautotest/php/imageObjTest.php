<?php

class imageObjTest extends \PHPUnit\Framework\TestCase
{
    protected $map;

    public function setUp(): void
    {
        $this->map = new mapObj('maps/ogr_query.map');
    }

    public function testDraw()
    {
        $image = $this->map->draw();
        $this->assertInstanceOf('imageObj', $image);
    }

    public function testSave()
    {
        $image = $this->map->draw();
        $image->save('./result/image_save_test.png');
        $this->assertFileExists('./result/image_save_test.png');
    }

    public function testGetBytes()
    {
        $image = $this->map->draw();
        $bytes = $image->getBytes();
        $this->assertNotEmpty($bytes);
        $this->assertGreaterThan(0, strlen($bytes));
    }

    public function testGetSize()
    {
        $image = $this->map->draw();
        $size = $image->getSize();
        $this->assertGreaterThan(0, $size);
    }

    public function testPasteImage()
    {
        $image1 = $this->map->draw();
        $image2 = $this->map->draw();
        $this->assertEquals(MS_SUCCESS, $image1->pasteImage($image2, 0.5));
    }

    public function testImageProperties()
    {
        $image = $this->map->draw();
        $this->assertGreaterThan(0, $image->width);
        $this->assertGreaterThan(0, $image->height);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        unset($this->map);
    }
}

?>
