/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Keyword definitions for the mapfiles and symbolfiles.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/


#ifndef MAPFILE_H
#define MAPFILE_H

enum MS_LEXER_STATES {MS_TOKENIZE_DEFAULT=0, MS_TOKENIZE_FILE, MS_TOKENIZE_STRING, MS_TOKENIZE_EXPRESSION, MS_TOKENIZE_URL_VARIABLE, MS_TOKENIZE_URL_STRING, MS_TOKENIZE_VALUE, MS_TOKENIZE_NAME};
enum MS_TOKEN_SOURCES {MS_FILE_TOKENS=0, MS_STRING_TOKENS, MS_URL_TOKENS};

/*
** Keyword definitions for the mapfiles and symbolfiles (used by lexer)
*/
#define ANGLE 1000
#define ANTIALIAS 1001
#define BACKGROUNDCOLOR 1002
#define BUFFER 1003
#define CLASS 1004
#define CLASSITEM 1005
#define COLOR 1006
#define DATA 1007
#define DD 1008

#define EMPTY 1010
#define END 1011
#define ERROR 1012
#define EXPRESSION 1013
#define EXTENT 1014
#define FEATURE 1015
#define FILLED 1016
#define FOOTER 1017
#define FONT 1018
#define FONTSET 1019
#define FROM 1020
#define GROUP 1021
#define HEADER 1022
#define IMAGE 1023
#define IMAGECOLOR 1024
#define IMAGEPATH 1025
#define IMAGEURL 1026
#define INDEX 1027
#define INTERLACE 1028
#define INTERVALS 1029
#define JOIN 1030
#define KEYSIZE 1031
#define KEYSPACING 1032
#define LABEL 1033
#define LABELCACHE 1035
#define LABELITEM 1036
#define LABELMAXSCALE 1037
#define LABELMINSCALE 1038
#define LAYER 1040
#define LEGEND 1041

#define LOG 1043
#define MAP 1044


#define MAXFEATURES 1047
#define MAXSCALE 1048
#define MAXSIZE 1049
#define MAXTEMPLATE 1050
#define MINDISTANCE 1051
#define MINFEATURESIZE 1053
#define MINSCALE 1054
#define MINSIZE 1055
#define MINTEMPLATE 1056
#define NAME 1057
#define NFTEMPLATE 1058

#define OFFSET 1060
#define OFFSITE 1061
#define OUTLINECOLOR 1062
#define PARTIALS 1063
#define POINTS 1064
#define POSITION 1065
#define POSTLABELCACHE 1098
#define PROJECTION 1066
#define QUERY 1067
#define QUERYITEM 1068
#define QUERYMAP 1069
#define REFERENCE 1070
#define SCALE 1071
#define SCALEBAR 1072

#define SHADOWCOLOR 1074
#define SHADOWSIZE 1075
#define SHAPEPATH 1076
#define SIZE 1077
#define STATUS 1078

#define STYLE 1080
#define SYMBOL 1081
#define SYMBOLSCALE 1082
#define TABLE 1083
#define TEMPLATE 1084
#define TEXT 1085
#define TEXTITEM 1086

#define TILEINDEX 1088
#define TILEITEM 1089
#define TOLERANCE 1090
#define TO 1091
#define TRANSPARENT 1092
#define TRANSFORM 1093
#define TYPE 1094
#define UNITS 1095
#define WEB 1096
#define WRAP 1097

#define FORCE 1098
#define TOLERANCEUNITS 1099
#define CHARACTER 1100

#define CONNECTION 1101
#define CONNECTIONTYPE 1102

#define SYMBOLSET 1105

#define IMAGETYPE 1113
#define IMAGEQUALITY 1114

#define GAP 1115

#define FILTER 1116
#define FILTERITEM 1117

#define REQUIRES 1118
#define LABELREQUIRES 1119

#define METADATA 1120
#define LATLON 1121

