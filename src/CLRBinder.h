#ifndef CLRBINDER_H_
#define CLRBINDER_H_

#include "node-clr.h"


enum {
	INCOMPATIBLE = 0,
	EXPLICIT_CONVERSION = 1,
	IMPLICIT_CONVERSION = 2,
	EXACT = 3,
};

class CLRBinder
{
public:
	static System::Object^ InvokeConstructor(
		v8::Handle<v8::Value> typeName,
		const v8::Arguments& args);

	static v8::Handle<v8::Value> InvokeMethod(
		v8::Handle<v8::Value> typeName,
		v8::Handle<v8::Value> name,
		v8::Handle<v8::Value> target,
		v8::Handle<v8::Value> args);

	static v8::Handle<v8::Value> GetField(
		v8::Handle<v8::Value> typeName,
		v8::Handle<v8::Value> name,
		v8::Handle<v8::Value> target);

	static void SetField(
		v8::Handle<v8::Value> typeName,
		v8::Handle<v8::Value> name,
		v8::Handle<v8::Value> target,
		v8::Handle<v8::Value> value);

private:
	static System::Object^ CLRBinder::InvokeConstructor(
		System::Type^ type,
		v8::Handle<v8::Array> args);

	static v8::Handle<v8::Value> InvokeMethod(
		System::Type^ type,
		System::String^ name,
		System::Object^ target,
		v8::Handle<v8::Array> args);
	
	static v8::Handle<v8::Value> GetField(
		System::Type^ type,
		System::String^ name,
		System::Object^ target);
	
	static void SetField(
		System::Type^ type,
		System::String^ name,
		System::Object^ target,
		v8::Handle<v8::Value> value);

	static System::Reflection::MethodBase^ SelectMethod(
		array<System::Reflection::MethodBase^>^ methods,
		v8::Handle<v8::Array> args);

	static array<System::Object^>^ BindToMethod(
		System::Reflection::MethodBase^ method,
		v8::Handle<v8::Array> args);

	static array<System::Object^>^ BindToMethod(
		System::Reflection::MethodBase^ method,
		v8::Handle<v8::Array> args,
		int% match);

	static System::Reflection::MethodBase^ FindMostSpecificMethod(
		array<System::Reflection::MethodBase^>^ methods,
		v8::Handle<v8::Array> args);

	static int CompareMethods(
		System::Reflection::MethodBase^ lhs,
		System::Reflection::MethodBase^ rhs);

	static int CompareTypes(
		System::Type^ lhs,
		System::Type^ rhs);
};

#endif
