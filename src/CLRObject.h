#ifndef CLROBJECT_H_
#define CLROBJECT_H_

#include "node-clr.h"


// node object wrapper that holds System::Object^
class CLRObject : public node::ObjectWrap
{
public:
	static void Init();

	static bool IsCLRObject(v8::Local<v8::Value> obj);

	static v8::Local<v8::Value> GetType(v8::Local<v8::Value> value);

	static bool IsCLRConstructor(v8::Local<v8::Value> value);

	static v8::Local<v8::Value> TypeOf(v8::Local<v8::Value> value);

	static v8::Local<v8::Object> Wrap(v8::Local<v8::Object> obj, System::Object^ value);

	static v8::Local<v8::Object> Wrap(System::Object^ value);

	static System::Object^ Unwrap(v8::Local<v8::Value> obj);

	static v8::Local<v8::Function> CreateConstructor(v8::Local<v8::String> typeName, v8::Local<v8::Function> initializer);

private:
	static NAN_METHOD(New);

	static Nan::Persistent<v8::ObjectTemplate> objectTemplate_;

	gcroot<System::Object^> value_;

	CLRObject(System::Object^ value);

	~CLRObject();
};

#endif
