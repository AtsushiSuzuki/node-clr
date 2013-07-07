#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;

V8Function::V8Function(Handle<Function> function)
	: threadId(uv_thread_self()), pFunc(new Persistent<Function>(Persistent<Function>::New(function)))
{
}

V8Function::~V8Function()
{
	delete pFunc;
}

System::Object^ V8Function::Invoke(array<System::Object^>^ args)
{
	if (this->threadId == uv_thread_self())
	{
		auto params = ToV8Arguments(args);
		auto result = (*this->pFunc)->Call(Local<v8::Object>(), (int)params.size(), &(params[0]));

		return ToCLRValue(result);
	}
	else
	{
		V8AsyncInvocation async(this, args);

		return async.InvokeAsync();
	}
}

System::Delegate^ V8Function::CreateDelegate()
{
	return gcnew System::Func<array<System::Object^>^, System::Object^>(this, &V8Function::Invoke);
}

System::Delegate^ V8Function::CreateDelegate(System::Type^ type)
{
	auto mi = type->GetMethod("Invoke");
	auto params = mi->GetParameters();
	
	if (mi->ReturnType == System::Void::typeid &&
		params->Length == 0)
	{
		auto action = V8Function::typeid->GetMethod(
			"Action0",
			BindingFlags::NonPublic | BindingFlags::Instance);
		return action->CreateDelegate(type, this);
	}
	else if (mi->ReturnType == System::Void::typeid)
	{
		auto types = gcnew array<System::Type^>(params->Length);
		for (int i = 0; i < params->Length; i++)
		{
			types[i] = params[i]->ParameterType;
		}

		auto action = V8Function::typeid->GetMethod(
			"Action" + params->Length.ToString(),
			BindingFlags::NonPublic | BindingFlags::Instance);
		return action->MakeGenericMethod(types)->CreateDelegate(type, this);
	}
	else
	{
		auto types = gcnew array<System::Type^>(params->Length + 1);
		for (int i = 0; i < params->Length; i++)
		{
			types[i] = params[i]->ParameterType;
		}
		types[params->Length] = mi->ReturnType;

		auto func = V8Function::typeid->GetMethod("Func" + params->Length.ToString());
		return func->MakeGenericMethod(types)->CreateDelegate(type, this);
	}
}

void V8Function::Action0()
{
	this->Invoke(gcnew array<System::Object^> { });
}

generic<typename T1>
void V8Function::Action1(T1 arg1)
{
	this->Invoke(gcnew array<System::Object^> { arg1 });
}

generic<typename T1, typename T2>
void V8Function::Action2(T1 arg1, T2 arg2)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2 });
}

generic<typename T1, typename T2, typename T3>
void V8Function::Action3(T1 arg1, T2 arg2, T3 arg3)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3 });
}

generic<typename T1, typename T2, typename T3, typename T4>
void V8Function::Action4(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5>
void V8Function::Action5(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void V8Function::Action6(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void V8Function::Action7(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
void V8Function::Action8(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
void V8Function::Action9(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
void V8Function::Action10(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
void V8Function::Action11(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
void V8Function::Action12(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
void V8Function::Action13(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14>
void V8Function::Action14(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15>
void V8Function::Action15(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16>
void V8Function::Action16(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16 });
}


generic<typename TResult>
TResult V8Function::Func0()
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { });
}

generic<typename T1, typename TResult>
TResult V8Function::Func1(T1 arg1)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1 });
}

generic<typename T1, typename T2, typename TResult>
TResult V8Function::Func2(T1 arg1, T2 arg2)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2 });
}

generic<typename T1, typename T2, typename T3, typename TResult>
TResult V8Function::Func3(T1 arg1, T2 arg2, T3 arg3)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename TResult>
TResult V8Function::Func4(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename TResult>
TResult V8Function::Func5(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename TResult>
TResult V8Function::Func6(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename TResult>
TResult V8Function::Func7(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename TResult>
TResult V8Function::Func8(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename TResult>
TResult V8Function::Func9(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename TResult>
TResult V8Function::Func10(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename TResult>
TResult V8Function::Func11(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename TResult>
TResult V8Function::Func12(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename TResult>
TResult V8Function::Func13(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename TResult>
TResult V8Function::Func14(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename TResult>
TResult V8Function::Func15(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename TResult>
TResult V8Function::Func16(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16 });
}
