#include "node-clr.h"

using namespace v8;
using namespace System::Reflection;


V8Delegate::V8Delegate(Local<Function> function)
	: func_(V8Function::New(function))
{
}

V8Delegate::~V8Delegate()
{
	this->func_->Destroy();
}

System::Object^ V8Delegate::Invoke(array<System::Object^>^ args)
{
	return this->func_->Invoke(args);
}

System::Delegate^ V8Delegate::CreateDelegate(Local<Function> func)
{
	auto thiz = gcnew V8Delegate(func);
	return gcnew System::Func<array<System::Object^>^, System::Object^>(thiz, &V8Delegate::Invoke);
}

System::Delegate^ V8Delegate::CreateDelegate(Local<Function> func, System::Type^ type)
{
	auto thiz = gcnew V8Delegate(func);

	auto mi = type->GetMethod("Invoke");
	auto params = mi->GetParameters();
	
	if (mi->ReturnType == System::Void::typeid &&
		params->Length == 0)
	{
		auto method = V8Delegate::typeid->GetMethod(
			"Action0",
			BindingFlags::NonPublic | BindingFlags::Instance);
		return method->CreateDelegate(type, thiz);
	}
	else if (mi->ReturnType == System::Void::typeid)
	{
		auto types = gcnew array<System::Type^>(params->Length);
		for (int i = 0; i < params->Length; i++)
		{
			types[i] = params[i]->ParameterType;
		}

		auto method = V8Delegate::typeid->GetMethod(
			"Action" + params->Length.ToString(),
			BindingFlags::NonPublic | BindingFlags::Instance);
		return method->MakeGenericMethod(types)->CreateDelegate(type, thiz);
	}
	else
	{
		auto types = gcnew array<System::Type^>(params->Length + 1);
		for (int i = 0; i < params->Length; i++)
		{
			types[i] = params[i]->ParameterType;
		}
		types[params->Length] = mi->ReturnType;

		auto method = V8Delegate::typeid->GetMethod(
			"Func" + params->Length.ToString(),
			BindingFlags::NonPublic | BindingFlags::Instance);
		return method->MakeGenericMethod(types)->CreateDelegate(type, thiz);
	}
}

void V8Delegate::Action0()
{
	this->Invoke(gcnew array<System::Object^> { });
}

generic<typename T1>
void V8Delegate::Action1(T1 arg1)
{
	this->Invoke(gcnew array<System::Object^> { arg1 });
}

generic<typename T1, typename T2>
void V8Delegate::Action2(T1 arg1, T2 arg2)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2 });
}

generic<typename T1, typename T2, typename T3>
void V8Delegate::Action3(T1 arg1, T2 arg2, T3 arg3)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3 });
}

generic<typename T1, typename T2, typename T3, typename T4>
void V8Delegate::Action4(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5>
void V8Delegate::Action5(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void V8Delegate::Action6(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void V8Delegate::Action7(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
void V8Delegate::Action8(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
void V8Delegate::Action9(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
void V8Delegate::Action10(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
void V8Delegate::Action11(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
void V8Delegate::Action12(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
void V8Delegate::Action13(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14>
void V8Delegate::Action14(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15>
void V8Delegate::Action15(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16>
void V8Delegate::Action16(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16 });
}


generic<typename TResult>
TResult V8Delegate::Func0()
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { });
}

generic<typename T1, typename TResult>
TResult V8Delegate::Func1(T1 arg1)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1 });
}

generic<typename T1, typename T2, typename TResult>
TResult V8Delegate::Func2(T1 arg1, T2 arg2)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2 });
}

generic<typename T1, typename T2, typename T3, typename TResult>
TResult V8Delegate::Func3(T1 arg1, T2 arg2, T3 arg3)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename TResult>
TResult V8Delegate::Func4(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename TResult>
TResult V8Delegate::Func5(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename TResult>
TResult V8Delegate::Func6(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename TResult>
TResult V8Delegate::Func7(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename TResult>
TResult V8Delegate::Func8(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename TResult>
TResult V8Delegate::Func9(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename TResult>
TResult V8Delegate::Func10(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename TResult>
TResult V8Delegate::Func11(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename TResult>
TResult V8Delegate::Func12(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename TResult>
TResult V8Delegate::Func13(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename TResult>
TResult V8Delegate::Func14(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename TResult>
TResult V8Delegate::Func15(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15 });
}

generic<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename TResult>
TResult V8Delegate::Func16(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	return (TResult)this->Invoke(gcnew array<System::Object^> { arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16 });
}
