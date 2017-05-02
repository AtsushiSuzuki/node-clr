#pragma once

#include "clr.h"


class CLRType : public Nan::ObjectWrap
{
public:
    gcroot<System::Type^> type_;

private:
	Nan::Persistent<v8::Object> constructor_;
	static gcroot<System::Collections::Generic::Dictionary<System::Type^, System::UIntPtr>^> cache_;

public:
	static CLRType* GetInstance(System::Type^ type);
    v8::Local<v8::Object> ToV8Value();
	static bool IsCLRType(v8::Local<v8::Value> value);
	static CLRType* Unwrap(v8::Local<v8::Value> value);

private:
	CLRType(System::Type^ type);
	~CLRType();

	static v8::Local<v8::ObjectTemplate> Template(System::Type^ type);
	static v8::Local<v8::ObjectTemplate> PrototypeTemplate(System::Type^ type);

	static void ConstructCallback(
		const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void MakeGenericCallback(
		const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void GetNestedTypeCallback(
		v8::Local<v8::String> property,
		const Nan::PropertyCallbackInfo<v8::Value>& info);
	static void GetStaticFieldCallback(
		v8::Local<v8::String> property,
		const Nan::PropertyCallbackInfo<v8::Value>& info);
	static void SetStaticFieldCallback(
		v8::Local<v8::String> property,
		v8::Local<v8::Value> value,
		const Nan::PropertyCallbackInfo<void>& info);
	static void GetStaticPropertyCallback(
		v8::Local<v8::String> property,
		const Nan::PropertyCallbackInfo<v8::Value>& info);
	static void SetStaticPropertyCallback(
		v8::Local<v8::String> property,
		v8::Local<v8::Value> value,
		const Nan::PropertyCallbackInfo<void>& info);
	static void InvokeStaticMethodCallback(
		const Nan::FunctionCallbackInfo<v8::Value>& info);
	static void GetFieldCallback(
		v8::Local<v8::String> property,
		const Nan::PropertyCallbackInfo<v8::Value>& info);
	static void SetFieldCallback(
		v8::Local<v8::String> property,
		v8::Local<v8::Value> value,
		const Nan::PropertyCallbackInfo<void>& info);
	static void GetPropertyCallback(
		v8::Local<v8::String> property,
		const Nan::PropertyCallbackInfo<v8::Value>& info);
	static void SetPropertyCallback(
		v8::Local<v8::String> property,
		v8::Local<v8::Value> value,
		const Nan::PropertyCallbackInfo<void>& info);
	static void InvokeMethodCallback(
		const Nan::FunctionCallbackInfo<v8::Value>& info);
};
