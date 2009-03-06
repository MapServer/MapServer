-----------------------------------
June, 28th 2006
Umberto Nicoletti (umberto.nicoletti@gmail.com)

I have added more functionality to the thread test by integrating it
in the Makefile so that it can be ran more easily and by adding a
query by attribute and a buffer functions. The latter requires GEOS
support in mapserver.

It is best to run the tests with MALLOC_CHECK_ variable set to 2 so
that memory errors are detected and reported (the jvm will crash
on the first error so it should be quite noticeable ;-) ).

-------- Original Message --------
Subject: [UMN_MAPSERVER-DEV] Multithreaded Mapscript
Date: Mon, 6 Jun 2005 19:22:41 -0700
From: Jerry Pisk <jerry.pisk@GMAIL.COM>
Reply-To: Jerry Pisk <jerry.pisk@GMAIL.COM>
To: MAPSERVER-DEV@LISTS.UMN.EDU

Hi everybody,

as I mentioned in my previous e-mail I was able to get mapserver to
work in a multithreaded program. But the code I covered was just a
fraction of mapserver's code, I only tested mapfile load and map
drawing from a PostGIS backend. Since I do not have the resources to
do extensive testing I decided to share what I've done with the
community. I wrote a simple Java program that creates a number of
threads and executes map operations on them. It uses Java mapscript to
call mapserver. The following assumes a JDK (I used 1.5 but older
versions should work as well) is installed and JAVA_HOME points to it.

There are two source files:
================  MapTest.java  ================

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
            tpool[i] = new MapThread(mapfile, iterations.intValue());
        }
        for( int i = 0; i < tpool.length; i++ ) {
            tpool[i].start();
        }
    }
}
================================================

================  MapThread.java  ================

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
================================================

To use these simply compile mapserver and mapscript (for java you may
need to run 'make interface' in mapscript/java). Save the two Java
files into a directory of your choice. Compile them (point to your
mapscript.jar, in mapscript/java/):

$JAVA_HOME/bin/javac -cp ./mapscript.jar *.java

and execute:

$JAVA_HOME/bin/java -Djava.library.path=./ -cp ./:./mapscript.jar
MapTest -t <t> -i <i> <map file>

where <t> is the number of threads to create, <i> is the number of
iterations to execute on each thread and <map file> is a full path to
your map file. Again, make sure you point to your mapscript.jar and
point to where libmapscript.so is in that java.library.path definition
(I copied those two files into the same directory as the Java code but
your setup may differ).

There were occasional libgd related errors (Unable to initialize
image) but those simply resulted in an exception being thrown, which
is in my humble opinion acceptable, as long as the code does not
crash.

The code does no error checking, feel free to add it but this is
really meant to be a developer's troubleshooting tool, not an end user
utility. Also if a map drawing fails that thread will simply exit
without finishing its number of iterations.

The point of all this is to help make mapserver thread safe. The code
above is meant as a simple starting point that should help mapserver
developers test the code in a multithreaded application. Any comments
and thoughts or even module tests (I do not have the resources to test
a lot of mapserver's code, especially all the various data back ends)
will be appreciated.

Jerry Pisk


----------
$Id$
