// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef V8_OBJECT_WRAP_H_
#define V8_OBJECT_WRAP_H_

#include "v8.h"
#include <assert.h>

class ObjectWrap {
 public:
  ObjectWrap() {
    refs_ = 0;
  }

  virtual ~ObjectWrap() {
    if (persistent().IsEmpty())
      return;
    assert(persistent().IsNearDeath());
    persistent().ClearWeak();
    persistent().Dispose();
  }


  template <class T>
  static inline T* Unwrap(v8::Handle<v8::Object> handle) {
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    // Cast to ObjectWrap before casting to T.  A direct cast from void
    // to T won't work right when T has more than one base class.
    void* ptr = handle->GetAlignedPointerFromInternalField(0);
    ObjectWrap* wrap = static_cast<ObjectWrap*>(ptr);
    return static_cast<T*>(wrap);
  }


  inline v8::Local<v8::Object> handle() {
    return handle(v8::Isolate::GetCurrent());
  }


  inline v8::Local<v8::Object> handle(v8::Isolate* isolate) {
    return v8::Local<v8::Object>::New(isolate, persistent());
  }


  inline v8::Persistent<v8::Object>& persistent() {
    return handle_;
  }


 protected:
  inline void Wrap(v8::Handle<v8::Object> handle) {
    assert(persistent().IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle->SetAlignedPointerInInternalField(0, this);
    persistent().Reset(v8::Isolate::GetCurrent(), handle);
    MakeWeak();
  }


  inline void MakeWeak(void) {
    persistent().MakeWeak(this, WeakCallback);
    persistent().MarkIndependent();
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
    assert(!persistent().IsEmpty());
    persistent().ClearWeak();
    refs_++;
  }

  /* Unref() marks an object as detached from the event loop.  This is its
   * default state.  When an object with a "weak" reference changes from
   * attached to detached state it will be freed. Be careful not to access
   * the object after making this call as it might be gone!
   * (A "weak reference" means an object that only has a
   * persistant handle.)
   *
   * DO NOT CALL THIS FROM DESTRUCTOR
   */
  virtual void Unref() {
    assert(!persistent().IsEmpty());
    assert(!persistent().IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0)
      MakeWeak();
  }

  int refs_;  // ro

 private:
  static void WeakCallback(v8::Isolate* isolate,
                           v8::Persistent<v8::Object>* pobj,
                           ObjectWrap* wrap) {
    v8::HandleScope scope(isolate);
    assert(wrap->refs_ == 0);
    assert(*pobj == wrap->persistent());
    assert((*pobj).IsNearDeath());
    delete wrap;
  }

  v8::Persistent<v8::Object> handle_;
};


#endif  // v8_OBJECT_WRAP_H_
