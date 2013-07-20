#ifndef CLROBJECT_H_
#define CLROBJECT_H_

#include "node-clr.h"


// node object wrapper that holds System::Object^
class CLRObject : public node::ObjectWrap
{
public:
	static v8::Handle<v8::Object> Wrap(v8::Handle<v8::Object> obj, System::Object^ value);

	static v8::Handle<v8::Object> Wrap(System::Object^ value);

	static bool IsWrapped(v8::Handle<v8::Value> obj);

	static System::Object^ Unwrap(v8::Handle<v8::Value> obj);

	static v8::Local<v8::Function> CreateConstructor(v8::Handle<v8::String> typeName, v8::Handle<v8::Function> initializer);

private:
	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	gcroot<System::Object^> value;

	CLRObject(System::Object^ value);

	~CLRObject();
};

#endif
