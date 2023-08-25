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

#include "mapserver.h"
#include "v8_mapscript.h"

/* This file could be refactored in the future to encapsulate the global
   functions and internal use functions in a class. */

/* Handler for Javascript Exceptions. Not exposed to JavaScript, used internally.
   Most of the code from v8 shell example.
*/
void msV8ReportException(TryCatch* try_catch, const char *msg = "")
{
  HandleScope handle_scope;

  if (!try_catch || !try_catch->HasCaught()) {
    msSetError(MS_V8ERR, "%s.", "msV8ReportException()", msg);
    return;
  }

  String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = *exception;
  Handle<Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    msSetError(MS_V8ERR, "Javascript Exception: %s.", "msV8ReportException()",
               exception_string);
  } else {
    String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = *filename;
    int linenum = message->GetLineNumber();
    msSetError(MS_V8ERR, "Javascript Exception: %s:%i: %s", "msV8ReportException()",
               filename_string, linenum, exception_string);
    String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = *sourceline;
    msSetError(MS_V8ERR, "Exception source line: %s", "msV8ReportException()",
               sourceline_string);
    String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = *stack_trace;
      msSetError(MS_V8ERR, "Exception StackTrace: %s", "msV8ReportException()",
                 stack_trace_string);
    }
  }
}

/* This function load a javascript file in memory. */
static Handle<Value> msV8ReadFile(V8Context *v8context, const char *path)
{
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    char err[MS_MAXPATHLEN+21];
    sprintf(err, "Error opening file: %s", path);
    msV8ReportException(NULL, err);
    return Handle<String>(String::New(""));
  }

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
    if (read == 0) {
      delete [] chars;
      fclose(file);
      msDebug("msV8ReadFile: error while reading file '%s'\n", path);
      return Undefined();
    }
    i += read;
  }

  fclose(file);
  Handle<String> result = String::New(chars, size);
  delete[] chars;
  return result;
}

/* Returns a compiled javascript script. */
static Handle<Script> msV8CompileScript(Handle<String> source, Handle<String> script_name)
{
  TryCatch try_catch;

  if (source.IsEmpty() || source->Length() == 0) {
    msV8ReportException(NULL, "No source to compile");
    return Handle<Script>();
  }

  Handle<Script> script = Script::Compile(source, script_name);
  if (script.IsEmpty() && try_catch.HasCaught()) {
    msV8ReportException(&try_catch);
  }

  return script;
}

/* Runs a compiled script */
static Handle<Value> msV8RunScript(Handle<Script> script)
{
  if (script.IsEmpty()) {
    ThrowException(String::New("No script to run"));
    return Handle<Value>();
  }

  TryCatch try_catch;
  Handle<Value> result = script->Run();

  if (result.IsEmpty() && try_catch.HasCaught()) {
    msV8ReportException(&try_catch);
  }

  return result;
}

/* Execute a javascript file */
static Handle<Value> msV8ExecuteScript(const char *path, int throw_exception = MS_FALSE)
{
  char fullpath[MS_MAXPATHLEN];
  map<string, Persistent<Script> >::iterator it;
  Isolate *isolate = Isolate::GetCurrent();
  V8Context *v8context = (V8Context*)isolate->GetData();

  /* construct the path */
  msBuildPath(fullpath, v8context->paths.top().c_str(), path);
  char *filepath = msGetPath((char*)fullpath);
  v8context->paths.push(filepath);
  free(filepath);

  Handle<Script> script;
  it = v8context->scripts.find(fullpath);
  if (it == v8context->scripts.end()) {
    Handle<Value> source = msV8ReadFile(v8context, fullpath);
    Handle<String> script_name = String::New(msStripPath((char*)path));
    script = msV8CompileScript(source->ToString(), script_name);
    if (script.IsEmpty()) {
      v8context->paths.pop();
      if (throw_exception) {
        return ThrowException(String::New("Error compiling script"));
      }
    }
    /* cache the compiled script */
    Persistent<Script> pscript;
    pscript.Reset(isolate, script);
    v8context->scripts[fullpath] = pscript;
  } else {
    script = v8context->scripts[fullpath];
  }

  Handle<Value> result = msV8RunScript(script);
  v8context->paths.pop();
  if (result.IsEmpty() && throw_exception) {
    return ThrowException(String::New("Error running script"));
  }

  return result;
}

/* END OF INTERNAL JAVASCRIPT FUNCTIONS */

/* JAVASCRIPT EXPOSED FUNCTIONS */

/* JavaScript Function to load javascript dependencies.
   Exposed to JavaScript as 'require()'. */
static Handle<Value> msV8Require(const Arguments& args)
{
  TryCatch try_catch;

  for (int i = 0; i < args.Length(); i++) {
    String::Utf8Value filename(args[i]);
    msV8ExecuteScript(*filename, MS_TRUE);
    if (try_catch.HasCaught()) {
      return try_catch.ReThrow();
    }
  }

  return Undefined();
}

/* Javascript Function print: print to debug file.
   Exposed to JavaScript as 'print()'. */
static Handle<Value> msV8Print(const Arguments& args)
{
  for (int i = 0; i < args.Length(); i++) {
    String::Utf8Value str(args[i]);
    msDebug("msV8Print: %s\n", *str);
  }

  return Undefined();
}

/* END OF JAVASCRIPT EXPOSED FUNCTIONS */

