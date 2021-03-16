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

Persistent<FunctionTemplate> Point::constructor;

void Point::Initialize(Handle<Object> target)
{
  HandleScope scope;

  Handle<FunctionTemplate> c = FunctionTemplate::New(Point::New);
  c->InstanceTemplate()->SetInternalFieldCount(1);
  c->SetClassName(String::NewSymbol("pointObj"));

  SET_ATTRIBUTE(c, "x", getProp, setProp);
  SET_ATTRIBUTE(c, "y", getProp, setProp);
  SET_ATTRIBUTE(c, "z", getProp, setProp);
  SET_ATTRIBUTE(c, "m", getProp, setProp);

  NODE_SET_PROTOTYPE_METHOD(c, "setXY", setXY);
  NODE_SET_PROTOTYPE_METHOD(c, "setXYZ", setXYZ);

  target->Set(String::NewSymbol("pointObj"), c->GetFunction());

  constructor.Reset(Isolate::GetCurrent(), c);
}

void Point::Dispose()
{
  Point::constructor.Dispose();
  Point::constructor.Clear();
}

Point::~Point()
{
  if (this->freeInternal) {
    msFree(this->get());
  }
}

Handle<Function> Point::Constructor()
{
  return (*Point::constructor)->GetFunction();
}

void Point::New(const v8::FunctionCallbackInfo<Value>& args)
{
  HandleScope scope;

  if (args[0]->IsExternal())
  {
    Local<External> ext = Local<External>::Cast(args[0]);
    void *ptr = ext->Value();
    Point *point = static_cast<Point*>(ptr);
    Handle<Object> self = args.Holder();
    point->Wrap(self);
    if (point->parent_) {
      self->SetHiddenValue(String::New("__parent__"), point->parent_->handle());
      point->disableMemoryHandler();
    }
  }
  else
  {
    pointObj *p = (pointObj *)msSmallMalloc(sizeof(pointObj));

    p->x = 0;
    p->y = 0;
    p->z = 0;
    p->m = 0;

    Point *point = new Point(p);
    point->Wrap(args.Holder());
  }
}

 Point::Point(pointObj *p, ObjectWrap *pa):
  ObjectWrap()
{
    this->this_ = p;
    this->parent_ = pa;
    this->freeInternal = true;
}

void Point::getProp(Local<String> property,
                    const PropertyCallbackInfo<Value>& info)
{
    HandleScope scope;
    Point* p = ObjectWrap::Unwrap<Point>(info.Holder());
    std::string name = TOSTR(property);
    Handle<Value> value = Undefined();
    
    if (name == "x")
      value = Number::New(p->get()->x);
    else if (name == "y")
      value = Number::New(p->get()->y);
    else if (name == "z")
      value = Number::New(p->get()->z);
    else if (name == "m")
      value = Number::New(p->get()->m);

    info.GetReturnValue().Set(value);
}

void Point::setProp(Local<String> property,
                    Local<Value> value,
                    const PropertyCallbackInfo<void>& info)
{
    HandleScope scope;
    Point* p = ObjectWrap::Unwrap<Point>(info.Holder());
    std::string name = TOSTR(property);
    if (!value->IsNumber())
      ThrowException(Exception::TypeError(
                           String::New("point value must be a number")));
    if (name == "x") {
      p->get()->x = value->NumberValue();
    } else if (name == "y") {
      p->get()->y = value->NumberValue();
    }
    else if (name == "z") {
      p->get()->z = value->NumberValue();
    } else if (name == "m") {
      p->get()->m = value->NumberValue();
    }
}

void Point::setXY(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  HandleScope scope;

  if (args.Length() < 2 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Point* p = ObjectWrap::Unwrap<Point>(args.Holder());

  p->get()->x = args[0]->NumberValue();
  p->get()->y = args[1]->NumberValue();

}

void Point::setXYZ(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  HandleScope scope;

  if (args.Length() < 3 ||
      !args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
    ThrowException(String::New("Invalid argument"));
    return;
  }

  Point* p = ObjectWrap::Unwrap<Point>(args.Holder());

  p->get()->x = args[0]->NumberValue();
  p->get()->y = args[1]->NumberValue();

  p->get()->z = args[2]->NumberValue();
  if (args.Length() > 3 && args[3]->IsNumber()) {
    p->get()->m = args[3]->NumberValue();
  }
}

#endif
