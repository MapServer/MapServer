import edu.umn.gis.mapscript.mapObj;
import edu.umn.gis.mapscript.OWSRequest;
import edu.umn.gis.mapscript.mapscript;

public class WxSTest {
    public static void main(String[] args)  {
	  
	String filter="A Point";

	mapObj map = new mapObj(args[0]);

        map.setMetaData( "ows_onlineresource", "http://dummy.org/" );

        OWSRequest req = new OWSRequest();

        req.setParameter( "SERVICE", "WMS" );
        req.setParameter( "VERSION", "1.1.0" );
        req.setParameter( "REQUEST", "GetCapabilities" );

        mapscript.msIO_installStdoutToBuffer();

        int owsResult = map.OWSDispatch( req );

        System.out.println( "OWSDispatch Result (expect 0): " + owsResult );

//        System.out.println( "Document:" );
//        System.out.println( mapscript.msIO_getStdoutBufferString() );

        byte[] resultBytes = mapscript.msIO_getStdoutBufferBytes();
        
        System.out.println( "Document Length (expect 10603):" + resultBytes.length);
    }
}

