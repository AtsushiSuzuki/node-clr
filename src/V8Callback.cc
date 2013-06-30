#include "node-clr.h"

using namespace v8;

V8Callback::V8Callback(Handle<Function> function)
	: function(new Persistent<Function>(Persistent<Function>::New(function)))
{
}

V8Callback::~V8Callback()
{
	delete function;
}

System::Object^ V8Callback::Invoke(array<System::Object^>^ args)
{
	// TODO: if current thread is not javascript context thread, dispatch to libuv
	auto params = V8Arguments(args);
	auto result = (*this->function)->Call(Local<v8::Object>(), (int)params.size(), &(params[0]));

	return CLRValue(result);
}