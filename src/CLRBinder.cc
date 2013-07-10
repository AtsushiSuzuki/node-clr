#include "node-clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System::Linq;
using namespace System::Reflection;


static array<System::Object^>^ BindToMethod(MethodBase^ method, Handle<Array> args);
static array<System::Object^>^ BindToMethod(MethodBase^ method, Handle<Array> args, int% score);
static MethodBase^ FindMostSpecificMethod(array<MethodBase^>^ methods, Handle<Array> args);
static int CompareMethods(MethodBase^ lhs, MethodBase^ rhs);
static int CompareTypes(System::Type^ lhs, System::Type^ rhs);
static System::Object^ ChangeType(Handle<Value> value, System::Type^ type);
static System::Object^ ChangeType(Handle<Value> value, System::Type^ type, int% score);
static System::Object^ ChangeType(System::Object^ value, System::Type^ type, int% score);


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

		auto ctor = (ConstructorInfo^)SelectMethod(
			Enumerable::ToArray(Enumerable::Cast<MethodBase^>(ctors)),
			args);
		return ctor->Invoke(
			BindingFlags::OptionalParamBinding,
			nullptr,
			BindToMethod(ctor, args),
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

	auto method = (MethodInfo^)SelectMethod(
		match->ToArray(),
		args);

	auto result = method->Invoke(
		target,
		BindingFlags::OptionalParamBinding,
		nullptr,
		BindToMethod(method, args),
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
		ChangeType(value, fi->FieldType));
}

enum {
	INCOMPATIBLE = 0,
	EXPLICIT_CONVERSION = 1,
	IMPLICIT_CONVERSION = 2,
	EXACT = 3,
};

MethodBase^ CLRBinder::SelectMethod(
	array<MethodBase^>^ methods,
	Handle<Array> args)
{
	if (methods->Length == 0)
	{
		throw gcnew System::MissingMethodException();
	}

	auto scores = gcnew array<int>(methods->Length);
	for (int i = 0; i < methods->Length; i++)
	{
		BindToMethod(methods[i], args, scores[i]);
	}

	int max = Enumerable::Max(scores);
	if (max < IMPLICIT_CONVERSION)
	{
		throw gcnew System::MissingMethodException();
	}

	auto canditates = gcnew List<MethodBase^>();
	for (int i = 0; i < methods->Length; i++)
	{
		if (scores[i] == max)
		{
			canditates->Add(methods[i]);
		}
	}

	return FindMostSpecificMethod(canditates->ToArray(), args);
}

array<System::Object^>^ BindToMethod(
	MethodBase^ method,
	Handle<Array> args)
{
	int score;
	auto result = BindToMethod(
		method,
		args,
		score);
	if (INCOMPATIBLE < score)
	{
		return result;
	}
	else
	{
		throw gcnew System::MissingMethodException();
	}
}

array<System::Object^>^ BindToMethod(
	MethodBase^ method,
	Handle<Array> args,
	int% score)
{
	auto params = method->GetParameters();

	// check for ref or out parameter
	for each (ParameterInfo^ param in params)
	{
		if (param->ParameterType->IsByRef)
		{
			score = INCOMPATIBLE;
			return nullptr;
		}
	}
	
	// check for parameter count
	int paramsMin = 0, paramsMax = 0;
	bool isVarArgs = false;
	for each (ParameterInfo^ param in params)
	{
		if (0 < param->GetCustomAttributes(System::ParamArrayAttribute::typeid, false)->Length)
		{
			paramsMax = int::MaxValue;
			isVarArgs = true;
		}
		else if (param->IsOptional)
		{
			paramsMax++;
		}
		else
		{
			paramsMin++;
			paramsMax++;
		}
	}
	if ((int)args->Length() < paramsMin ||
		paramsMax < (int)args->Length())
	{
		score = INCOMPATIBLE;
		return nullptr;
	}

	// get varargs type
	System::Type^ varArgsType = nullptr;
	if (isVarArgs)
	{
		auto paramType = params[params->Length - 1]->ParameterType;
		if (paramType->IsArray && paramType->HasElementType)
		{
			varArgsType = paramType->GetElementType();
		}
		else if (paramType->IsGenericType)
		{
			varArgsType = paramType->GetGenericArguments()[0];
		}
		else
		{
			varArgsType = System::Object::typeid;
		}
	}

	// bind parameters
	score = EXACT;
	auto arguments = gcnew array<System::Object^>(System::Math::Min((int)args->Length(), params->Length));
	for (int i = 0; i < (int)args->Length(); i++)
	{
		if (isVarArgs &&
			i == params->Length - 1 &&
			i == (int)args->Length() - 1)
		{
			int score1;
			auto arg1 = ChangeType(args->Get(Number::New(i)), params[i]->ParameterType, score1);
			int score2;
			auto arg2 = ChangeType(args->Get(Number::New(i)), varArgsType, score2);

			if (score1 >= score2)
			{
				arguments[i] = arg1;
				score = System::Math::Min(score, score1);
			}
			else
			{
				auto arr = System::Array::CreateInstance(varArgsType, 1);
				arr->SetValue(arg2, 0);
				arguments[i] = arr;
				score = System::Math::Min(score, score2);
			}
		}
		else if (i < params->Length)
		{
			int s;
			arguments[i] = ChangeType(args->Get(Number::New(i)), params[i]->ParameterType, s);

			score = System::Math::Min(score, s);
		}
		else
		{
			int s;
			auto arg =  ChangeType(args->Get(Number::New(i)), varArgsType, s);

			System::Array^ arr;
			if (arguments[arguments->Length - 1] == nullptr)
			{
				arr = System::Array::CreateInstance(varArgsType, args->Length() - params->Length + 1);
				arguments[arguments->Length - 1] = arr;
			}
			else
			{
				arr = (System::Array^)arguments[arguments->Length - 1];
			}

			arr->SetValue(arg, i - params->Length + 1);
			score = System::Math::Min(score, s);
		}
	}

	return arguments;
}

