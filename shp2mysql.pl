#!/usr/bin/perl

# Copyright (c) 2003, Attila Csipa, Manufaktura Internet Inzenjering doo


$|=1;

use DBI;
use Geo::Shapelib qw/:all/;
#use Shape qw/:all/; # use this if you have an older shapelib version installed and the above line fails (also change line 29)

$dbname = "changeme";
$host = "localhost";
$dbuser = "root";
$dbpass = "";

$mac = 0;	# this is not a mac
$ENDIAN = 1;    # 0 = big endian - mac, 1 = little endian - intel

# --- do not edit below this line

die "Configure database parameters on the beginning of this script file\n" if $dbname eq "changeme";
$dbh = DBI->connect("DBI:mysql:$dbname:$host:3306",$dbuser,$dbpass) || die $DBI::errstr ;
$dbh->{RaiseError} = 1;

die "Usage: shp2mysql shpname\n\nwhere shpname is the name (with path) to the base name of the shape file (do not include file name extensions !)\n" unless $ARGV[0];
die "Shapefile $ARGV[0] not found !" unless -f "$ARGV[0].shp";
$shape = new Geo::Shapelib $ARGV[0];
#$shape = new Shape $ARGV[0];

$WKB_POINT = 1;
$WKB_LINESTRING = 2;
$WKB_POLYGON = 3;
$WKB_MULTIPOINT = 4;
$WKB_MULTILINESTRING = 5;
$WKB_MULTIPOLYGON = 6;
$WKB_GEOMETRYCOLLECTION = 7;

sub query
{
    my $sth = $dbh->prepare(shift) or die "DBI error $!";
    $sth->execute or die "DBI error $!";
    return $sth;
}

sub rect
{
    my ($x0, $y0, $x1, $y1) = @_;
    my $minx = $x0 <  $x1 ? $x0 : $x1;
    my $maxx = $x0 >= $x1 ? $x0 : $x1;
    my $miny = $y0 <  $y1 ? $y0 : $y1;
    my $maxy = $y0 >= $y1 ? $y0 : $y1;
    return ($minx, $miny, $maxx, $maxy);

}


$dbh->do("CREATE TABLE IF NOT EXISTS GEOMETRY_COLUMNS (
      F_TABLE_CATALOG varchar(100) NOT NULL default '',
      F_TABLE_SCHEMA varchar(100) NOT NULL default '',
      F_TABLE_NAME varchar(100) NOT NULL default '',
      F_GEOMETRY_COLUMN varchar(100) NOT NULL default '',
      G_TABLE_CATALOG varchar(100) NOT NULL default '',
      G_TABLE_SCHEMA varchar(100) NOT NULL default '',
      G_TABLE_NAME varchar(100) NOT NULL default '',
      STORAGE_TYPE int(11) NOT NULL default '0',
      GEOMETRY_TYPE int(11) NOT NULL default '0',
      COORD_DIMENSION int(11) NOT NULL default '0',
      MAX_PPR int(11) NOT NULL default '0',
      SRID int(11) NOT NULL default '0',
      PRIMARY KEY  (F_TABLE_CATALOG,F_TABLE_SCHEMA,F_TABLE_NAME,F_GEOMETRY_COLUMN)
    ) TYPE=MyISAM;");

$dbh->do("CREATE TABLE IF NOT EXISTS SPATIAL_REF_SYS (
      SRID int(11) NOT NULL default '0',
      AUTH_NAME varchar(255) default NULL,
      AUTH_SRID int(11) default NULL,
      SRTEXT varchar(255) default NULL,
      PRIMARY KEY  (SRID)
    ) TYPE=MyISAM;");

