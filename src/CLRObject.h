#ifndef CLROBJECT_H_
#define CLROBJECT_H_

#include "node-clr.h"


// node object wrapper that holds System::Object^
class CLRObject : public node::ObjectWrap
{
public:
	static void Init();

	static v8::Handle<v8::Object> Wrap(v8::Handle<v8::Object> obj, System::Object^ value);

	static v8::Handle<v8::Object> Wrap(System::Object^ value);

	static bool IsWrapped(v8::Handle<v8::Value> obj);

	static System::Object^ Unwrap(v8::Handle<v8::Value> obj);

	static v8::Local<v8::Function> CreateConstructor(v8::Handle<v8::String> typeName, v8::Handle<v8::Function> initializer);

private:
	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	static v8::Persistent<v8::ObjectTemplate> objectTemplate_;

	gcroot<System::Object^> value_;

	CLRObject(System::Object^ value);

	~CLRObject();
};

#endif
