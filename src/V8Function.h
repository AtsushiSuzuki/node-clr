#ifndef V8FUNCTION_H_
#define V8FUNCTION_H_

#include "node-clr.h"


class V8Function
{
public:
	V8Function(v8::Handle<v8::Function> func);

	~V8Function();

	System::Object^ Invoke(array<System::Object^>^ args);

private:
	unsigned int threadId;

	v8::Persistent<v8::Function> func;

	System::Object^ InvokeImpl(array<System::Object^>^ args);

	System::Object^ InvokeAsync(array<System::Object^>^ args);

	static void AsyncCallback(uv_async_t* handle, int status);


	struct InvocationContext
	{
		V8Function* thiz;

		uv_sem_t semaphore;

		gcroot<array<System::Object^>^> args;

		gcroot<System::Object^> result;

		gcroot<System::Exception^> exception;
	};
};

#endif
