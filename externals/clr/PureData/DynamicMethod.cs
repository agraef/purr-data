using System;
using System.Reflection;
using System.Reflection.Emit;

namespace PureData
{
    public delegate void DynamicMethodBang(External target);
    public delegate void DynamicMethodFloat(External target,float f);
    public delegate void DynamicMethodSymbol(External target,Symbol s);
    public delegate void DynamicMethodPointer(External target,Pointer p);
    public delegate void DynamicMethodList(External target,Atom[] l);
    public delegate void DynamicMethodAnything(External target,int i,Symbol s,Atom[] l);
    public delegate void DynamicMethodObject(External target,int i,object o);

    class DynamicMethods
    {
        private static Delegate CreateIntern(Delegate del, Type dyntype)
        {
            MethodInfo dynmethod = dyntype.GetMethod("Invoke");
            ParameterInfo[] dynparms = dynmethod.GetParameters();
            MethodInfo method = del.Method;

#if DEBUG
            if (dynmethod.ReturnType != typeof(void)) throw new ArgumentException("Return type must be void");

            ParameterInfo[] parms = method.GetParameters();
            int numparms = parms.Length;

            if (dynparms.Length != numparms + 1) throw new ArgumentException("Number of parameters don't match");

            for (int i = 0; i < numparms; ++i)
                if (dynparms[i + 1].ParameterType != parms[i].ParameterType) throw new ArgumentException("Parameter types don't match");
#endif

            Type[] argtypes = new Type[dynparms.Length];
            for (int i = 0; i < dynparms.Length; ++i)
                argtypes[i] = dynparms[i].ParameterType;

            // Create dynamic method and obtain its IL generator to inject code.
            DynamicMethod dynam = new DynamicMethod("dummy", typeof(void), argtypes, typeof(DynamicMethods));
            ILGenerator il = dynam.GetILGenerator();

            #region IL Code generation
            // If method isn't static push target instance on top of stack.
            if (!method.IsStatic)
                // Argument 0 of dynamic method is target instance.
                il.Emit(OpCodes.Ldarg_0);

            // Push parameters onto the stack
            for (int i = 0; i < dynparms.Length - 1; ++i)
                il.Emit(OpCodes.Ldarg, i + 1);

            // Perform actual call.
            // If method is not final a callvirt is required otherwise a normal call will be emitted.
            il.Emit(method.IsFinal ? OpCodes.Call : OpCodes.Callvirt, method);

            // Emit return opcode.
            il.Emit(OpCodes.Ret);
            #endregion

            return dynam.CreateDelegate(dyntype);
        }

        #region Specialized DynamicMethod makers
        public static DynamicMethodBang Create(Public.MethodBang method)
        {
            return (DynamicMethodBang)CreateIntern(method, typeof(DynamicMethodBang));
        }

        public static DynamicMethodFloat Create(Public.MethodFloat method)
        {
            return (DynamicMethodFloat)CreateIntern(method,typeof(DynamicMethodFloat));
        }

        public static DynamicMethodSymbol Create(Public.MethodSymbol method)
        {
            return (DynamicMethodSymbol)CreateIntern(method,typeof(DynamicMethodSymbol));
        }

        public static DynamicMethodPointer Create(Public.MethodPointer method)
        {
            return (DynamicMethodPointer)CreateIntern(method,typeof(DynamicMethodPointer));
        }

        public static DynamicMethodList Create(Public.MethodList method)
        {
            return (DynamicMethodList)CreateIntern(method,typeof(DynamicMethodList));
        }

        public static DynamicMethodAnything Create(Public.MethodAnything method)
        {
            return (DynamicMethodAnything)CreateIntern(method,typeof(DynamicMethodAnything));
        }

        public static DynamicMethodObject Create(Public.MethodObject method)
        {
            return (DynamicMethodObject)CreateIntern(method, typeof(DynamicMethodObject));
        }
        #endregion
    }
}
