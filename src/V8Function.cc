#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;

V8Function::V8Function(Handle<Function> function)
	: threadId(uv_thread_self()), func(Persistent<Function>::New(function))
{
}

V8Function::~V8Function()
{
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

System::Object^ V8Function::InvokeImpl(array<System::Object^>^ args)
{
	HandleScope scope;
	
	std::vector<Handle<Value> > params;
	for each (System::Object^ arg in args)
	{
		params.push_back(ToV8Value(arg));
	}

	TryCatch trycatch;
	auto result = this->func->Call(
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
	uv_sem_t semaphore;
	uv_sem_init(&semaphore, 0);

	uv_async_t async;
	uv_async_init(uv_default_loop(), &async, &V8Function::AsyncCallback);

	InvocationContext ctx = {
		this,
		async,
		semaphore,
		args,
		nullptr,
		nullptr
	};
	async.data = &ctx;

	uv_async_send(&async);
	uv_sem_wait(&semaphore);

	uv_sem_destroy(&semaphore);

	if (static_cast<System::Exception^>(ctx.exception) != nullptr)
	{
		throw static_cast<System::Exception^>(ctx.exception);
	}

	return ctx.result;
}

void V8Function::AsyncCallback(uv_async_t* handle, int status)
{
	auto ctx = (InvocationContext*)handle->data;

	try
	{
		ctx->result = ctx->thiz->InvokeImpl(ctx->args);
	}
	catch (System::Exception^ ex)
	{
		ctx->exception = ex;
	}

	uv_sem_post(&ctx->semaphore);
	uv_close((uv_handle_t*)(&ctx->async), nullptr);
}