/* Create and return a v8 context. Thread safe. */
void msV8CreateContext(mapObj *map)
{

  Isolate *isolate = Isolate::GetCurrent();
  Isolate::Scope isolate_scope(isolate);
  HandleScope handle_scope(isolate);

  V8Context *v8context = new V8Context(isolate);

  Handle<ObjectTemplate> global_templ = ObjectTemplate::New();
  global_templ->Set(String::New("require"), FunctionTemplate::New(msV8Require));
  global_templ->Set(String::New("print"), FunctionTemplate::New(msV8Print));
  global_templ->Set(String::New("alert"), FunctionTemplate::New(msV8Print));

  Handle<Context> context_ = Context::New(v8context->isolate, NULL, global_templ);
  v8context->context.Reset(v8context->isolate, context_);

  /* we have to enter the context before getting global instance */
  Context::Scope context_scope(context_);
  Handle<Object> global = context_->Global();
  Shape::Initialize(global);
  Point::Initialize(global);
  Line::Initialize(global);

  v8context->paths.push(map->mappath);
  v8context->isolate->SetData(v8context);
  v8context->layer = NULL;

  map->v8context = (void*)v8context;
}

void msV8ContextSetLayer(mapObj *map, layerObj *layer)
{
  V8Context* v8context = V8CONTEXT(map);

  if (!v8context) {
    msSetError(MS_V8ERR, "V8 Persistent Context is not created.", "msV8ContextSetLayer()");
    return;
  }

  v8context->layer = layer;
}

static void msV8FreeContextScripts(V8Context *v8context)
{
  map<string, Persistent<Script> >::iterator it;
  for(it=v8context->scripts.begin(); it!=v8context->scripts.end(); ++it)
  {
    ((*it).second).Dispose();
  }
}

void msV8FreeContext(mapObj *map)
{
  V8Context* v8context = V8CONTEXT(map);
  Shape::Dispose();
  Point::Dispose();
  Line::Dispose();
  msV8FreeContextScripts(v8context);
  v8context->context.Dispose();
  delete v8context;
}

/* Create a V8 execution context, execute a script and return the feature
 * style. */
char* msV8GetFeatureStyle(mapObj *map, const char *filename, layerObj *layer, shapeObj *shape)
{
  TryCatch try_catch;
  V8Context* v8context = V8CONTEXT(map);
  char *ret = NULL;
    
  if (!v8context) {
    msSetError(MS_V8ERR, "V8 Persistent Context is not created.", "msV8ReportException()");
    return NULL;
  }

  Isolate::Scope isolate_scope(v8context->isolate);
  HandleScope handle_scope(v8context->isolate);

  /* execution context */
  Local<Context> context = Local<Context>::New(v8context->isolate, v8context->context);
  Context::Scope context_scope(context);
  Handle<Object> global = context->Global();

  Shape *shape_ = new Shape(shape);
  shape_->setLayer(layer); // hack to set the attribute names, should change in future.
  shape_->disableMemoryHandler(); /* the internal object should not be freed by the v8 GC */
  Handle<Value> ext = External::New(shape_);
  global->Set(String::New("shape"),
              Shape::Constructor()->NewInstance(1, &ext));
  
  msV8ExecuteScript(filename);
  Handle<Value> value = global->Get(String::New("styleitem"));
  if (value->IsUndefined()) {
    msDebug("msV8GetFeatureStyle: Function 'styleitem' is missing.\n");
    return ret;
  }
  Handle<Function> func = Handle<Function>::Cast(value);
  Handle<Value> result = func->Call(global, 0, 0);
  if (result.IsEmpty() && try_catch.HasCaught()) {
    msV8ReportException(&try_catch);
  }

  if (!result.IsEmpty() && !result->IsUndefined()) {
     String::AsciiValue ascii(result);
     ret = msStrdup(*ascii);
  }

  return ret;
}

/* for geomtransform, we don't have the mapObj */
shapeObj *msV8TransformShape(shapeObj *shape, const char* filename)
{
  TryCatch try_catch;
  Isolate *isolate = Isolate::GetCurrent();
  V8Context *v8context = (V8Context*)isolate->GetData();

  HandleScope handle_scope(v8context->isolate);

  /* execution context */
  Local<Context> context = Local<Context>::New(v8context->isolate, v8context->context);
  Context::Scope context_scope(context);
  Handle<Object> global = context->Global();

  Shape* shape_ = new Shape(shape);
  shape_->setLayer(v8context->layer);
  shape_->disableMemoryHandler();
  Handle<Value> ext = External::New(shape_);
  global->Set(String::New("shape"),
              Shape::Constructor()->NewInstance(1, &ext));

  msV8ExecuteScript(filename);
  Handle<Value> value = global->Get(String::New("geomtransform"));
  if (value->IsUndefined()) {
    msDebug("msV8TransformShape: Function 'geomtransform' is missing.\n");
    return NULL;
  }
  Handle<Function> func = Handle<Function>::Cast(value);
  Handle<Value> result = func->Call(global, 0, 0);
  if (result.IsEmpty() && try_catch.HasCaught()) {
    msV8ReportException(&try_catch);
  }

  if (!result.IsEmpty() && result->IsObject()) {
    Handle<Object> obj = result->ToObject();
    if (obj->GetConstructorName()->Equals(String::New("shapeObj"))) {
      Shape* new_shape = ObjectWrap::Unwrap<Shape>(result->ToObject());
      if (shape == new_shape->get()) {
        shapeObj *new_shape_ = (shapeObj *)msSmallMalloc(sizeof(shapeObj));
        msInitShape(new_shape_);
        msCopyShape(shape, new_shape_);
        return new_shape_;
      }
      else {
        new_shape->disableMemoryHandler();
        return new_shape->get();
      }
    }
  }

  return NULL;
}

#endif /* USE_V8_MAPSCRIPT */
