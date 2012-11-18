using System;
using System.Runtime.InteropServices; // for structures
using System.Collections;
#if NET_2_0
using System.Collections.Generic;
#endif

namespace PureData
{
    [StructLayout (LayoutKind.Sequential)]
    public struct Symbol
    {
        // this should NOT be public (or at least read only)
        private readonly IntPtr sym;

        public Symbol(string s)
        {
            this = Internal.SymGen(s);
        }

        public override string ToString()
        {
            return Internal.SymEval(this);
        }

        public static bool operator ==(Symbol s1,Symbol s2)
        {
            return s1.sym == s2.sym;
        }

        public static bool operator !=(Symbol s1, Symbol s2)
        {
            return s1.sym != s2.sym;
        }

        public override bool Equals(object o)
        {
            try { return this == (Symbol)o; }
            catch {}
            return false;
        }

        public override int GetHashCode()
        {
            return (int)sym;
        }
    }

    [StructLayout (LayoutKind.Sequential)]
    public struct Pointer
    {
        private readonly IntPtr ptr;

        public override string ToString()
        {
            return ptr.ToString();
        }
    }

	[StructLayout (LayoutKind.Sequential)]
	public struct Atom 
	{
        private enum AtomType {Null = 0, Float = 1, Symbol = 2, Pointer = 3};
        
        [StructLayout (LayoutKind.Explicit)]
        private struct Word
        {
            [FieldOffset(0)] public float w_float;
            [FieldOffset(0)] public Symbol w_sym;
            [FieldOffset(0)] public Pointer w_ptr;
        }

		private AtomType type;
		private Word word;
		
		public Atom(float f)
		{
			type = AtomType.Float;
			word = new Word();
			word.w_float = f;
		}

		public Atom(int i)
		{
            type = AtomType.Float;
            word = new Word();
            word.w_float = (float)i;
        }

        public Atom(Symbol s)
        {
            type = AtomType.Symbol;
            word = new Word();
            word.w_sym = s;
        }
        
		public Atom(string s)
		{
            type = AtomType.Symbol;
            word = new Word();
            word.w_sym = new Symbol(s);
		}

        public Atom(Pointer p)
        {
            type = AtomType.Pointer;
            word = new Word();
            word.w_ptr = p;
        }

        public bool IsFloat { get { return type == AtomType.Float; } }
        public bool IsSymbol { get { return type == AtomType.Symbol; } }
        public bool IsPointer { get { return type == AtomType.Pointer; } }

        public float ToFloat()
        {
            if(IsFloat)
                return word.w_float;
            else
                throw new System.InvalidCastException("Can't be cast to float.");
        }
        
        public Symbol ToSymbol()
        {
            if(IsSymbol)
                return word.w_sym;
            else
                throw new System.InvalidCastException("Can't be cast to Symbol.");
        }
        
        public Pointer ToPointer()
        {
            if(IsPointer)
                return word.w_ptr;
            else
                throw new System.InvalidCastException("Can't be cast to Pointer.");
        }
        
        override public string ToString()
        {
            if(IsFloat)
                return word.w_float.ToString();
            else if(IsSymbol)
                return word.w_sym.ToString();
            else if(IsPointer)
                return word.w_ptr.ToString();
            else
                // should never happen
                throw new System.InvalidProgramException("Internal error.");
        }

        public static explicit operator float(Atom a)
        {
            return a.ToFloat();
        }

        public static explicit operator Symbol(Atom a)
        {
            return a.ToSymbol();
        }

        public static explicit operator Pointer(Atom a)
        {
            return a.ToPointer();
        }
    }
	
    internal class AtomListEnum
        : IEnumerator
    {
        public AtomList list;

        // Enumerators are positioned before the first element
        // until the first MoveNext() call.
        int position = -1;

        public AtomListEnum(AtomList l)
        {
            list = l;
        }

        public bool MoveNext()
        {
            return ++position < list.Count;
        }

        public void Reset()
        {
            position = -1;
        }

        public object Current
        {
            get
            {
                try
                {
                    return list[position];
                }
                catch (IndexOutOfRangeException)
                {
                    throw new InvalidOperationException();
                }
            }
        }
    }


    // attention: this is dangerous, because we could do the following
    // AtomList l2 = l;  
    // with l also being an AtomList... the two private members of the struct will get copied, although atoms is only a temporary reference

    [StructLayout (LayoutKind.Sequential)]
    internal unsafe struct AtomList
#if xNET_2_0
		: ICollection<Atom>
#else
        : ICollection
#endif
    {
        private readonly int len;
        private readonly Atom *atoms;
        
        public int Count { get { return len; } }
#if xNET_2_0
        public bool IsReadOnly { get { return false; } } // member of generic.ICollection<Atom> (C# 2.0)
#endif        
        public bool IsSynchronized { get { return false; } }
        public Object SyncRoot { get { return null; } }

        // protect this from being used
        private AtomList(AtomList a) { len = 0; atoms = null; }

#if xNET_2_0
        public void CopyTo(Atom[] array,int start)
#else        
        public void CopyTo(Array array,int start)
#endif        
        {
            if(len > array.GetUpperBound(0)+1-start)
                throw new System.ArgumentException("Destination array is not long enough.");
            int i;                
            for(i = 0; i < len-start; ++i)
                array.SetValue(atoms[start+i],i);
        }

        public IEnumerator GetEnumerator()
        {
            return new AtomListEnum(this);
        }
        
        public Atom this[int i]
        {
            get
            {
                if(i < 0 || i >= len)
                    throw new System.IndexOutOfRangeException("Index outside array bounds.");
                return atoms[i];
            }
            set
            {
                if(i < 0 || i >= len)
                    throw new System.IndexOutOfRangeException("Index outside array bounds.");
                atoms[i] = value;
            }
        }

//#if 1 // !NET_2_0
        public static explicit operator Atom[](AtomList l)
        {
            Atom[] ret = new Atom[l.Count];
            int i;
            for(i = 0; i < l.Count; ++i) ret[i] = l.atoms[i];
            return ret;
        }
//#endif        

        override public string ToString()
        {
            string n = "{";
            if(len > 0) {
                int i;
                for(i = 0; i < len-1; ++i) n += atoms[i].ToString()+",";
                n += atoms[i].ToString();
            }
            return n+"}";
        }
    }	
}
