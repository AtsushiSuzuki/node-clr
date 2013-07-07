#ifndef V8ASYNCINVOCATION_H_
#define V8ASYNCINVOCATION_H_

#include "node-clr.h"


class V8AsyncInvocation
{
public:
	V8AsyncInvocation(V8Function^ function, array<System::Object^>^ args);
	~V8AsyncInvocation();
	System::Object^ InvokeAsync();

private:
	uv_sem_t semaphore;
	gcroot<V8Function^> function;
	gcroot<array<System::Object^>^> args;
	gcroot<System::Object^> result;
	
	static void AsyncCallback(uv_async_t* handle, int status);
};

#endif
