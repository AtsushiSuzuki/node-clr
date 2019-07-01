#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;


V8Function* V8Function::New(Local<Function> func)
{
	return new V8Function(func);
}

V8Function::V8Function(Local<Function> func)
	: threadId(GetCurrentThreadId()), terminate(false)
{
	function.Reset(func);

	uv_async_init(uv_default_loop(), &this->async, &V8Function::AsyncCallback);
	uv_unref((uv_handle_t*)&this->async);
	this->async.data = this;
	uv_mutex_init(&this->lock);
}

System::Object^ V8Function::Invoke(array<System::Object^>^ args)
{
	if (this->threadId == GetCurrentThreadId())
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
	uv_mutex_lock(&this->lock);
	this->terminate = true;
	uv_mutex_unlock(&this->lock);

	uv_async_send(&this->async);
}

V8Function::~V8Function()
{
	uv_close((uv_handle_t*)(&this->async), nullptr);
	uv_mutex_destroy(&this->lock);
}

System::Object^ V8Function::InvokeImpl(array<System::Object^>^ args)
{
	Nan::HandleScope scope;

	std::vector<Local<Value> > params;
	for each (System::Object^ arg in args)
	{
		params.push_back(ToV8Value(arg));
	}

	Nan::TryCatch trycatch;
	auto result = Nan::MakeCallback(
		Nan::GetCurrentContext()->Global(),
		Nan::New(this->function),
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

NAUV_WORK_CB(V8Function::AsyncCallback)
{
	auto thiz = (V8Function*)async->data;

	InvocationContext* ctx = nullptr;
	uv_mutex_lock(&thiz->lock);
	if (0 < thiz->invocations.size())
	{
		ctx = thiz->invocations.front();
		thiz->invocations.pop();
	}
	uv_mutex_unlock(&thiz->lock);

	if (ctx != nullptr)
	{
		try
		{
			ctx->result = thiz->InvokeImpl(ctx->args);
		}
		catch (System::Exception^ ex)
		{
			ctx->exception = ex;
		}
		uv_sem_post(&ctx->completed);
	}

	uv_mutex_lock(&thiz->lock);
	auto rest = thiz->invocations.size();
	auto terminate = thiz->terminate;
	uv_mutex_unlock(&thiz->lock);

	if (0 < rest)
	{
		uv_async_send(&thiz->async);
	}
	else if (rest == 0 && terminate)
	{
		delete thiz;
	}
}
