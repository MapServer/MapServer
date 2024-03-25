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

#ifndef LINE_H_
#define LINE_H_

#include <v8.h>

using namespace v8;

class Line : public ObjectWrap {
public:
  static void Initialize(Handle<Object> target);
  static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Dispose();
  static Handle<Function> Constructor();

  Line(lineObj *l, ObjectWrap *p = NULL);
  ~Line();
  inline lineObj *get() { return this_; }
  inline void disableMemoryHandler() { this->freeInternal = false; }

  static void getProp(Local<String> property,
                      const PropertyCallbackInfo<Value> &info);
  static void setProp(Local<String> property, Local<Value> value,
                      const PropertyCallbackInfo<void> &info);

  static void getPoint(const v8::FunctionCallbackInfo<Value> &args);
  static void addXY(const v8::FunctionCallbackInfo<Value> &args);
  static void addXYZ(const v8::FunctionCallbackInfo<Value> &args);
  static void addPoint(const v8::FunctionCallbackInfo<Value> &args);

private:
  static Persistent<FunctionTemplate> constructor;
  lineObj *this_;
  ObjectWrap *parent_;
  bool freeInternal;
};

#endif
