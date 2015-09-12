#ifndef V8FUNCTION_H_
#define V8FUNCTION_H_

#include "node-clr.h"


class V8Function
{
private:
	struct InvocationContext
	{
		gcroot<array<System::Object^>^> args;
		gcroot<System::Object^> result;
		gcroot<System::Exception^> exception;
		uv_sem_t completed;
	};

private:
	DWORD threadId;
	Nan::Persistent<v8::Function> function;
	uv_async_t async;
	uv_mutex_t lock;
	std::queue<InvocationContext*> invocations;
	bool terminate;

public:
	static V8Function* New(v8::Local<v8::Function> func);
	System::Object^ Invoke(array<System::Object^>^ args);
	void Destroy();

private:
	V8Function(v8::Local<v8::Function> func);
	System::Object^ InvokeImpl(array<System::Object^>^ args);
	System::Object^ InvokeAsync(array<System::Object^>^ args);
	static NAUV_WORK_CB(AsyncCallback);
	~V8Function();
};

#endif
