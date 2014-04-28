#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;


V8Function* V8Function::New(Handle<Function> func)
{
	return new V8Function(func);
}

V8Function::V8Function(Handle<Function> func)
	: threadId(uv_thread_self()), function(Persistent<Function>::New(func)), invocations(), terminate(false)
{
	uv_async_init(uv_default_loop(), &this->async, &V8Function::AsyncCallback);
	this->async.data = this;
	uv_sem_init(&this->semaphore, 0);
}

System::Object^ V8Function::Invoke(array<System::Object^>^ args)
{
	if (this->threadId == uv_thread_self())
	{
		return this->InvokeImpl(args);
	}
	else
	{
		return this->InvokeAsync(args);
	}
}

void V8Function::Destroy()
{
	this->terminate = true;
	uv_async_send(&this->async);
}

V8Function::~V8Function()
{
	uv_close((uv_handle_t*)(&this->async), nullptr);
	uv_sem_destroy(&this->semaphore);
}

System::Object^ V8Function::InvokeImpl(array<System::Object^>^ args)
{
	HandleScope scope;
	
	std::vector<Handle<Value> > params;
	for each (System::Object^ arg in args)
	{
		params.push_back(ToV8Value(arg));
	}

	TryCatch trycatch;
	auto result = this->function->Call(
		Context::GetCurrent()->Global(),
		(int)params.size(),
		(0 < params.size())
			? &(params[0])
			: nullptr);
	if (trycatch.HasCaught())
	{
		throw ToCLRException(trycatch.Exception());
	}

	return ToCLRValue(result);
}

System::Object^ V8Function::InvokeAsync(array<System::Object^>^ args)
{
	InvocationContext ctx = { args };
	// TODO: lock
	this->invocations.push_back(&ctx);

	uv_async_send(&this->async);
	uv_sem_wait(&this->semaphore);
	
	if (static_cast<System::Exception^>(ctx.exception) != nullptr)
	{
		throw static_cast<System::Exception^>(ctx.exception);
	}
	else
	{
		return ctx.result;
	}
}

void V8Function::AsyncCallback(uv_async_t* handle, int status)
{
	auto thiz = (V8Function*)handle->data;
	// TODO: lock
	std::vector<InvocationContext*> invocations(thiz->invocations);
	thiz->invocations.clear();

	for (auto it = invocations.begin(); it != invocations.end(); it++)
	{
		try
		{
			(*it)->result = thiz->InvokeImpl((*it)->args);
		}
		catch (System::Exception^ ex)
		{
			(*it)->exception = ex;
		}
		uv_sem_post(&thiz->semaphore);
	}

	if (thiz->terminate)
	{
		delete thiz;
	}
}
