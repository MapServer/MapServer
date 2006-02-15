import edu.umn.gis.mapscript.*;

/**
 * <p>Title: Mapscript conn pool</p>
 * <p>Description: n.a. </p>
 * @author Umberto Nicoletti (umberto.nicoletti@gmail.com)
 * @version 1.0
 */

public class ConnPool {

  public static void main(String[] args) {
    // at least it shouldn't crash
    System.out.println("Running mapscript.msConnPoolCloseUnreferenced");
    mapscript.msConnPoolCloseUnreferenced();
    System.out.println("Finished.");
  }

}
