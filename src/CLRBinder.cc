#include "node-clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Linq;
using namespace System::Reflection;


static System::Object^ CLRWildcardValue(Handle<Value> value);
static array<System::Object^>^ ToCLRWildcardArguments(Handle<Array> args);


System::Object^ CLRBinder::InvokeConstructor(
	Handle<Value> typeName,
	const Arguments& args)
{
	auto type = CLRGetType(ToCLRString(typeName));

	auto arr = Array::New();
	for (int i = 0; i < args.Length(); i++)
	{
		arr->Set(Number::New(i), args[i]);
	}

	return InvokeConstructor(
		type,
		arr);
}

System::Object^ CLRBinder::InvokeConstructor(
	System::Type^ type,
	Handle<Array> args)
{
	if (args->Length() == 0)
	{
		return System::Activator::CreateInstance(type);
	}
	else
	{
		auto ctors = type->GetConstructors();

		array<System::Object^>^ params;
		auto ctor = (ConstructorInfo^)SelectMethod(
			Enumerable::ToArray(Enumerable::Cast<MethodBase^>(ctors)),
			args,
			params);
		return ctor->Invoke(
			BindingFlags::OptionalParamBinding,
			nullptr,
			params,
			nullptr);
	}
}

Handle<Value> CLRBinder::InvokeMethod(
	Handle<Value> typeName,
	Handle<Value> name,
	Handle<Value> target,
	Handle<Value> args)
{
	auto type = CLRGetType(ToCLRString(typeName));

	return InvokeMethod(
		type,
		ToCLRString(name),
		(CLRObject::IsWrapped(target))
			? CLRObject::Unwrap(target)
			: nullptr,
		Handle<Array>::Cast(args));
}

Handle<Value> CLRBinder::InvokeMethod(
	System::Type^ type,
	System::String^ name,
	System::Object^ target,
	Handle<Array> args)
{
	auto methods = type->GetMethods(
		BindingFlags::Public |
		((target != nullptr) ? BindingFlags::Instance : BindingFlags::Static));
	
	auto match = gcnew List<MethodBase^>();
	for each (auto method in methods)
	{
		if (name == method->Name)
		{
			match->Add(method);
		}
	}

	array<System::Object^>^ params;
	auto method = (MethodInfo^)SelectMethod(
		match->ToArray(),
		args,
		params);

	auto result = method->Invoke(
		target,
		BindingFlags::OptionalParamBinding,
		nullptr,
		params,
		nullptr);
	if (result == nullptr &&
		method->ReturnType == System::Void::typeid)
	{
		return Undefined();
	}
	else
	{
		return ToV8Value(result);
	}
}

MethodBase^ CLRBinder::SelectMethod(
	array<MethodBase^>^ methods,
	Handle<Array> args,
	array<System::Object^>^% params)
{
	// HACK: DefaultBinderの実装を流用する代わりに、独自で機能を実装する
	if (methods->Length == 0)
	{
		throw gcnew System::MissingMethodException();
	}

	MethodBase^ method = nullptr;
	params = ToCLRWildcardArguments(args);
	System::Object^ state;
	try
	{
		method = System::Type::DefaultBinder->BindToMethod(
			BindingFlags::OptionalParamBinding,
			methods,
			params,
			nullptr,
			nullptr,
			nullptr,
			state);
	}
	catch (AmbiguousMatchException^)
	{
	}
	catch (System::MissingMethodException^)
	{
	}

	if (method != nullptr)
	{
		params = ToCLRArguments(args, method->GetParameters());
		return method;
	}

	params = ToCLRArguments(args, nullptr);
	return System::Type::DefaultBinder->BindToMethod(
		BindingFlags::OptionalParamBinding,
		methods,
		params,
		nullptr,
		nullptr,
		nullptr,
		state);
}

System::Object^ CLRWildcardValue(
	Handle<Value> value)
{
	if (value->IsNull() || value->IsUndefined())
	{
		return nullptr;
	}
	else if (value->IsBoolean())
	{
		return value->BooleanValue();
	}
	else if (value->IsInt32())
	{
		return value->Int32Value();
	}
	else if (value->IsUint32())
	{
		return value->Uint32Value();
	}
	else if (value->IsNumber())
	{
		return value->NumberValue();
	}
	else if (value->IsString())
	{
		return ToCLRString(value);
	}
	else if (value->IsFunction())
	{
		// return null
		return nullptr;
	}
	else if (value->IsArray())
	{
		// return null
		return nullptr;
	}
	else if (value->IsObject())
	{
		// return null
		return nullptr;
	}
	else if (value->IsExternal())
	{
		return (System::IntPtr)(External::Unwrap(value));
	}

	throw gcnew System::NotImplementedException();
}

array<System::Object^>^ ToCLRWildcardArguments(
	Handle<Array> args)
{
	auto arr = gcnew array<System::Object^>(args->Length());
	for (int i = 0; i < (int)args->Length(); i++)
	{
		arr[i] = CLRWildcardValue(args->Get(Number::New(i)));
	}
	return arr;
}

Handle<Value> CLRBinder::GetField(
	Handle<Value> typeName,
	Handle<Value> name,
	Handle<Value> target)
{
	auto type = CLRGetType(ToCLRString(typeName));

	return GetField(
		type,
		ToCLRString(name),
		(CLRObject::IsWrapped(target))
			? CLRObject::Unwrap(target)
			: nullptr);
}

Handle<Value> CLRBinder::GetField(
	System::Type^ type,
	System::String^ name,
	System::Object^ target)
{
	auto fi = type->GetField(name);
	auto result = fi->GetValue(target);
	return ToV8Value(result);
}

void CLRBinder::SetField(
	Handle<Value> typeName,
	Handle<Value> name,
	Handle<Value> target,
	Handle<Value> value)
{
	auto type = CLRGetType(ToCLRString(typeName));

	SetField(
		type,
		ToCLRString(name),
		(CLRObject::IsWrapped(target))
			? CLRObject::Unwrap(target)
			: nullptr,
		value);
}

void CLRBinder::SetField(
	System::Type^ type,
	System::String^ name,
	System::Object^ target,
	Handle<Value> value)
{
	auto fi = type->GetField(name);
	fi->SetValue(
		target,
		ToCLRValue(value, fi->FieldType));
}