%sqlcreate = ( point => "  id int(11) NOT NULL auto_increment,
  layer int(11) NOT NULL default '0',
  x1 double default NULL,
  y1 double default NULL,
  x2 double default NULL,
  y2 double default NULL,
  vertices int(11) NOT NULL default '1',
  GID int(11) NOT NULL,
  PRIMARY KEY  (id),
  FOREIGN KEY GID(GID, x1, y1, x2, y2) REFERENCES geometry_point_num (GID),
  KEY layer (layer),
  KEY turbo (x2,y2,x1,y1)",

                arc => "  id int(11) NOT NULL auto_increment,
  layer int(11) NOT NULL default '0',
  x1 double default NULL,
  y1 double default NULL,
  x2 double default NULL,
  y2 double default NULL,
  vertices int(11) NOT NULL default '1',
  GID int(11) NOT NULL,
  PRIMARY KEY  (id),
  FOREIGN KEY GID(GID, x1, y1, x2, y2) REFERENCES geometry_arc_num (GID),
  KEY layer (layer),
  KEY turbo (x2,y2,x1,y1)",

                polygon => "  id int(11) NOT NULL auto_increment,
  layer int(11) NOT NULL default '0',
  x1 double default NULL,
  y1 double default NULL,
  x2 double default NULL,
  y2 double default NULL,
  vertices int(11) NOT NULL default '1',
  GID int(11) NOT NULL,
  PRIMARY KEY  (id),
  FOREIGN KEY GID(GID, x1, y1, x2, y2) REFERENCES geometry_polygon_num (GID),
  KEY layer (layer),
  KEY turbo (x2,y2,x1,y1)",

                annotation => "  id int(11) NOT NULL auto_increment,
  layer int(11) NOT NULL default '0',
  x1 double default NULL,
  y1 double default NULL,
  x2 double default NULL,
  y2 double default NULL,
  vertices int(11) NOT NULL default '1',
  angle double default '0.0',
  size int not null default '10',
  txt VARCHAR(255) NOT NULL default '',
  GID int(11) NOT NULL,
  PRIMARY KEY  (id),
  FOREIGN KEY GID(GID, x1, y1, x2, y2) REFERENCES geometry_annotation_num (GID),
  KEY layer (layer),
  KEY turbo (x2,y2,x1,y1)",

                num => "  GID INTEGER NOT NULL,
  ESEQ INTEGER NOT NULL,
  ETYPE INTEGER NOT NULL,
  SEQ INTEGER NOT NULL,
  X1 double default NULL,
  Y1 double default NULL,
  X2 double default NULL,
  Y2 double default NULL,
  CONSTRAINT GID_PK PRIMARY KEY (GID, ESEQ, SEQ),
  KEY x (X1,Y1)",
                bin => "  GID INTEGER NOT NULL,
  XMIN double default NULL,
  YMIN double default NULL,
  XMAX double default NULL,
  YMAX double default NULL,
  WKB_GEOMETRY BLOB,
  KEY (GID),
  KEY x (XMIN,YMIN,XMAX,YMAX)"
);

$sqlcreate{'3'} = $sqlcreate{'arc'};
$sqlcreate{'line'} = $sqlcreate{'arc'};

