#ifndef V8BINDER_H_
#define V8BINDER_H_

#include "node-clr.h"


class V8Binder
{
public:
	static v8::Handle<v8::Value> InvokeMember(
		v8::Handle<v8::String> type,
		v8::Handle<v8::String> name,
		System::Reflection::BindingFlags attr,
		v8::Handle<v8::Value> target,
		v8::Handle<v8::Array> args);
	
private:
	static v8::Handle<v8::Value> V8Binder::InvokeMember(
		System::Type^ type,
		System::String^ name,
		System::Reflection::BindingFlags attr,
		System::Object^ target,
		v8::Handle<v8::Array> args);
};

#endif