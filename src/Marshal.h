#ifndef MARSHAL_H_
#define MARSHAL_H_

#include "node-clr.h"


/*
 * String conversions
 */

System::String^ ToCLRString(v8::Handle<v8::Value> value);

v8::Local<v8::String> ToV8String(System::String^ value);

v8::Local<v8::String> ToV8Symbol(System::String^ value);


/*
 * Value conversion
 */

System::Object^ ToCLRValue(v8::Handle<v8::Value> value);

v8::Handle<v8::Value> ToV8Value(System::Object^ value);


/*
 * Arguments conversions
 */

std::vector<v8::Handle<v8::Value> > ToV8Arguments(array<System::Object^>^ args);


/*
 * Exception conversions
 */

v8::Local<v8::Value> ToV8Exception(System::Exception^ ex);

#endif
