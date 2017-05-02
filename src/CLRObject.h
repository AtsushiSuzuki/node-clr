#pragma once
#include "clr.h"


class CLRObject : public Nan::ObjectWrap
{
public:
    gcroot<System::Object^> object_;

public:
    CLRObject(System::Object^ value);
	~CLRObject();
	v8::Local<v8::Object> ToV8Value();
	static bool IsCLRObject(v8::Local<v8::Value> value);
	static CLRObject* Unwrap(v8::Local<v8::Value> value);
};
