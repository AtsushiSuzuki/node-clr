#ifndef UTIL_H_
#define UTIL_H_

#include "node-clr.h"

System::String^ CLRString(v8::Handle<v8::Value> value);
v8::Local<v8::String> V8String(System::String ^value);
v8::Local<v8::String> V8Symbol(System::String ^value);
System::Object^ CLRValue(v8::Handle<v8::Value> value);
v8::Handle<v8::Value> V8Value(System::Object^ value);
array<System::Object^>^ CLRArguments(const v8::Arguments &args);
array<System::Object^>^ CLRArguments(v8::Handle<v8::Array> args);
std::vector<v8::Handle<v8::Value> > V8Arguments(array<System::Object^>^ args);
v8::Local<v8::Value> V8Exception(System::Exception ^ex);

#endif
