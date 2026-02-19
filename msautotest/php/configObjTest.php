<?php

class configObjTest extends \PHPUnit\Framework\TestCase
{
    protected $config_file = 'maps/sample.conf';

    public function testConstructor()
    {
        $config = new configObj($this->config_file);
        $this->assertInstanceOf('configObj', $config);
    }

    public function testConstructorNull()
    {
        $config = new configObj();
        $this->assertInstanceOf('configObj', $config);
    }
}
