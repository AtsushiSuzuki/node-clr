#include "node-clr.h"

using namespace v8;

V8AsyncInvocation::V8AsyncInvocation(V8Function^ function, array<System::Object^>^ args)
	: semaphore(), function(function), args(args), result(nullptr)
{
	uv_sem_init(&this->semaphore, 0);
}

V8AsyncInvocation::~V8AsyncInvocation()
{
	uv_sem_destroy(&this->semaphore);
}

System::Object^ V8AsyncInvocation::InvokeAsync()
{
	uv_async_t async;
	uv_async_init(uv_default_loop(), &async, &V8AsyncInvocation::AsyncCallback);
	async.data = this;

	uv_async_send(&async);

	uv_sem_wait(&this->semaphore);
	return this->result;
}

void V8AsyncInvocation::AsyncCallback(uv_async_t* handle, int status)
{
	HandleScope scope;

	auto thiz = (V8AsyncInvocation*)handle->data;

	thiz->result = thiz->function->Invoke(thiz->args);
	
	uv_sem_post(&thiz->semaphore);
}
