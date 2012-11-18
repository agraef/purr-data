using System;
using Timing;
using PureData;

public class test:
    External
{
    Atom[] args;
    float farg;

    // necessary (for now...)
    public test() {}  

    public test(Atom[] args)
    {
        Post("Test.ctor "+args.ToString());

        // save args
        this.args = args;

        //        AddInlet(s_list,new PureData.Symbol("list2"));
        AddInlet();
        AddInlet(ref farg);
        AddInlet();
        AddOutletAnything();
    }

    // this function MUST exist
    // returns void or ClassType
    private static ClassType Setup(test obj)
    {
//        Post("Test.Setup");

        AddMethod(obj.bang);
        AddMethod(obj.MyFloat);
        AddMethod(obj.symbol);
        AddMethod(obj.list);
        AddMethod(0,"set",obj.set);
        AddMethod(0,"send",obj.send);
        AddMethod(0,"trigger",obj.trigger);
        AddMethod(0,obj.MyObject);
        AddMethod(0,obj.MyAnything);
        AddMethod(1,obj.MyFloat1);
        AddMethod(1,obj.MyAny1);
        return ClassType.Default;
    }

    protected void bang() 
    { 
        Post("Test-BANG "+farg.ToString()); 
        Outlet(0);
    }

    protected virtual void MyFloat(float f) 
    { 
        Post("Test-FLOAT "+f.ToString()); 
        Outlet(0,f);
    }

    protected virtual void MyFloat1(float f) 
    { 
        Post("Test-FLOAT1 "+f.ToString()); 
    }

    protected virtual void MyAny1(int ix,Symbol s,Atom[] l) 
    { 
        Post(ix.ToString()+": Test-ANY1 "+l.ToString()); 
    }

    protected virtual void symbol(Symbol s) 
    { 
        Post("Test-SYMBOL "+s.ToString()); 
        Outlet(0,s);
    }

    protected virtual void list(Atom[] l) 
    { 
        Post("Test-LIST "+l.ToString()); 
        Outlet(0,l);
    }

    protected virtual void set(int ix,Symbol s,Atom[] l) 
    { 
        Post("Test-SET "+l.ToString()); 
        Outlet(0,new Symbol("set"),l);
    }

    protected virtual void send(int ix,Symbol s,Atom[] l) 
    { 
        Send(new Symbol("receiver"),l);
    }

    protected virtual void trigger() 
    { 
        OutletEx(0,"hey");
    }

    protected virtual void MyObject(int ix,object obj) 
    { 
        Post("OBJECT "+obj.ToString());
        OutletEx(0,obj);
    }

    protected virtual void MyAnything(int ix,Symbol s,Atom[] l) 
    { 
        Post(ix.ToString()+": Test-("+s.ToString()+") "+l.ToString()); 
        Outlet(0,s,l);
    }
}
