<?php

class msIOTest extends \PHPUnit\Framework\TestCase
{
    public function testInstallStdoutToBuffer()
    {
        msIO_installStdoutToBuffer();
        // If we get here without error, the function works
        msIO_resetHandlers();
        $this->assertTrue(true);
    }

    public function testGetStdoutBufferString()
    {
        msIO_installStdoutToBuffer();

        $map = new mapObj('maps/ows_wms.map');
        $request = new OWSRequest();
        $request->setParameter('SERVICE', 'WMS');
        $request->setParameter('VERSION', '1.1.1');
        $request->setParameter('REQUEST', 'GetCapabilities');
        @$map->OWSDispatch($request);

        $str = msIO_getStdoutBufferString();
        $this->assertIsString($str);
        $this->assertNotEmpty($str);

        msIO_resetHandlers();
    }

    public function testGetStdoutBufferBytes()
    {
        msIO_installStdoutToBuffer();

        $map = new mapObj('maps/ows_wms.map');
        $request = new OWSRequest();
        $request->setParameter('SERVICE', 'WMS');
        $request->setParameter('VERSION', '1.1.1');
        $request->setParameter('REQUEST', 'GetCapabilities');
        @$map->OWSDispatch($request);

        $bytes = msIO_getStdoutBufferBytes();
        $this->assertNotEmpty($bytes);

        msIO_resetHandlers();
    }

    public function testStripContentType()
    {
        msIO_installStdoutToBuffer();

        $map = new mapObj('maps/ows_wms.map');
        $request = new OWSRequest();
        $request->setParameter('SERVICE', 'WMS');
        $request->setParameter('VERSION', '1.1.1');
        $request->setParameter('REQUEST', 'GetCapabilities');
        @$map->OWSDispatch($request);

        $contentType = msIO_stripStdoutBufferContentType();
        // Content-type may or may not be present depending on config
        if ($contentType !== null) {
            $this->assertIsString($contentType);
        }

        msIO_resetHandlers();
    }

    public function testStripContentHeaders()
    {
        msIO_installStdoutToBuffer();

        $map = new mapObj('maps/ows_wms.map');
        $request = new OWSRequest();
        $request->setParameter('SERVICE', 'WMS');
        $request->setParameter('VERSION', '1.1.1');
        $request->setParameter('REQUEST', 'GetCapabilities');
        @$map->OWSDispatch($request);

        msIO_stripStdoutBufferContentHeaders();
        $str = msIO_getStdoutBufferString();
        $this->assertIsString($str);

        msIO_resetHandlers();
    }

    public function testResetHandlers()
    {
        msIO_installStdoutToBuffer();
        msIO_resetHandlers();
        // Should not crash or error
        $this->assertTrue(true);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        msIO_resetHandlers();
    }
}
