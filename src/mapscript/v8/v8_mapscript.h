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

#ifndef V8_MAPSCRIPT_H
#define V8_MAPSCRIPT_H

#include "mapserver-config.h"
#ifdef USE_V8_MAPSCRIPT

/* This need should be removed in future v8 version */
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR 1
#define V8_USE_UNSAFE_HANDLES 1

#include "mapserver.h"
#include <v8.h>
#include <string>
#include <stack>
#include <map>
#include "v8_object_wrap.hpp"
#include "point.hpp"
#include "line.hpp"
#include "shape.hpp"

using namespace v8;

using std::string;
using std::stack;
using std::map;

class V8Context
{
public:
  V8Context(Isolate *isolate)
    : isolate(isolate) {}
  Isolate *isolate;
  stack<string> paths; /* for relative paths and the require function */
  map<string, Persistent<Script> > scripts;
  Persistent<Context> context;
  layerObj *layer; /* current layer, used in geomtransform */
};

#define V8CONTEXT(map) ((V8Context*) (map)->v8context)

inline void NODE_SET_PROTOTYPE_METHOD(v8::Handle<v8::FunctionTemplate> recv,
                                      const char* name,
                                      v8::FunctionCallback callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(callback);
  recv->InstanceTemplate()->Set(v8::String::NewFromUtf8(isolate, name),
                                t->GetFunction());
}
#define NODE_SET_PROTOTYPE_METHOD NODE_SET_PROTOTYPE_METHOD

#define TOSTR(obj) (*String::Utf8Value((obj)->ToString()))

#define SET(target, name, value)                 \
  (target)->PrototypeTemplate()->Set(String::NewSymbol(name), value);

#define SET_ATTRIBUTE(t, name, get, set)   \
  t->InstanceTemplate()->SetAccessor(String::NewSymbol(name), get, set)

#define SET_ATTRIBUTE_RO(t, name, get)             \
  t->InstanceTemplate()->SetAccessor(              \
        String::NewSymbol(name),                   \
        get, 0,                                    \
        Handle<Value>(),                           \
        DEFAULT,                                   \
        static_cast<PropertyAttribute>(            \
          ReadOnly|DontDelete))

#define NODE_DEFINE_CONSTANT(target, name, constant)     \
    (target)->Set(String::NewSymbol(name),               \
                  Integer::New(constant),                \
                  static_cast<PropertyAttribute>(        \
                  ReadOnly|DontDelete));

char* getStringValue(Local<Value> value, const char *fallback="");


#endif

#endif
