#ifndef V8CALLBACK_H_
#define V8CALLBACK_H_

#include "node-clr.h"

ref class V8Callback
{
public:
	V8Callback(v8::Handle<v8::Function> function);
	~V8Callback();
	System::Object^ Invoke(array<System::Object^>^ args);

private:
	v8::Persistent<v8::Function>* function;
};

#endif