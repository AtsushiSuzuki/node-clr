#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;


V8Function* V8Function::New(Handle<Function> func)
{
	return new V8Function(func);
}

V8Function::V8Function(Handle<Function> func)
	: threadId(uv_thread_self()), function(Persistent<Function>::New(func)), terminate(false)
{
	uv_async_init(uv_default_loop(), &this->async, &V8Function::AsyncCallback);
	this->async.data = this;
	uv_mutex_init(&this->lock);
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
	uv_mutex_destroy(&this->lock);
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

	uv_sem_init(&ctx.completed, 0);

	uv_mutex_lock(&this->lock);
	this->invocations.push(&ctx);
	uv_mutex_unlock(&this->lock);

	uv_async_send(&this->async);
	uv_sem_wait(&ctx.completed);

	uv_sem_destroy(&ctx.completed);
	
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

	do
	{
		uv_mutex_lock(&thiz->lock);
		auto ctx = thiz->invocations.front();
		thiz->invocations.pop();
		uv_mutex_unlock(&thiz->lock);

		try
		{
			ctx->result = thiz->InvokeImpl(ctx->args);
		}
		catch (System::Exception^ ex)
		{
			ctx->exception = ex;
		}
		uv_sem_post(&ctx->completed);

		uv_mutex_lock(&thiz->lock);
		auto rest = thiz->invocations.size();
		uv_mutex_unlock(&thiz->lock);
		if (0 < rest)
		{
			// continue;
			uv_async_send(&thiz->async);
			break;
		}
		else if (rest == 0 && thiz->terminate)
		{
			delete thiz;
		}
	} while (false);
}
