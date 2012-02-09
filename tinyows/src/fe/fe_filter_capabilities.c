/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE. 
*/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../ows/ows.h"


/* 
 * Describe functions supported by the wfs
 */
static void fe_functions_capabilities(const ows * o)
{
	int version;
	buffer *fct_name;

	assert(o != NULL);

	version = ows_version_get(o->request->version);
	fct_name = buffer_init();
	if (version == 100)
		buffer_add_str(fct_name, "Function_Name");
	else
		buffer_add_str(fct_name, "FunctionName");

	fprintf(o->output, "   <ogc:Functions>\n");
	fprintf(o->output, "    <ogc:%ss>\n", fct_name->buf);
	fprintf(o->output,
	   "     <ogc:%s nArgs='1'>abs</ogc:%s>\n", fct_name->buf,
	   fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>acos</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>asin</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>atan</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>avg</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>cbrt</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>ceil</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>ceiling</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>cos</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>cot</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>count</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>degrees</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>exp</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>floor</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>length</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>ln</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>log</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>min</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>max</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>radians</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>round</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>sin</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>sqrt</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>tan</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     <ogc:%s nArgs='1'>trunc</ogc:%s>\n",
	   fct_name->buf, fct_name->buf);
	fprintf(o->output, "     </ogc:%ss>\n", fct_name->buf);
	fprintf(o->output, "     </ogc:Functions>\n");

	buffer_free(fct_name);
}


/* 
 * Describe what specific filter capabilties are supported by the wfs server 
 * Version 1.0.0 Filter Encoding 
 */
void fe_filter_capabilities_100(const ows * o)
{

	assert(o != NULL);

	fprintf(o->output, "<ogc:Filter_Capabilities>\n");

	/* Spatial Capabilities */
	fprintf(o->output, " <ogc:Spatial_Capabilities>\n");
	fprintf(o->output, "  <ogc:Spatial_Operators>\n");
	fprintf(o->output, "  <ogc:Disjoint/>\n");
	fprintf(o->output, "  <ogc:Equals/>\n");
	fprintf(o->output, "  <ogc:DWithin/>\n");
	fprintf(o->output, "  <ogc:Beyond/>\n");
	fprintf(o->output, "  <ogc:Intersect/>\n");
	fprintf(o->output, "  <ogc:Touches/>\n");
	fprintf(o->output, "  <ogc:Crosses/>\n");
	fprintf(o->output, "  <ogc:Within/>\n");
	fprintf(o->output, "  <ogc:Contains/>\n");
	fprintf(o->output, "  <ogc:Overlaps/>\n");
	fprintf(o->output, "  <ogc:BBOX/>\n");
	fprintf(o->output, " </ogc:Spatial_Operators>\n");
	fprintf(o->output, " </ogc:Spatial_Capabilities>\n");

	/* Scalar Capabilities */
	fprintf(o->output, " <ogc:Scalar_Capabilities>\n");
	fprintf(o->output, "  <ogc:Logical_Operators/>\n");
	fprintf(o->output, "  <ogc:Comparison_Operators>\n");
	fprintf(o->output, "   <ogc:Simple_Comparisons/>\n");
	fprintf(o->output, "   <ogc:Between/>\n");
	fprintf(o->output, "   <ogc:Like/>\n");
	fprintf(o->output, "   <ogc:NullCheck/>\n");
	fprintf(o->output, "  </ogc:Comparison_Operators>\n");
	fprintf(o->output, "  <ogc:Arithmetic_Operators>\n");
	fprintf(o->output, "   <ogc:Simple_Arithmetic/>\n");
	fe_functions_capabilities(o);
	fprintf(o->output, "  </ogc:Arithmetic_Operators>\n");
	fprintf(o->output, " </ogc:Scalar_Capabilities>\n");

	fprintf(o->output, "</ogc:Filter_Capabilities>\n");
}


/* 
 * Describe what specific filter capabilties are supported by the wfs server
 * Version 1.1.0 Filter Encoding
 */
void fe_filter_capabilities_110(const ows * o)
{

	assert(o != NULL);

	fprintf(o->output, "<ogc:Filter_Capabilities>\n");

	/* Spatial Capabililties */
	fprintf(o->output, " <ogc:Spatial_Capabilities>\n");

	fprintf(o->output, "  <ogc:GeometryOperands>\n");
	fprintf(o->output,
	   "   <ogc:GeometryOperand>gml:Envelope</ogc:GeometryOperand>\n");
	fprintf(o->output,
	   "   <ogc:GeometryOperand>gml:Point</ogc:GeometryOperand>\n");
	fprintf(o->output,
	   "   <ogc:GeometryOperand>gml:LineString</ogc:GeometryOperand>\n");
	fprintf(o->output,
	   "   <ogc:GeometryOperand>gml:Polygon</ogc:GeometryOperand>\n");
	fprintf(o->output, "  </ogc:GeometryOperands>\n");

	fprintf(o->output, "  <ogc:SpatialOperators>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Disjoint'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Equals'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='DWithin'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Beyond'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Intersects'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Touches'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Crosses'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Within'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Contains'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='Overlaps'/>\n");
	fprintf(o->output, "  <ogc:SpatialOperator name='BBOX'/>\n");
	fprintf(o->output, " </ogc:SpatialOperators>\n");
	fprintf(o->output, " </ogc:Spatial_Capabilities>\n");

	/* Scalar Capabililties */
	fprintf(o->output, " <ogc:Scalar_Capabilities>\n");
	fprintf(o->output, "  <ogc:LogicalOperators/>\n");

	fprintf(o->output, " 	<ogc:ComparisonOperators>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>EqualTo</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>NotEqualTo</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>LessThan</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>GreaterThan</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>LessThanEqualTo</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>GreaterThanEqualTo</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>Between</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>Like</ogc:ComparisonOperator>\n");
	fprintf(o->output,
	   "   <ogc:ComparisonOperator>NullCheck</ogc:ComparisonOperator>\n");
	fprintf(o->output, "  </ogc:ComparisonOperators>\n");

	fprintf(o->output, "  <ogc:ArithmeticOperators>\n");
	fprintf(o->output, "   <ogc:SimpleArithmetic/>\n");
	fe_functions_capabilities(o);
	fprintf(o->output, "  </ogc:ArithmeticOperators>\n");

	fprintf(o->output, " </ogc:Scalar_Capabilities>\n");

	/* Id Capabilities */
	fprintf(o->output, " <ogc:Id_Capabilities>\n");
	fprintf(o->output, "  <ogc:EID/>\n");
	fprintf(o->output, "  <ogc:FID/>\n");
	fprintf(o->output, " </ogc:Id_Capabilities>\n");

	fprintf(o->output, "</ogc:Filter_Capabilities>\n");
}


/*
 * vim: expandtab sw=4 ts=4
 */
