#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;

Handle<Value> V8Binder::InvokeMember(
	Handle<String> typeName,
	Handle<String> name,
	BindingFlags attr,
	Handle<Value> target,
	Handle<Array> args)
{
	System::Type^ type;
	try
	{
		type = System::Type::GetType(CLRString(typeName));
	}
	catch (System::Exception^ ex)
	{
		ThrowException(V8Exception(ex));
		return Undefined();
	}

	return InvokeMember(
		type,
		CLRString(name),
		attr,
		(CLRObject::IsWrapped(target)) ? CLRObject::Unwrap(target) : nullptr,
		args);
}

Handle<Value> V8Binder::InvokeMember(
	System::Type^ type,
	System::String^ name,
	BindingFlags attr,
	System::Object^ target,
	Handle<Array> args)
{
	System::Object^ result;
	try
	{
		result = type->InvokeMember(
			name,
			((target != nullptr) ? BindingFlags::Instance : BindingFlags::Static) | BindingFlags::Public | attr | BindingFlags::OptionalParamBinding,
			nullptr,
			target,
			CLRArguments(args));
	}
	catch (System::Exception^ ex)
	{
		ThrowException(V8Exception(ex));
		return Undefined();
	}

	return V8Value(result);
}