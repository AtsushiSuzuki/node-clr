#include "clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Reflection;


gcroot<Dictionary<System::Type^, System::UIntPtr>^> CLRType::cache_ = gcnew Dictionary<System::Type^, System::UIntPtr>();

CLRType* CLRType::GetInstance(System::Type^ type)
{
	if (type == nullptr)
	{
		throw gcnew System::ArgumentNullException("type");
	}

	System::UIntPtr ptr;
	if (cache_->TryGetValue(type, ptr))
	{
		return static_cast<CLRType*>(static_cast<void*>(ptr));
	}

	auto wrap = new CLRType(type);
	cache_->Add(type, static_cast<System::UIntPtr>(static_cast<void*>(wrap)));

	return wrap;
}

CLRType::CLRType(System::Type^ type)
    : type_(type)
{
}

CLRType::~CLRType()
{
}

generic <typename T> where T : MemberInfo
IEnumerable<System::String^>^ sort(array<T>^ arr)
{
	auto set = gcnew SortedSet<System::String^>();
	for each (MemberInfo^ item in arr)
	{
		auto ci = dynamic_cast<ConstructorInfo^>(item);
		if (ci != nullptr && ci->IsSpecialName)
		{
			continue;
		}
		auto ei = dynamic_cast<EventInfo^>(item);
		if (ei != nullptr && ei->IsSpecialName)
		{
			continue;
		}
		auto fi = dynamic_cast<FieldInfo^>(item);
		if (fi != nullptr && fi->IsSpecialName)
		{
			continue;
		}
		auto mi = dynamic_cast<MethodInfo^>(item);
		if (mi != nullptr && mi->IsSpecialName)
		{
			continue;
		}
		auto pi = dynamic_cast<PropertyInfo^>(item);
		if (pi != nullptr && pi->IsSpecialName)
		{
			continue;
		}
		auto ti = dynamic_cast<System::Type^>(item);
		if (ti != nullptr && ti->IsSpecialName)
		{
			continue;
		}
		set->Add(item->Name);
	}
	return set;
}


Local<ObjectTemplate> CLRType::Template(System::Type^ type)
{
	auto tmpl = Nan::New<ObjectTemplate>();
	tmpl->SetInternalFieldCount(1);
	Nan::SetCallAsFunctionHandler(tmpl, ConstructCallback);

	if (type->IsGenericTypeDefinition)
	{
		auto fn = Nan::New<FunctionTemplate>(MakeGenericCallback);
		Nan::SetTemplate(tmpl, Nan::New<String>("of").ToLocalChecked(), fn, PropertyAttribute());
	}

	auto flags = BindingFlags::Static | BindingFlags::Public | BindingFlags::FlattenHierarchy;
	for each (auto name in sort(type->GetNestedTypes(flags))) {
		Nan::SetAccessor(tmpl, ToV8String(name), GetNestedTypeCallback);
	}
	for each (auto name in sort(type->GetEvents(flags)))
	{
		// TODO: not implemented.
	}
	for each (auto name in sort(type->GetFields(flags)))
	{
		Nan::SetAccessor(tmpl, ToV8String(name), GetStaticFieldCallback, SetStaticFieldCallback);
	}
	for each (auto name in sort(type->GetProperties(flags)))
	{
		Nan::SetAccessor(tmpl, ToV8String(name), GetStaticPropertyCallback, SetStaticPropertyCallback);
	}
	for each (auto name in sort(type->GetMethods(flags)))
	{
		auto fn = Nan::New<FunctionTemplate>(InvokeStaticMethodCallback, ToV8String(name));
		Nan::SetTemplate(tmpl, ToV8String(name), fn, ReadOnly);
	}

	Nan::SetTemplate(tmpl, Nan::New<String>("name").ToLocalChecked(), ToV8String(type->Name), PropertyAttribute(ReadOnly | DontEnum));
	Nan::SetTemplate(tmpl, Nan::New<String>("prototype").ToLocalChecked(), PrototypeTemplate(type), PropertyAttribute(DontEnum));

	return tmpl;
}

Local<ObjectTemplate> CLRType::PrototypeTemplate(System::Type^ type)
{
	auto tmpl = Nan::New<ObjectTemplate>();

	auto flags = BindingFlags::Instance | BindingFlags::Public | BindingFlags::FlattenHierarchy;
	for each (auto name in sort(type->GetEvents(flags)))
	{
		// TODO: not implemented.
	}
	for each (auto name in sort(type->GetFields(flags)))
	{
		Nan::SetAccessor(tmpl, ToV8String(name), GetFieldCallback, SetFieldCallback);
	}
	for each (auto name in sort(type->GetProperties(flags)))
	{
		Nan::SetAccessor(tmpl, ToV8String(name), GetPropertyCallback, SetPropertyCallback);
	}
	for each (auto name in sort(type->GetMethods(flags)))
	{
		auto fn = Nan::New<FunctionTemplate>(InvokeMethodCallback, ToV8String(name));
		Nan::SetTemplate(tmpl, ToV8String(name), fn, ReadOnly);
	}

	return tmpl;
}

Local<Object> CLRType::ToV8Value()
{
	if (constructor_.IsEmpty())
	{
		auto tmpl = Template(type_);
		auto ctor = Nan::NewInstance(tmpl).ToLocalChecked();
		Nan::SetPrivate(ctor, Nan::New<String>("clr").ToLocalChecked(), Nan::New<String>("type").ToLocalChecked());
		Wrap(ctor);

		constructor_.Reset(ctor);
		return ctor;
	}

	return Nan::New<Object>(constructor_);
}