MethodBase^ FindMostSpecificMethod(
	array<MethodBase^>^ methods,
	Handle<Array> args)
{
	auto current = methods[0];
	for (int i = 1; i < methods->Length; i++)
	{
		if (0 < CompareMethods(current, methods[i]))
		{
			current = methods[i];
		}
	}
	return current;
}

int CompareMethods(MethodBase^ lhs, MethodBase^ rhs)
{
	auto params1 = lhs->GetParameters();
	auto params2 = rhs->GetParameters();
	
	auto count = System::Math::Min(params1->Length, params2->Length);
	for (int i = 0; i < count; i++)
	{
		int c = CompareTypes(params1[i]->ParameterType, params2[i]->ParameterType);
		if (c != 0)
		{
			return c;
		}
	}

	return 0;
}

int CompareTypes(System::Type^ lhs, System::Type^ rhs)
{
	if (lhs == rhs)
	{
		return 0;
	}
	else
	{
		if (lhs->IsAssignableFrom(rhs))
		{
			return 1;
		}
		else if (rhs->IsAssignableFrom(lhs))
		{
			return -1;
		}

		// compare primitive types
		if (lhs->IsPrimitive && rhs->IsPrimitive)
		{
			return (int)System::Type::GetTypeCode(rhs) - (int)System::Type::GetTypeCode(lhs);
		}

		// compare array types
		if (lhs->IsArray && rhs->IsArray)
		{
			return CompareTypes(lhs->GetElementType(), rhs->GetElementType());
		}
		if (lhs->IsGenericType && rhs->IsGenericType &&
			lhs->GetGenericTypeDefinition() == rhs->GetGenericTypeDefinition())
		{
			auto typeParams1 = lhs->GetGenericArguments();
			auto typeParams2 = rhs->GetGenericArguments();

			for (int i = 0; i < typeParams1->Length; i++)
			{
				int c = CompareTypes(typeParams1[i], typeParams2[i]);
				if (c != 0)
				{
					return c;
				}
			}
		}

		// compare delegate types
		if (System::Delegate::typeid->IsAssignableFrom(lhs) &&
			System::Delegate::typeid->IsAssignableFrom(rhs))
		{
			auto params1 = lhs->GetMethod("Invoke")->GetParameters();
			auto params2 = rhs->GetMethod("Invoke")->GetParameters();

			int c = params2->Length - params1->Length;
			if (c != 0)
			{
				return c;
			}

			for (int i = 0; i < params1->Length; i++)
			{
				c = CompareTypes(params1[i]->ParameterType, params2[i]->ParameterType);
				if (c != 0)
				{
					return c;
				}
			}
		}

		return 0;
	}
}

System::Object^ ChangeType(
	Handle<Value> value,
	System::Type^ type)
{
	int score;
	auto result = ChangeType(
		value,
		type,
		score);

	if (INCOMPATIBLE < score)
	{
		return result;
	}
	else
	{
		throw gcnew System::InvalidCastException();
	}
}

