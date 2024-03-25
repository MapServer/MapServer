/**********************************************************************
 *
 * Project:  MapServer
 * Purpose:  V8 JavaScript Engine Support
 * Author:   Alan Boudreault (aboudreault@mapgears.com)
 *
 **********************************************************************
 * Copyright (c) 2013, Alan Boudreault, MapGears
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "mapserver-config.h"
#ifdef USE_V8_MAPSCRIPT

#include "v8_mapscript.h"

using namespace v8;

Persistent<FunctionTemplate> Line::constructor;

void Line::Initialize(Handle<Object> target) {
  HandleScope scope;

  Handle<FunctionTemplate> c = FunctionTemplate::New(Line::New);
  c->InstanceTemplate()->SetInternalFieldCount(1);
  c->SetClassName(String::NewSymbol("lineObj"));

  SET_ATTRIBUTE_RO(c, "numpoints", getProp);

  NODE_SET_PROTOTYPE_METHOD(c, "point", getPoint);
  NODE_SET_PROTOTYPE_METHOD(c, "addXY", addXY);
  NODE_SET_PROTOTYPE_METHOD(c, "addXYZ", addXYZ);
  NODE_SET_PROTOTYPE_METHOD(c, "add", addPoint);

  target->Set(String::NewSymbol("lineObj"), c->GetFunction());

  constructor.Reset(Isolate::GetCurrent(), c);
}

void Line::Dispose() {
  Line::constructor.Dispose();
  Line::constructor.Clear();
}

Line::~Line() {
  if (this->freeInternal) {
    msFree(this->this_->point);
    msFree(this->this_);
  }
}

Handle<Function> Line::Constructor() {
  return (*Line::constructor)->GetFunction();
}

void Line::New(const v8::FunctionCallbackInfo<Value> &args) {
  HandleScope scope;

  if (args[0]->IsExternal()) {
    Local<External> ext = Local<External>::Cast(args[0]);
    void *ptr = ext->Value();
    Line *line = static_cast<Line *>(ptr);
    Handle<Object> self = args.Holder();
    line->Wrap(self);
    if (line->parent_) {
      self->SetHiddenValue(String::New("__parent__"), line->parent_->handle());
      line->disableMemoryHandler();
    }
  } else {
    lineObj *l = (lineObj *)msSmallMalloc(sizeof(lineObj));

    l->numpoints = 0;
    l->point = NULL;

    Line *line = new Line(l);
    line->Wrap(args.Holder());
  }
}

Line::Line(lineObj *l, ObjectWrap *p) : ObjectWrap() {
  this->this_ = l;
  this->parent_ = p;
  this->freeInternal = true;
}

void Line::getProp(Local<String> property,
                   const PropertyCallbackInfo<Value> &info) {
  HandleScope scope;
  Line *l = ObjectWrap::Unwrap<Line>(info.Holder());
  std::string name = TOSTR(property);
  Handle<Value> value = Undefined();

  if (name == "numpoints")
    value = Integer::New(l->get()->numpoints);

  info.GetReturnValue().Set(value);
}

void Line::getPoint(const v8::FunctionCallbackInfo<Value> &args) {
  HandleScope scope;

  if (args.Length() < 1 || !args[0]->IsInt32()) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Line *l = ObjectWrap::Unwrap<Line>(args.Holder());
  lineObj *line = l->get();

  int index = args[0]->Int32Value();

  if (index < 0 || index >= line->numpoints) {
    ThrowException(String::New("Invalid point index."));
    return;
  }

  Point *point = new Point(&line->point[index], l);
  Handle<Value> ext = External::New(point);
  args.GetReturnValue().Set(Point::Constructor()->NewInstance(1, &ext));
}

void Line::addXY(const v8::FunctionCallbackInfo<Value> &args) {
  HandleScope scope;

  if (args.Length() < 2 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Line *l = ObjectWrap::Unwrap<Line>(args.Holder());
  lineObj *line = l->get();

  if (line->numpoints == 0) /* new */
    line->point = (pointObj *)msSmallMalloc(sizeof(pointObj));
  else /* extend array */
    line->point = (pointObj *)msSmallRealloc(
        line->point, sizeof(pointObj) * (line->numpoints + 1));

  line->point[line->numpoints].x = args[0]->NumberValue();
  line->point[line->numpoints].y = args[1]->NumberValue();
  if (args.Length() > 2 && args[2]->IsNumber())
    line->point[line->numpoints].m = args[2]->NumberValue();

  line->numpoints++;
}

void Line::addXYZ(const v8::FunctionCallbackInfo<Value> &args) {
  HandleScope scope;

  if (args.Length() < 3 || !args[0]->IsNumber() || !args[1]->IsNumber() ||
      !args[2]->IsNumber()) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Line *l = ObjectWrap::Unwrap<Line>(args.Holder());
  lineObj *line = l->get();

  if (line->numpoints == 0) /* new */
    line->point = (pointObj *)msSmallMalloc(sizeof(pointObj));
  else /* extend array */
    line->point = (pointObj *)msSmallRealloc(
        line->point, sizeof(pointObj) * (line->numpoints + 1));

  line->point[line->numpoints].x = args[0]->NumberValue();
  line->point[line->numpoints].y = args[1]->NumberValue();
  line->point[line->numpoints].z = args[2]->NumberValue();
  if (args.Length() > 3 && args[3]->IsNumber())
    line->point[line->numpoints].m = args[3]->NumberValue();
  line->numpoints++;
}

void Line::addPoint(const v8::FunctionCallbackInfo<Value> &args) {
  HandleScope scope;

  if (args.Length() < 1 || !args[0]->IsObject() ||
      !args[0]->ToObject()->GetConstructorName()->Equals(
          String::New("pointObj"))) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Line *l = ObjectWrap::Unwrap<Line>(args.Holder());
  lineObj *line = l->get();
  Point *p = ObjectWrap::Unwrap<Point>(args[0]->ToObject());
  pointObj *point = p->get();

  if (line->numpoints == 0) /* new */
    line->point = (pointObj *)msSmallMalloc(sizeof(pointObj));
  else /* extend array */
    line->point = (pointObj *)msSmallRealloc(
        line->point, sizeof(pointObj) * (line->numpoints + 1));

  line->point[line->numpoints].x = point->x;
  line->point[line->numpoints].y = point->y;
  line->point[line->numpoints].z = point->z;
  if (args.Length() > 3 && args[3]->IsNumber())
    line->point[line->numpoints].m = point->m;
  line->numpoints++;
}

#endif
