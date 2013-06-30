#ifndef CLROBJECT_H_
#define CLROBJECT_H_

#include "node-clr.h"


class CLRObject : public node::ObjectWrap
{
public:
	static v8::Local<v8::Object> Wrap(System::Object^ wrapped);

	static bool IsWrapped(v8::Handle<v8::Value> wrapper);

	static System::Object^ Unwrap(v8::Handle<v8::Value> wrapper);

	static v8::Handle<v8::Value> CreateConstructor(const v8::Arguments& args);

private:
	static v8::Handle<v8::Value> New(const v8::Arguments& args);

	gcroot<System::Object^> wrapped;
	
	CLRObject(System::Object^ wrapped);

	~CLRObject();
};

#endif
