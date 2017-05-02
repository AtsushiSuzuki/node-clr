#pragma once

#include "clr.h"


enum {
	INCOMPATIBLE = 0,
	EXPLICIT_CONVERSION = 1,
	IMPLICIT_CONVERSION = 2,
	EXACT = 3,
};

class Binder
{
public:
	static System::Object^ Construct(
		System::Type^ type,
		const Nan::FunctionCallbackInfo<v8::Value>& args);

	static System::Type^ GetNestedType(
		System::Type^ type,
		System::String^ name);

	static void SetField(
		System::Type^ type,
		System::Object^ target,
		System::String^ name,
		v8::Local<v8::Value> value);

	static System::Object^ GetField(
		System::Type^ type,
		System::Object^ target,
		System::String^ name);

	static void SetProperty(
		System::Type^ type,
		System::Object^ object,
		System::String^ name,
		v8::Local<v8::Value> value);

	static System::Object^ GetProperty(
		System::Type^ type,
		System::Object^ target,
		System::String^ name);

	static System::Object^ InvokeMethod(
		System::Type^ type,
		System::Object^ object,
		System::String^ name,
		const Nan::FunctionCallbackInfo<v8::Value>& args);

public:
	static int CanChangeType(
		v8::Local<v8::Value> value,
		System::Type^ type);

	static System::Object^ ChangeType(
		v8::Local<v8::Value> value,
		System::Type^ type);

	static System::Object^ Invoke(
		System::Object^ object,
		array<System::Reflection::MethodBase^>^ methods,
		const Nan::FunctionCallbackInfo<v8::Value>& args);
};
