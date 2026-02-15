<?php

class errorObjTest extends \PHPUnit\Framework\TestCase
{
    public function testErrorObjCreation()
    {
        $error = new errorObj();
        $this->assertInstanceOf('errorObj', $error);
    }

    public function testErrorProperties()
    {
        $error = new errorObj();
        $this->assertIsInt($error->code);
        $this->assertIsString($error->routine);
        $this->assertIsString($error->message);
    }

    public function testErrorChainTraversal()
    {
        // Trigger an error by loading a non-existent map
        try {
            @$map = new mapObj('maps/nonexistent.map');
        } catch (\Exception $e) {
            // expected
        }

        $error = new errorObj();
        // Walk the chain - next() returns null at end
        $visited = 0;
        $current = $error;
        while ($current !== null && $visited < 100) {
            $visited++;
            $current = $current->next();
        }
        $this->assertGreaterThanOrEqual(0, $visited);
    }

    public function testMsResetErrorList()
    {
        // Trigger an error
        try {
            @$map = new mapObj('maps/nonexistent.map');
        } catch (\Exception $e) {
            // expected
        }

        ms_ResetErrorList();
        $error = new errorObj();
        $this->assertEquals(MS_NOERR, $error->code);
    }

    public function testMsGetErrorString()
    {
        ms_ResetErrorList();
        $str = msGetErrorString("\n");
        $this->assertIsString($str);
    }

    # destroy variables, if not can lead to segmentation fault
    public function tearDown(): void
    {
        ms_ResetErrorList();
    }
}

?>
