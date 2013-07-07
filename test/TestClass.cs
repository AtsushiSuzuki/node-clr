using System;
using System.Threading;

namespace NodeCLRTest
{
	public class TestClass1
	{
		public class TestClass2
		{
		}
		
		public static event EventHandler Event1;
		
		public static Int32 Field1;
		
		public static readonly Int32 Field2;
		
		public static Int32 Property1 { get; set; }
		
		public static Int32 Property2 { get { return 42; } }
		
		public static Int32 Property3 { set { } }
		
		public static Int32 Method1(Int32 x, Int32 y)
		{
			return x + y;
		}
		
		
		TestClass1()
		{
		}
		
		public event EventHandler Event2;
		
		public Int32 Field3;
		
		public readonly Int32 Field4;
		
		public Int32 Property4 { get; set; }
		
		public Int32 Property5 { get { return 60; } }
		
		public Int32 Property6 { set { } }
		
		public Int32 Method2(Int32 x, Int32 y)
		{
			return x - y;
		}
		
		public static void Run(ThreadStart action)
		{
			action();
		}
		
		public static void RunAsync(ThreadStart action)
		{
			var t = new System.Threading.Thread(action);
			t.Start();
		}
	}
}
