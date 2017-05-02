#pragma once

#include "clr.h"


/*
 * String conversions
 */

System::String^ ToCLRString(v8::Local<v8::Value> value);

v8::Local<v8::String> ToV8String(System::String^ value);


/*
 * Value conversion
 */

v8::Local<v8::Value> ToV8Value(System::Object^ value);

System::Object^ ToCLRValue(v8::Local<v8::Value> value);

System::Object^ ToCLRValue(v8::Local<v8::Value> value, System::Type^ type);

/*
 * Exception conversions
 */

v8::Local<v8::Value> ToV8Error(System::Exception^ ex);
