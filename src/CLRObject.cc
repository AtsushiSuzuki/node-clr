#include "node-clr.h"

using namespace v8;


Local<Object> CLRObject::Wrap(System::Object^ wrapped)
{
	using namespace v8;
	HandleScope scope;

	auto tpl = ObjectTemplate::New();
	tpl->SetInternalFieldCount(1);
	auto wrapper = tpl->NewInstance();

	auto obj = new CLRObject(wrapped);
	obj->node::ObjectWrap::Wrap(wrapper);
	wrapper->Set(
		String::NewSymbol("__clr_type__"),
		V8String((wrapped != nullptr)
			? wrapped->GetType()->AssemblyQualifiedName
			: System::Object::typeid->AssemblyQualifiedName),
		(PropertyAttribute)(ReadOnly | DontEnum | DontDelete));

	return scope.Close(wrapper);
}

bool CLRObject::IsWrapped(Handle<Value> wrapper)
{
	using namespace v8;

	return wrapper->IsObject() &&
		Handle<Object>::Cast(wrapper)->Has(String::NewSymbol("__clr_type__"));
}

System::Object^ CLRObject::Unwrap(Handle<Value> wrapper)
{
	using namespace v8;

	assert(IsWrapped(wrapper));
	auto obj = node::ObjectWrap::Unwrap<CLRObject>(Handle<Object>::Cast(wrapper));
	return obj->wrapped;
}

Handle<Value> CLRObject::CreateConstructor(const Arguments& args)
{
	using namespace v8;
	HandleScope scope;

	if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsFunction())
	{
		ThrowException(Exception::TypeError(String::New("Argument error")));
		return scope.Close(Undefined());
	}
	auto type = System::Type::GetType(CLRString(args[0]));
	if (type == nullptr)
	{
		ThrowException(Exception::TypeError(String::New("No such CLR type")));
		return scope.Close(Undefined());
	}

	auto tpl = FunctionTemplate::New(New);
	tpl->SetClassName(V8String(type->Name));
	tpl->Set(String::NewSymbol("__clr_type__"), V8String(type->AssemblyQualifiedName), (PropertyAttribute)(ReadOnly | DontEnum | DontDelete));
	tpl->Set(String::NewSymbol("__initializer__"), args[1], (PropertyAttribute)(ReadOnly | DontEnum | DontDelete));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	return scope.Close(tpl->GetFunction());
}

Handle<Value> CLRObject::New(const Arguments& args)
{
	using namespace v8;
	HandleScope scope;

	auto fullName = args.Callee()->Get(String::NewSymbol("__clr_type__"));
	if (!fullName->IsString())
	{
		ThrowException(Exception::TypeError(String::New("Internal Error: Illegal invocation")));
		return scope.Close(Undefined());
	}
	auto initializer = args.Callee()->Get(String::NewSymbol("__initializer__"));
	if (!initializer->IsFunction())
	{
		ThrowException(Exception::TypeError(String::New("Internal Error: Illegal invocation")));
		return scope.Close(Undefined());
	}
		
	auto type = System::Type::GetType(CLRString(fullName));
	if (type == nullptr)
	{
		ThrowException(Exception::TypeError(String::New("Internal Error: No such CLR type")));
		return scope.Close(Undefined());
	}

	System::Object^ wrapped;
	try
	{
		wrapped = System::Activator::CreateInstance(type, CLRArguments(args));
	}
	catch (System::Exception^ ex)
	{
		ThrowException(V8Exception(ex));
		return scope.Close(Undefined());
	}

	auto obj = new CLRObject(wrapped);
	obj->node::ObjectWrap::Wrap(args.This());
	args.This()->Set(
		String::NewSymbol("__clr_type__"),
		V8String((wrapped != nullptr)
			? wrapped->GetType()->AssemblyQualifiedName
			: System::Object::typeid->AssemblyQualifiedName),
		(PropertyAttribute)(ReadOnly | DontEnum | DontDelete));

	std::vector<Handle<Value> > params;
	for (int i = 0; i < args.Length(); i++)
	{
		params.push_back(args[i]);
	}
	Local<Function>::Cast(initializer)->Call(args.This(), args.Length(), (0 < params.size()) ? &(params[0]) : nullptr);

	return scope.Close(Undefined());
}

CLRObject::CLRObject(System::Object^ wrapped)
	: wrapped(wrapped)
{
}

CLRObject::~CLRObject()
{
}