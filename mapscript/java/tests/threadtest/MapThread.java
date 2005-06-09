// $Id$
//
// See README_THREADTEST.TXT for usage details.
//


import edu.umn.gis.mapscript.mapObj;

public class MapThread extends Thread {
    MapThread(String mapfile, int iterations) {
        this.mapfile = mapfile;
        this.iterations = iterations;
    }

    public void run() {
        mapObj  map = new mapObj(mapfile);

        for( int i = 0; i < iterations; i++ ) {
            map.draw();
            // Add additional test code here
        }
    }

    String      mapfile;
    int         iterations;
}
