/*
 Created by unicoletti@prometeo.it to test backwards-compatible
 pointObj constructor. See bug #1106.

 2/1/2005: Created
*/

import edu.umn.gis.mapscript.*;

public class MakePoint {
	public static void main(String[] args) {
	pointObj result = new pointObj(0,0,0);
	System.out.println("Point made!");
	}
}
