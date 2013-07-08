#ifndef NODE_CLR_H_
#define NODE_CLR_H_

#include <vector>
#include <vcclr.h>
#include <v8.h>
#include <node.h>

#using <mscorlib.dll>
#using <System.dll>
#using <System.Core.dll>

#include "Marshal.h"
#include "CLRObject.h"
#include "CLRBinder.h"
#include "V8Function.h"
#include "V8AsyncInvocation.h"

System::Type^ CLRGetType(System::String^ name);

#endif