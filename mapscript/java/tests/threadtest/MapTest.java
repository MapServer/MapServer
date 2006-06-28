// $Id$
//
// See README_THREADTEST.TXT for usage details.
//

public class MapTest {
    public static void main(String args[]) {
        String      mapfile = null;
        Integer     threads = null;
        Integer     iterations = null;

        for( int i = 0; i < args.length; i++ ) {
            if( "-t".equals(args[i]) ) {
                i++;
                threads = new Integer(args[i]);
                continue;
            }
            if( "-i".equals(args[i]) ) {
                i++;
                iterations = new Integer(args[i]);
                continue;
            }

            mapfile = args[i];
        }

        Thread[]    tpool = new Thread[threads.intValue()];
        for( int i = 0; i < tpool.length; i++ ) {
            tpool[i] = new MapThread(mapfile, iterations.intValue(), i);
        }
        for( int i = 0; i < tpool.length; i++ ) {
            tpool[i].start();
        }
    }
}