sub mklayer
{
    my ($sh, $type, $add) = @_;
	  $sh =~ tr/-/_/;
#    print "$sh/$type ($sqlcreate{$type})\n";
        $dbh->do("drop table if exists $sh");
        $dbh->do("drop table if exists $sh");
        $dbh->do("drop table if exists ${sh}_num");
        $dbh->do("drop table if exists ${sh}_bin");
        $dbh->do("create table if not exists $sh ($sqlcreate{$type} $add)");
        $dbh->do("create table if not exists ${sh}_num ($sqlcreate{num})");
        $dbh->do("create table if not exists ${sh}_bin ($sqlcreate{bin})");
        $dbh->do("delete from GEOMETRY_COLUMNS where F_TABLE_NAME = ".$dbh->quote($sh));
        $dbh->do("insert into GEOMETRY_COLUMNS 
        (F_TABLE_CATALOG,F_TABLE_SCHEMA,F_TABLE_NAME,F_GEOMETRY_COLUMN,G_TABLE_CATALOG,G_TABLE_SCHEMA,G_TABLE_NAME,STORAGE_TYPE,GEOMETRY_TYPE,COORD_DIMENSION,MAX_PPR,SRID)
        values ('$dbname', '', ".$dbh->quote($sh).", 'GID', '$dbname', '', ".$dbh->quote("${sh}_bin").", 1, '$gtype{$type}', 2, 2, 0)");
}

sub wkb
{
    $int = $mac ? "N" : "V";
    my ($poly, $end, $wkbt)= (shift, shift, shift);
    my $wkb = pack("C$int$int", $end, $WKB_GEOMETRYCOLLECTION, 1);
    $wkb .= pack("C$int", $end, $wkbt);
    my $nr = shift;
    if ($poly){
        $wkb .= pack("$int", $nr);
    }
    while ($nr -- > 0){
        my $pts = shift;
        if ($wkbt != $WKB_POINT){
            $wkb .= pack("$int", $pts);
        }
        $double = "";;
        for (my $i=0; $i<$pts; $i++){
            $double = pack("dd", shift, shift);
            if ($mac){
                @o = unpack("CCCCCCCCCCCCCCCC", $double);
                $double = pack("CCCCCCCCCCCCCCCC", $o[7],$o[6],$o[5],$o[4],$o[3],$o[2],$o[1],$o[0],$o[15],$o[14],$o[13],$o[12],$o[11],$o[10],$o[9],$o[8]); 
            }
            $wkb .= $double;
        }
    }
    return $wkb; 
}

@tmp = split(/\//, $ARGV[0]);
$sh = pop @tmp;

undef $fields;
undef @field;
undef $appendvars;
undef $vars;
#$shape->{FieldTypes} = ['Integer'];
for (@{$shape->{FieldNames}}){
	$fields .= ", f_$_ VARCHAR(255) NOT NULL DEFAULT ''";
	push @field, "f_$_";
	$appendvars .= ", f_$_ ";
	$vars++;
}
	
if ($shape->{Shapetype} eq POINT){
	mklayer($sh , 'point', $fields);
} elsif ($shape->{Shapetype} eq LINE or $shape->{Shapetype} eq 3){
	mklayer($sh , 'line', $fields);
} elsif ($shape->{Shapetype} eq ARC){
        mklayer($sh , 'arc', $fields);
} elsif ($shape->{Shapetype} eq POLYGON){
	mklayer($sh , 'polygon', $fields);
}
	
			       
for (0..@{$shape->{Shapes}}-1){
	undef $appenddata;
	for ($v = 0; $v < $vars; $v++){
		$appenddata .= ", ".$dbh->quote(${$shape->{ShapeRecords}}[$_][$v]);
#print $dbh->quote(${$shape->{ShapeRecords}}[$_][$v]);
	}
	$vrt=-1;
	while (${$shape->{Shapes}}[$_]{Vertices}[++$vrt][0] ne undef){
         $gid++;
         $feat++;
	 undef %object;
	 if ($shape->{Shapetype} eq LINE or $shape->{Shapetype} eq 3){
		$object{x0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt-1][0];
		$object{y0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt-1][1];
		$object{x1} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][0];
		$object{y1} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][1];
		next unless $object{x0};
# line
	    print "-";
            $dbh->do("insert into ${sh}_num (GID, ESEQ, SEQ, ETYPE, X1, Y1, X2, Y2) values ('$gid', 1, 1, 2, '$object{x0}','$object{y0}','$object{x1}','$object{y1}')");
            $string =  wkb(0, $ENDIAN, $WKB_LINESTRING, 1, 2,$object{x0}, $object{y0}, $object{x1}, $object{y1});
            my $sth = $dbh->prepare("insert into ${sh}_bin (GID, XMIN, YMIN, XMAX, YMAX, WKB_GEOMETRY) values ('$gid', '$object{x0}','$object{y0}','$object{x1}','$object{y1}', ? )");
             $sth->execute($string);
            $sth->finish;
            ($minx, $miny, $maxx, $maxy) = rect($object{x0}, $object{y0}, $object{x1}, $object{y1});
            $dbh->do("insert into $sh (id, layer, vertices, GID, x1, y1, x2, y2 $appendvars) values ('$feat', '$layer{$sh}', 1, '$gid', '$minx', '$miny', '$maxx', '$maxy' $appenddata)") if $vrt eq 0;

	} elsif ($shape->{Shapetype} eq ARC){
                $object{x0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt-1][0];
                $object{y0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt-1][1];
                $object{x1} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][0];
                $object{y1} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][1];
                next unless $object{x0};
# line
            print "-";
            $dbh->do("insert into ${sh}_num (GID, ESEQ, SEQ, ETYPE, X1, Y1, X2, Y2) values ('$gid', 1, 1, 2, '$object{x0}','$object{y0}','$object{x1}','$object{y1}')");
            $string =  wkb(0, $ENDIAN, $WKB_LINESTRING, 1, 2,$object{x0}, $object{y0}, $object{x1}, $object{y1});
            my $sth = $dbh->prepare("insert into ${sh}_bin (GID, XMIN, YMIN, XMAX, YMAX, WKB_GEOMETRY) values ('$gid', '$object{x0}','$object{y0}','$object{x1}','$object{y1}', ? )");
             $sth->execute($string);
            $sth->finish;
            ($minx, $miny, $maxx, $maxy) = rect($object{x0}, $object{y0}, $object{x1}, $object{y1});
            $dbh->do("insert into $sh (id, layer, vertices, GID, x1, y1, x2, y2 $appendvars) values ('$feat', '$layer{$sh}', 1, '$gid', '$minx', '$miny', '$maxx', '$maxy' $appenddata)") if $vrt eq 0;


	 } elsif ($shape->{Shapetype} eq POINT){
		$object{x0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][0];
		$object{y0} = ${$shape->{Shapes}}[$_]{Vertices}[$vrt][1];
# point

	    print ".";
            $dbh->do("insert into ${sh}_num (GID, ESEQ, SEQ, ETYPE, X1, Y1, X2, Y2) values ('$gid', 1, 1, 1, '$object{x0}','$object{y0}','0','0')");
            my $sth = $dbh->prepare("insert into ${sh}_bin (GID, XMIN, YMIN, XMAX, YMAX, WKB_GEOMETRY) values ('$gid', '$object{x0}','$object{y0}','$object{x0}','$object{y0}', ?)");
            $sth->execute(wkb(0, $ENDIAN, $WKB_POINT, 1, 1,$object{x0}, $object{y0}));
            $sth->finish;
            $dbh->do("insert into $sh (id, layer, vertices, GID, x1, y1, x2, y2 $appendvars) values ('$feat', '$layer{$sh}', 1, '$gid', '$object{x0}', '$object{y0}', '$object{x0}', '$object{y0}' $appenddata)");
	    
	 } elsif ($shape->{Shapetype} eq POLYGON){
# poly

	    print "O";
		@vertex = @{${$shape->{Shapes}}[$_]{Vertices}};
	    
            ($minx, $miny, $maxx, $maxy) = rect($vertex[0][0], $vertex[0][1], $vertex[0][0], $vertex[0][1]);
            $eseq=1;
            push @vertices, $vertex[0][0], $vertex[0][1];
            for ($v = 1; $v<=$#vertex; $v++) {
                $dbh->do("insert into ${sh}_num (GID, ESEQ, SEQ, ETYPE, X1, Y1, X2, Y2) values ('$gid', 1, $v, 3, '$vertex[$v -1][0]','$vertex[$v -1][1]', '$vertex[$v][0]','$vertex[$v][1]')");
                push @vertices, $vertex[$v][0], $vertex[$v][1];
                $maxx = $vertex[$v][0] if ($vertex[$v][0] > $maxx);
                $minx = $vertex[$v][0] if ($vertex[$v][0] < $minx);
                $maxy = $vertex[$v][1] if ($vertex[$v][1] > $maxy);
                $miny = $vertex[$v][1] if ($vertex[$v][1] < $miny);
#                $dbh->do("insert into vertex (shape,x,y,z) values ('$shape_id','$vertex[$v][0]','$vertex[$v][1]','0')");
            }
            my $sth = $dbh->prepare("insert into ${sh}_bin (GID, XMIN, YMIN, XMAX, YMAX, WKB_GEOMETRY) values ('$gid', '$minx','$miny','$maxx','$maxy', ?)");
            $sth->execute(wkb(1, $ENDIAN, $WKB_POLYGON, 1, $#vertex+1, @vertices));
            $sth->finish;
            undef @vertices;
            $dbh->do("insert into $sh (id, layer, vertices, GID, x1, y1, x2, y2 $appendvars) values ('$feat', '$layer{$sh}', '$#vertex', '$gid', '$minx', '$miny', '$maxx', '$maxy' $appenddata)") if $vrt eq 0;
	    $vrt = $#vertex;
	 }

	}
}
print "\n";