#define RESOLUTION 1122

#define SIZEUNITS 1123

#define STYLEITEM 1124

#define TITLE 1126

#define LINECAP 1127
#define LINEJOIN 1128
#define LINEJOINMAXSIZE 1129

#define TRANSPARENCY 1130

#define MARKER 1131
#define MARKERSIZE 1132
#define MINBOXSIZE 1133
#define MAXBOXSIZE 1134

#define OUTPUTFORMAT 1135
#define MIMETYPE 1136
#define DRIVER 1137
#define IMAGEMODE 1138
#define FORMATOPTION 1139

#define GRATICULE 1140
#define GRID 1141

#define MAXARCS 1142
#define MINARCS 1143
#define MAXINTERVAL 1144
#define MININTERVAL 1145
#define MAXSUBDIVIDE 1146
#define MINSUBDIVIDE  1147
#define LABELFORMAT  1148

#define DATAPATTERN 1150
#define FILEPATTERN 1151
#define TEMPLATEPATTERN 1152

#define PROCESSING  1153

/* The DEBUG macro is also used to request debugging output.  Redefine
   for keyword purposes.  */

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 1154

#define EXTENSION 1155

#define KEYIMAGE 1156
#define QUERYFORMAT 1157

#define CONFIG 1158
#define BANDSITEM 1159

#define ENCODING 1162

#define WIDTH 1163
#define MINWIDTH 1164
#define MAXWIDTH 1165

#define OUTLINEWIDTH 1166

/* Color Range support (was Gradient Support)*/
#define COLORRANGE 1170
#define DATARANGE 1172
#define RANGEITEM 1173

/* WKT support (bug 1466) */
#define WKT 1180

#define LEGENDFORMAT 1190 /* bug 1518 */
#define BROWSEFORMAT 1191

#define RELATIVETO 1192

#define OPACITY 1193
#define PRIORITY 1194
#define PATTERN 1195

#define MAXSCALEDENOM 1196
#define MINSCALEDENOM 1197
#define LABELMAXSCALEDENOM 1198
#define LABELMINSCALEDENOM 1199
#define SYMBOLSCALEDENOM 1200
#define SCALEDENOM 1201

#define CLASSGROUP 1202

#define ALIGN 1203 /* bug 2468 */

#define MAXGEOWIDTH 1204
#define MINGEOWIDTH 1205

#define ITEMS 1206

/* rfc40 label wrapping */
#define MAXLENGTH 1210
#define MINLENGTH 1211

/* rfc44 URL configuration support */
#define VALIDATION 1212

/* rfc48 geometry transforms */
#define GEOMTRANSFORM 1220

/* rfc55 output resolution */
#define DEFRESOLUTION 1221

/* label repeat enhancement */
#define REPEATDISTANCE 1222

/* rfc60 label collision detection */
#define MAXOVERLAPANGLE 1223

/* rfc66 temporary path */
#define TEMPPATH 1224

/* rfc68 union connection type */
#define UNION 1225

/* rfc69 cluster */
#define CLUSTER 1226
#define MAXDISTANCE 1227
#define REGION 1228

#define INITIALGAP 1229
#define ANCHORPOINT 1230

#define MASK 1250

#define POLAROFFSET 1251

/* rfc78 leader-lines labels */
#define LEADER 1260
#define GRIDSTEP 1261

/* rfc 86 scale-dependant token substitutions */
#define SCALETOKEN 1270
#define VALUES 1271

#define TILESRS 1272

/* rfc 93 support for utfgrid */
#define UTFDATA 1280
#define UTFITEM 1281

/* rfc 113 layer compositing */
#define COMPOSITE 1290
#define COMPOP 1291
#define COMPFILTER 1292


#define BOM 1300

/* rfc59 bindvals objects */
#define BINDVALS 2000

#define CONNECTIONOPTIONS 2001

#endif /* MAPFILE_H */