bool CLRType::IsCLRType(Local<Value> value)
{
	if (value.IsEmpty() || !value->IsObject())
	{
		return false;
	}
	auto obj = Nan::To<Object>(value).ToLocalChecked();
	auto kind = Nan::GetPrivate(obj, Nan::New<String>("clr").ToLocalChecked());
	return !kind.IsEmpty() && kind.ToLocalChecked()->Equals(Nan::New<String>("type").ToLocalChecked());
}

CLRType* CLRType::Unwrap(Local<Value> value)
{
	return Nan::ObjectWrap::Unwrap<CLRType>(Nan::To<Object>(value).ToLocalChecked());
}

void CLRType::ConstructCallback(
	const Nan::FunctionCallbackInfo<Value>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;
	if (!info.IsConstructCall())
	{
		return Nan::ThrowTypeError("Illegal invocation");
	}

	System::Object^ obj;
	try
	{
		obj = ::Binder::Construct(type, info);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::MakeGenericCallback(
	const Nan::FunctionCallbackInfo<Value>& info)
{
	if (!CLRType::IsCLRType(info.Holder()))
	{
		return Nan::ThrowTypeError("Illegal invocation: receiver is not CLR type.");
	}
	auto type = CLRType::Unwrap(info.Holder())->type_;

	auto args = gcnew array<System::Type^>(info.Length());
	for (int i = 0; i < info.Length(); i++)
	{
		if (!CLRType::IsCLRType(info[i]))
		{
			return Nan::ThrowTypeError("arguments must be CLR type.");
		}
		args[i] = CLRType::Unwrap(info[i])->type_;
	}

	System::Type^ newType;
	try
	{
		newType = type->MakeGenericType(args);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	info.GetReturnValue().Set(CLRType::GetInstance(newType)->ToV8Value());
}

void CLRType::GetNestedTypeCallback(
	Local<String> property,
	const Nan::PropertyCallbackInfo<Value>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;

	auto nestedType = ::Binder::GetNestedType(type, ToCLRString(property));

	return info.GetReturnValue().Set(CLRType::GetInstance(nestedType)->ToV8Value());
}

void CLRType::GetStaticFieldCallback(
	Local<String> property,
	const Nan::PropertyCallbackInfo<Value>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;

	System::Object^ obj;
	try
	{
		obj = ::Binder::GetField(type, nullptr, ToCLRString(property));
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::SetStaticFieldCallback(
	Local<String> property,
	Local<Value> value,
	const Nan::PropertyCallbackInfo<void>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;

	try
	{
		::Binder::SetField(type, nullptr, ToCLRString(property), value);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}
}

void CLRType::GetStaticPropertyCallback(
	Local<String> property,
	const Nan::PropertyCallbackInfo<Value>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;

	System::Object^ obj;
	try
	{
		obj = ::Binder::GetProperty(type, nullptr, ToCLRString(property));
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::SetStaticPropertyCallback(
	Local<String> property,
	Local<Value> value,
	const Nan::PropertyCallbackInfo<void>& info)
{
	auto type = CLRType::Unwrap(info.Holder())->type_;

	try
	{
		::Binder::SetProperty(type, nullptr, ToCLRString(property), value);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}
}

void CLRType::InvokeStaticMethodCallback(
	const Nan::FunctionCallbackInfo<Value>& info)
{
	if (!CLRType::IsCLRType(info.Holder()))
	{
		return Nan::ThrowTypeError("Illegal invocation: receiver is not CLR type.");
	}
	auto type = CLRType::Unwrap(info.Holder())->type_;
	auto name = Nan::To<String>(info.Data()).ToLocalChecked();

	System::Object^ obj;
	try
	{
		obj = ::Binder::InvokeMethod(type, nullptr, ToCLRString(name), info);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::GetFieldCallback(
	Local<String> property,
	const Nan::PropertyCallbackInfo<Value>& info)
{
	auto target = CLRObject::Unwrap(info.Holder())->object_;
	auto type = target->GetType();

	System::Object^ obj;
	try
	{
		obj = ::Binder::GetField(type, target, ToCLRString(property));
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::SetFieldCallback(
	Local<String> property,
	Local<Value> value,
	const Nan::PropertyCallbackInfo<void>& info)
{
	auto target = CLRObject::Unwrap(info.Holder())->object_;
	auto type = target->GetType();

	try
	{
		::Binder::SetField(type, target, ToCLRString(property), value);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}
}

void CLRType::GetPropertyCallback(
	Local<String> property,
	const Nan::PropertyCallbackInfo<Value>& info)
{
	auto target = CLRObject::Unwrap(info.Holder())->object_;
	auto type = target->GetType();

	System::Object^ obj;
	try
	{
		obj = ::Binder::GetProperty(type, target, ToCLRString(property));
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}

void CLRType::SetPropertyCallback(
	Local<String> property,
	Local<Value> value,
	const Nan::PropertyCallbackInfo<void>& info)
{
	auto target = CLRObject::Unwrap(info.Holder())->object_;
	auto type = target->GetType();

	try
	{
		::Binder::SetProperty(type, target, ToCLRString(property), value);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}
}

void CLRType::InvokeMethodCallback(
	const Nan::FunctionCallbackInfo<Value>& info)
{
	if (!CLRObject::IsCLRObject(info.Holder()))
	{
		return Nan::ThrowTypeError("Illegal invocation: receiver is not CLR object.");
	}
	auto target = CLRObject::Unwrap(info.Holder())->object_;
	auto type = target->GetType();
	auto name = Nan::To<String>(info.Data()).ToLocalChecked();

	System::Object^ obj;
	try
	{
		obj = ::Binder::InvokeMethod(type, target, ToCLRString(name), info);
	}
	catch (System::Exception^ ex)
	{
		return Nan::ThrowError(ToV8Error(ex));
	}

	return info.GetReturnValue().Set(::ToV8Value(obj));
}
