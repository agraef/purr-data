using System;
using System.Runtime.CompilerServices; // for extern import
using System.Runtime.InteropServices; // for structures
using System.Reflection;
using System.Collections;

namespace PureData
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct ClassPtr
    {
        private IntPtr ptr;

        public void Clear() { ptr = IntPtr.Zero; }
        public bool Valid() { return ptr != IntPtr.Zero; }

        public override string ToString() { return ptr.ToString(); }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ExternalPtr
    {
        private IntPtr ptr;

        public void Clear() { ptr = IntPtr.Zero; }
        public bool Valid() { return ptr != IntPtr.Zero; }

        public override string ToString() { return ptr.ToString(); }
    }

    // data passed from/to native side
    [StructLayout(LayoutKind.Sequential)]
    internal class PassedData
    {
        public Class klass;
        public External ext;
        public object obj;
    }

    // PD core functions
    public class Internal 
    {
        // the ExternalPtr when initializing an External instance
        internal static ExternalPtr extptr;

        internal static PassedData pass;

        // --------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Symbol SymGen(string sym);        

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static string SymEval(Symbol sym);        

        // --------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj, Symbol sel, Symbol to_sel);

        // map to data member
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj, ref float f); 

        // map to data member
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj, ref Symbol s);

        // map to data member
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj,ref Pointer f);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj, Symbol type); // create proxy inlet (typed)

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddInlet(ExternalPtr obj); // create proxy inlet (anything)

        // --------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void AddOutlet(ExternalPtr obj, Symbol type);

        // --------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n, float f);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n, Symbol s);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n, Pointer p);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n, Atom a);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Outlet(ExternalPtr obj, int n, Symbol s, Atom[] l);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void OutletEx(ExternalPtr obj, int n, object o);

        // --------------------------------------------------------------------------

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Bind(ExternalPtr obj, Symbol dst);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        internal extern static void Unbind(ExternalPtr obj, Symbol dst);
    }


    public abstract class Public
        : Internal
    {
        // to be returned by Setup function
        protected enum ClassType { Default = 0, PD = 1, GObj = 2, Patchable = 3, NoInlet = 8 }

        // --------------------------------------------------------------------------

        public delegate void MethodBang();
        public delegate void MethodFloat(float f);
        public delegate void MethodSymbol(Symbol s);
        public delegate void MethodPointer(Pointer p);
        public delegate void MethodList(Atom[] lst);
        public delegate void MethodAnything(int inlet, Symbol tag, Atom[] lst);
        public delegate void MethodObject(int inlet, object o);

        // --------------------------------------------------------------------------

        public readonly static Symbol _ = new Symbol("");
        public readonly static Symbol _bang = new Symbol("bang");
        public readonly static Symbol _float = new Symbol("float");
        public readonly static Symbol _symbol = new Symbol("symbol");
        public readonly static Symbol _pointer = new Symbol("pointer");
        public readonly static Symbol _list = new Symbol("list");

        // --------------------------------------------------------------------------

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        public extern static void Post(string message);

        public static void Post(string format, object arg0) { Post(String.Format(format, arg0)); }
        public static void Post(string format, object arg0, object arg1) { Post(String.Format(format, arg0, arg1)); }
        public static void Post(string format, object arg0, object arg1, object arg2) { Post(String.Format(format, arg0, arg1, arg2)); }
        public static void Post(string format, params object[] args) { Post(String.Format(format, args)); }

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        public extern static void PostError(string message);

        public static void PostError(string format, object arg0) { PostError(String.Format(format, arg0)); }
        public static void PostError(string format, object arg0, object arg1) { PostError(String.Format(format, arg0, arg1)); }
        public static void PostError(string format, object arg0, object arg1, object arg2) { PostError(String.Format(format, arg0, arg1, arg2)); }
        public static void PostError(string format, params object[] args) { PostError(String.Format(format, args)); }

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        public extern static void PostVerbose(int lvl, string message);

        public static void PostVerbose(int lvl, string format, object arg0) { PostVerbose(lvl, String.Format(format, arg0)); }
        public static void PostVerbose(int lvl, string format, object arg0, object arg1) { PostVerbose(lvl, String.Format(format, arg0, arg1)); }
        public static void PostVerbose(int lvl, string format, object arg0, object arg1, object arg2) { PostVerbose(lvl, String.Format(format, arg0, arg1, arg2)); }
        public static void PostVerbose(int lvl, string format, params object[] args) { PostVerbose(lvl, String.Format(format, args)); }

        // --------------------------------------------------------------------------

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static string SearchPath(string file);
    }


    public sealed class Class
        : Public
    {
        // PD class pointer
        private readonly ClassPtr ptr;

        // class type
        private Type extclass;

        // --------------------------------------------------------------------------        

        // simple methods for inlet 0
        private DynamicMethodBang m_bang = null;
        private DynamicMethodFloat m_float = null;
        private DynamicMethodSymbol m_symbol = null;
        private DynamicMethodPointer m_pointer = null;
        private DynamicMethodList m_list = null;
        private DynamicMethodAnything m_anything = null;
        private DynamicMethodObject m_object = null;

        // --------------------------------------------------------------------------        

        private static void ResizeArray(ref Hashtable[] oldArray, int newSize)
        {
            int oldSize = oldArray.Length;
            System.Type elementType = oldArray.GetType().GetElementType();
            Hashtable[] newArray = (Hashtable[])System.Array.CreateInstance(elementType, newSize);
            int preserveLength = System.Math.Min(oldSize, newSize);
            if (preserveLength > 0)
                System.Array.Copy(oldArray, newArray, preserveLength);
            oldArray = newArray;
        }

        private struct MapValue
        {
            public MapValue(Kind k,Delegate d) { this.k = k; this.d = d; }

            public Kind k;
            public Delegate d;
        }

        private Hashtable[] m_map = new Hashtable[0];

        // --------------------------------------------------------------------------        

        public override string ToString()
        {
            return extclass.Name;
        }

        // --------------------------------------------------------------------------        

        private enum Kind { k_bang, k_float, k_symbol, k_pointer, k_list, k_anything, k_object }

        private enum MethodFlags { f_none = 0,f_bang = 0x01, f_float = 0x02, f_symbol = 0x04, f_pointer = 0x08, f_list = 0x10, f_anything = 0x20 }

        private static MethodFlags methodflags;


        // --------------------------------------------------------------------------        

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static bool RegisterClass(ClassPtr ptr, Symbol sym, ClassType cflags, MethodFlags mflags);

        private void AddMethodIntern(int inlet, Symbol sel, Kind k, Delegate d)
        {
            // add to map
            if (m_map.Length <= inlet) ResizeArray(ref m_map, inlet + 1);
            Hashtable h = m_map[inlet];
            if(h == null) m_map[inlet] = h = new Hashtable(); 
            h[sel] = new MapValue(k, d);

            methodflags |= MethodFlags.f_anything;
        }

        public void AddMethod(int inlet, Symbol sel, MethodBang d)
        {
            DynamicMethodBang dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _bang)
            {
                m_bang = dyn;
                methodflags |= MethodFlags.f_bang;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_bang, dyn);
        }

        public void AddMethod(int inlet, Symbol sel, MethodFloat d)
        {
            DynamicMethodFloat dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _float)
            {
                m_float = dyn;
                methodflags |= MethodFlags.f_float;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_float, dyn);
        }

        public void AddMethod(int inlet, Symbol sel, MethodSymbol d)
        {
            DynamicMethodSymbol dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _symbol)
            {
                m_symbol = dyn;
                methodflags |= MethodFlags.f_symbol;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_symbol, dyn);
        }

        public void AddMethod(int inlet, Symbol sel, MethodPointer d)
        {
            DynamicMethodPointer dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _pointer)
            {
                m_pointer = dyn;
                methodflags |= MethodFlags.f_pointer;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_pointer, dyn);
        }

        public void AddMethod(int inlet, Symbol sel, MethodList d)
        {
            DynamicMethodList dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _list)
            {
                m_list = dyn;
                methodflags |= MethodFlags.f_list;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_list, dyn);
        }

        public void AddMethod(int inlet, Symbol sel, MethodAnything d)
        {
            DynamicMethodAnything dyn = DynamicMethods.Create(d);
            if (inlet == 0 && sel == _)
            {
                m_anything = dyn;
                methodflags |= MethodFlags.f_anything;
            }
            else
                AddMethodIntern(inlet, sel, Kind.k_anything, dyn);
        }

        public void AddMethod(int inlet, MethodObject d)
        {
            DynamicMethodObject dyn = DynamicMethods.Create(d);
            m_object = dyn;
            methodflags |= MethodFlags.f_anything;
        }

        // --------------------------------------------------------------------------        

        private Class(ClassPtr p,Type c)
        {
            ptr = p;
            extclass = c;
        }

        private delegate void DelegateClass(ClassPtr ptr, Symbol sym);
        private static void NewClass(ClassPtr ptr, Symbol sym)
        {
            Internal.pass.klass = null;

            try 
            {
                string name = sym.ToString();

                // load assembly according to name
                string file = SearchPath(name + ".dll");
                if (file.Length == 0) return; // throw new ArgumentException("Assembly file " + name + " not found");
                Assembly assembly = Assembly.LoadFile(file);
                if (assembly == null) return; // throw new ArgumentException("Assembly " + name + " could not be loaded");
                Type exttp = assembly.GetType(name);
                if (exttp == null) throw new ArgumentException("Class " + name + " could not be found");

                Class klass = new Class(ptr, exttp);

                // create dummy external
                ConstructorInfo ctor = exttp.GetConstructor(System.Type.EmptyTypes);
                if (ctor == null) throw new MissingMethodException("External class must have a default constructor");

                Internal.pass.klass = klass;
                Internal.extptr.Clear();

                External dummy = (External)ctor.Invoke(null);

                // reset flags
                methodflags = MethodFlags.f_none;

                // call Setup method
                MethodInfo setup = exttp.GetMethod("Setup",BindingFlags.NonPublic|BindingFlags.Static,null, new Type[1] { exttp },null);
                if (setup == null) throw new MissingMethodException("External class must have a Setup function");

                object ret = setup.Invoke(exttp, new object[1] { dummy });
                ClassType classflags;
                try { classflags = (ClassType)ret; }
                catch { classflags = ClassType.Default; }

                // set callbacks
                RegisterClass(ptr, sym, classflags, methodflags);
            }
            catch (Exception exc) 
            {
                Internal.pass.klass = null;
                PostError(exc.ToString());
            }
        }

        private delegate void DelegateNew(ExternalPtr ptr,AtomList args);
        private static void NewInstance(ExternalPtr ptr,AtomList args)
        {
            External instance;
            
            Class cl = Internal.pass.klass; // set by native side
            Internal.extptr = ptr;

//            Post("CLASS-NEW {0}",cl);
            try 
            {
                // TODO: create dynamic delegate for that....
                System.Reflection.ConstructorInfo m = cl.extclass.GetConstructor(new Type[1] { typeof(Atom[]) });
                if (m != null)
                    instance = (External)m.Invoke(new object[1] { (Atom[])args });
                else
                {
                    // search for the argument-less constructor... it must exist, we searched before
                    m = cl.extclass.GetConstructor(System.Type.EmptyTypes);
                    instance = (External)m.Invoke(null);
                }
            }
            catch (Exception exc) 
            {
                instance = null;
                PostError(exc.ToString()); 
            }

#if DEBUG
            Internal.extptr.Clear();
#endif

            Internal.pass.ext = instance; // back to native
         }

        private delegate void DelegateBang();
        private static void CallBang()
        {
            External e = Internal.pass.ext;
            try { e.klass.m_bang(e); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegateFloat(float f);
        private static void CallFloat(float f)
        {
            External e = Internal.pass.ext;
            try { e.klass.m_float(e, f); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegateSymbol(Symbol s);
        private static void CallSymbol(Symbol s)
        {
            External e = Internal.pass.ext;
            try { e.klass.m_symbol(e, s); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegatePointer(Pointer p);
        private static void CallPointer(Pointer p)
        {
            External e = Internal.pass.ext;
            try { e.klass.m_pointer(e, p); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegateList(AtomList l);
        private static void CallList(AtomList l)
        {
            External e = Internal.pass.ext;
            try { e.klass.m_list(e, (Atom[])l); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegateAnything(int inlet, Symbol s, AtomList l);
        private static void CallAnything(int inlet, Symbol s, AtomList l)
        {
//            Post("CLASS-ANYTHING {0}->{1}:{2}", inlet,s,l);

            try {
                External e = Internal.pass.ext;
                Class c = e.klass;

                Hashtable h;
                try { h = (Hashtable)c.m_map[inlet]; }
                catch (IndexOutOfRangeException) { h = null; }

                if(h != null)
                {
                    object fnd = h[s];
                    if (fnd != null)
                    {
                        MapValue mv = (MapValue)fnd;
                        switch(mv.k)
                        {
                            case Kind.k_bang:
                                {
                                    ((DynamicMethodBang)mv.d)(e);
                                    return;
                                }
                            case Kind.k_float:
                                {
                                    float f = l.Count > 0 ? (float)l[0] : 0;
                                    ((DynamicMethodFloat)mv.d)(e, f);
                                    return;
                                }
                            case Kind.k_symbol:
                                {
                                    Symbol sym = l.Count > 0 ? (Symbol)l[0] : _;
                                    ((DynamicMethodSymbol)mv.d)(e, sym);
                                    return;
                                }
                            case Kind.k_pointer:
                                {
                                    Pointer p = l.Count > 0 ? (Pointer)l[0] : new Pointer();
                                    ((DynamicMethodPointer)mv.d)(e, p);
                                    return;
                                }
                            case Kind.k_list:
                                ((DynamicMethodList)mv.d)(e,(Atom[])l);
                                return;
                            case Kind.k_anything:
                                ((DynamicMethodAnything)mv.d)(e,inlet,s,(Atom[])l);
                                return;
                            default:
                                throw new NotImplementedException("Selector " + s.ToString() + " not handled");
                        }
                    }
                }

                // no explicit method found...
                c.m_anything(e, inlet, s, (Atom[])l); 
            }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        private delegate void DelegateObject(int inlet);
        private static void CallObject(int inlet)
        {
//            Post("CLASS-OBJECT {0}", inlet);

            External e = Internal.pass.ext;
            try { e.klass.m_object(e, inlet, Internal.pass.obj); }
            catch (Exception exc) { PostError(exc.ToString()); }
        }

        // --------------------------------------------------------------------------        

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void Register(DelegateClass d_class, DelegateNew d_new, DelegateBang d_bang, DelegateFloat d_float, DelegateSymbol d_symbol, DelegatePointer d_pointer, DelegateList d_list, DelegateAnything d_anything, DelegateObject d_object);

        private static void Setup()
        {
            try
            {
                Internal.pass = new PassedData();

                Register(
                    new DelegateClass(NewClass),
                    new DelegateNew(NewInstance),
                    new DelegateBang(CallBang),
                    new DelegateFloat(CallFloat),
                    new DelegateSymbol(CallSymbol),
                    new DelegatePointer(CallPointer),
                    new DelegateList(CallList),
                    new DelegateAnything(CallAnything),
                    new DelegateObject(CallObject)
                );
            }
            catch (Exception exc) { PostError(exc.ToString()); }
        }
    }

   
    // This is the base class for a PD/CLR external
    public abstract class External
        : Public
    {
        // PD object pointer
        internal readonly ExternalPtr ptr;
        // Class
        internal readonly Class klass;

        // --------------------------------------------------------------------------

        protected External()
        {
            ptr = Internal.extptr;
            klass = Internal.pass.klass;
        }

        public override string ToString()
        {
            return ptr.Valid()?klass.ToString():"???";
        }

        // --------------------------------------------------------------------------

        protected static void AddMethod(int inlet, Symbol sel, MethodBang m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, Symbol sel, MethodFloat m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, Symbol sel, MethodSymbol m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, Symbol sel, MethodPointer m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, Symbol sel, MethodList m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, Symbol sel, MethodAnything m) { ((External)m.Target).klass.AddMethod(inlet, sel, m); }
        protected static void AddMethod(int inlet, MethodObject m) { ((External)m.Target).klass.AddMethod(inlet, m); }

        protected static void AddMethod(int inlet, MethodBang m) { AddMethod(inlet, _bang, m); }
        protected static void AddMethod(int inlet, MethodFloat m) { AddMethod(inlet, _float, m); }
        protected static void AddMethod(int inlet, MethodSymbol m) { AddMethod(inlet, _symbol, m); }
        protected static void AddMethod(int inlet, MethodPointer m) { AddMethod(inlet, _pointer, m); }
        protected static void AddMethod(int inlet, MethodList m) { AddMethod(inlet, _list, m); }
        protected static void AddMethod(int inlet, MethodAnything m) { AddMethod(inlet, _, m); }

        protected static void AddMethod(MethodBang m) { AddMethod(0, m); }
        protected static void AddMethod(MethodFloat m) { AddMethod(0, m); }
        protected static void AddMethod(MethodSymbol m) { AddMethod(0, m); }
        protected static void AddMethod(MethodPointer m) { AddMethod(0, m); }
        protected static void AddMethod(MethodList m) { AddMethod(0, m); }
        protected static void AddMethod(MethodAnything m) { AddMethod(0, m); }
        protected static void AddMethod(MethodObject m) { AddMethod(0, m); }

        protected static void AddMethod(int inlet, string sel, MethodBang m) { AddMethod(inlet, new Symbol(sel), m); }
        protected static void AddMethod(int inlet, string sel, MethodFloat m) { AddMethod(inlet, new Symbol(sel), m); }
        protected static void AddMethod(int inlet, string sel, MethodSymbol m) { AddMethod(inlet, new Symbol(sel), m); }
        protected static void AddMethod(int inlet, string sel, MethodPointer m) { AddMethod(inlet, new Symbol(sel), m); }
        protected static void AddMethod(int inlet, string sel, MethodList m) { AddMethod(inlet, new Symbol(sel), m); }
        protected static void AddMethod(int inlet, string sel, MethodAnything m) { AddMethod(inlet, new Symbol(sel), m); }

        // --------------------------------------------------------------------------

        protected void AddInlet(ref float f) { Internal.AddInlet(ptr, ref f); } // map to data member
        protected void AddInlet(ref Symbol s) { Internal.AddInlet(ptr, ref s); } // map to data member
        protected void AddInlet(ref Pointer p) { Internal.AddInlet(ptr,ref p); } // map to data member
//        protected void AddInlet(Symbol type) { Internal.AddInlet(ptr,type); } // create typed inlet
        protected void AddInlet() { Internal.AddInlet(ptr); } // create inlet responding to any message
        protected void AddInlet(Symbol sel, Symbol to_sel) { Internal.AddInlet(ptr, sel, to_sel); } // redirect messages to defined selector

        // --------------------------------------------------------------------------

        protected void AddOutlet(Symbol type) { Internal.AddOutlet(ptr, type); }

        protected void AddOutletBang() { AddOutlet(_bang); }
        protected void AddOutletFloat() { AddOutlet(_float); }
        protected void AddOutletSymbol() { AddOutlet(_symbol); }
        protected void AddOutletPointer() { AddOutlet(_pointer); }
        protected void AddOutletList() { AddOutlet(_list); }
        protected void AddOutletAnything() { AddOutlet(_); }

        // --------------------------------------------------------------------------

        protected void Outlet(int n) { Internal.Outlet(ptr,n); }
        protected void Outlet(int n,float f) { Internal.Outlet(ptr,n,f); }
        protected void Outlet(int n,Symbol s) { Internal.Outlet(ptr,n,s); }
        protected void Outlet(int n,Pointer p) { Internal.Outlet(ptr,n,p); }
        protected void Outlet(int n,Atom a) { Internal.Outlet(ptr,n,a); }
        protected void Outlet(int n,Atom[] l) { Internal.Outlet(ptr,n,_list,l); }
        protected void Outlet(int n,Symbol s,Atom[] l) { Internal.Outlet(ptr,n,s,l); }

        protected void OutletEx(int n,object o) { Internal.OutletEx(ptr,n,o); }

        // --------------------------------------------------------------------------

        // bind to symbol
        protected void Bind(Symbol sym) { Internal.Bind(ptr,sym); }
        protected void Unbind(Symbol sym) { Internal.Unbind(ptr,sym); }

        // send to receiver symbol
        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        protected extern static void Send(Symbol sym,Atom a);

        protected static void Send(Symbol sym) { Send(sym,_bang,new Atom[0]); }
        protected static void Send(Symbol sym,float f) { Send(sym,new Atom(f)); }
        protected static void Send(Symbol sym,Symbol s) { Send(sym,new Atom(s)); }
        protected static void Send(Symbol sym,Pointer p) { Send(sym,new Atom(p)); }
        protected static void Send(Symbol sym,Atom[] l) { Send(sym,_list,l); }

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        protected extern static void Send(Symbol sym,Symbol s,Atom[] l);

        [MethodImplAttribute (MethodImplOptions.InternalCall)]
        protected extern static void SendEx(Symbol sym,object o);
    }
}