System::Object^ ChangeType(
	Handle<Value> value,
	System::Type^ type,
	int% score)
{
	// unwrap value
	if (CLRObject::IsWrapped(value))
	{
		return ChangeType(CLRObject::Unwrap(value), type, score);
	}

	// null
	if (value->IsNull() ||
		value->IsUndefined())
	{
		if (!type->IsValueType || (
			type->IsGenericType &&
			type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition()))
		{
			score = EXACT;
			return nullptr;
		}
		else
		{
			score = INCOMPATIBLE;
			return System::Activator::CreateInstance(type);
		}
	}

	// unwrap nullable
	if (type->IsGenericType &&
		type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
	{
		type = type->GetGenericArguments()[0];
	}

	// primitives
	// TODO: overflow
	// TODO: changeType
	if (value->IsBoolean() || value->IsBooleanObject())
	{
		if (type == System::Boolean::typeid)
		{
			score = EXACT;
			return value->BooleanValue();
		}
	}
	else if (value->IsInt32())
	{
		if (type == System::Int32::typeid ||
			type == System::Int64::typeid ||
			type == System::Single::typeid ||
			type == System::Double::typeid ||
			type == System::Decimal::typeid)
		{
			score = EXACT;
			return value->Int32Value();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::UInt32::typeid ||
			type == System::UInt64::typeid ||
			type == System::Char::typeid)
		{

			score = IMPLICIT_CONVERSION;
			return value->Int32Value();
		}
	}
	else if (value->IsUint32())
	{
		if (type == System::UInt32::typeid ||
			type == System::Int64::typeid ||
			type == System::UInt64::typeid ||
			type == System::Single::typeid ||
			type == System::Double::typeid ||
			type == System::Decimal::typeid)
		{
			score = EXACT;
			return value->Uint32Value();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::Int32::typeid ||
			type == System::Char::typeid)
		{
			score = IMPLICIT_CONVERSION;
			return value->Uint32Value();
		}
	}
	else if (value->IsNumber() || value->IsNumberObject())
	{
		if (type == System::Double::typeid)
		{
			score = EXACT;
			return value->NumberValue();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::Int32::typeid ||
			type == System::UInt32::typeid ||
			type == System::Int64::typeid ||
			type == System::UInt64::typeid ||
			type == System::Char::typeid ||
			type == System::Single::typeid ||
			type == System::Decimal::typeid)
		{
			score = IMPLICIT_CONVERSION;
			return value->NumberValue();
		}
	}
	else if (value->IsString() || value->IsStringObject())
	{
		if (type == System::String::typeid)
		{
			score = EXACT;
			return ToCLRString(value);
		}
	}
	else if (value->IsFunction())
	{
		auto func = gcnew V8Function(Handle<Function>::Cast(value));
		if (type == System::Object::typeid ||
			type == System::Delegate::typeid ||
			type == System::MulticastDelegate::typeid)
		{
			score = EXACT;
			return func->CreateDelegate();
		}
		else if (System::Delegate::typeid->IsAssignableFrom(type))
		{
			score = EXACT;
			return func->CreateDelegate(type);
		}
	}
	else if (value->IsArray())
	{
		// TODO: handle cyclic reference
		auto elementType = System::Object::typeid;
		if (type->IsArray && type->HasElementType)
		{
			elementType = type->GetElementType();
		}
		else if (type->IsGenericType)
		{
			auto d = type->GetGenericTypeDefinition();
			if (d == IEnumerable<int>::typeid->GetGenericTypeDefinition() ||
				d == ICollection<int>::typeid->GetGenericTypeDefinition() ||
				d == IList<int>::typeid->GetGenericTypeDefinition() ||
				d == IReadOnlyCollection<int>::typeid->GetGenericTypeDefinition() ||
				d == IReadOnlyList<int>::typeid->GetGenericTypeDefinition())
			{
				elementType = type->GetGenericArguments()[0];
			}
		}

		auto from = Handle<Array>::Cast(value);
		auto to = System::Array::CreateInstance(elementType, from->Length());
		for (int i = 0; i < (int)from->Length(); i++)
		{
			int s;
			to->SetValue(
				ChangeType(
					from->Get(i),
					elementType,
					s),
				i);
			score = System::Math::Min(s, score);
		}

		if (type->IsAssignableFrom(to->GetType()))
		{
			return to;
		}
	}
	else
	{
		// TODO: handle DataContractAttribute
		// TODO: handle cyclic reference

		auto from = Handle<Object>::Cast(value);
		auto to = (IDictionary<System::String^, System::Object^>^)(gcnew ExpandoObject());
		
		auto names = from->GetOwnPropertyNames();
		for (int i = 0; i < (int)names->Length(); i++)
		{
			int s;
			auto name = names->Get(i);
			to[ToCLRString(name)] = ChangeType(from->Get(name), System::Object::typeid, s);

			score = System::Math::Min(s, score);
		}

		if (type->IsAssignableFrom(ExpandoObject::typeid))
		{
			return to;
		}
	}

	if (type == System::Object::typeid)
	{
		score = EXACT;
		return ToCLRValue(value);
	}
	else
	{
		score = INCOMPATIBLE;
		return nullptr;
	}
}

System::Object^ ChangeType(
	System::Object^ value,
	System::Type^ type,
	int% score)
{
	if (value != nullptr &&
		type->IsAssignableFrom(value->GetType()))
	{
		score = EXACT;
		return value;
	}

	try
	{
		score = EXACT;
		return System::Convert::ChangeType(value, type);
	}
	catch (System::InvalidCastException^)
	{
		score = INCOMPATIBLE;
		return nullptr;
	}
}