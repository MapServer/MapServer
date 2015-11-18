import edu.umn.gis.mapscript.mapObj;
import edu.umn.gis.mapscript.OWSRequest;
import edu.umn.gis.mapscript.mapscript;
import java.io.*;

class WxSTest_thread extends Thread {

    public String	mapName;
    public byte[]       resultBytes;

    public void run() {
	mapObj map = new mapObj(mapName);

        map.setMetaData( "ows_onlineresource", "http://dummy.org/" );

        OWSRequest req = new OWSRequest();

        req.setParameter( "SERVICE", "WMS" );
        req.setParameter( "VERSION", "1.1.0" );
        req.setParameter( "REQUEST", "GetCapabilities" );

        mapscript.msIO_installStdoutToBuffer();

        int owsResult = map.OWSDispatch( req );

        if( owsResult != 0 )
            System.out.println( "OWSDispatch Result (expect 0): " + owsResult );

//        System.out.println( "Document:" );
//        System.out.println( mapscript.msIO_getStdoutBufferString() );

        resultBytes = mapscript.msIO_getStdoutBufferBytes();
        mapscript.msIO_resetHandlers();
    }
}

public class WxSTest {
    public static void main(String[] args)  {
        try {
            WxSTest_thread tt[] = new WxSTest_thread[100];
            int i;
            int expectedLength=0, success = 0, failure=0;

            for( i = 0; i < tt.length; i++ )
            {
                tt[i] = new WxSTest_thread();
                tt[i].mapName = args[0];
            }
            
            for( i = 0; i < tt.length; i++ )
                tt[i].start();

            for( i = 0; i < tt.length; i++ )
            {
                tt[i].join();
                if( i == 0 )
                {
                    expectedLength = tt[i].resultBytes.length;
                    System.out.println( "["+i+"] Document Length: " + expectedLength + ", expecting somewhere around 10000 or more." );
                }
                else if( expectedLength != tt[i].resultBytes.length )
                {
                    System.out.println( "["+i+"] Document Length:" + tt[i].resultBytes.length + " Expected:" + expectedLength );
                    failure++;
                }
                else
                    success++;
		
		// dump test results to fs for post-mortem inspection
		FileOutputStream fos = new FileOutputStream("/tmp/wxs_test_"+i);
		fos.write(tt[i].resultBytes);
		fos.close();
            }

            System.out.println( "Successes: " + success );
            System.out.println( "Failures: " + failure );

        } catch( Exception e ) {
            e.printStackTrace();
        }
    }
}

