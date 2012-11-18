using System;
using PureData;

/// <summary>
/// Descrizione di riepilogo per Counter.
/// </summary>
public class Counter:
	External
{
    int i_count,i_down,i_up;
    float step;

    public Counter() { }

    public Counter(Atom[] args)
	{
	    this.step = args.Length >= 3?(float)args[2]:1;
        
        float f2 = args.Length >= 2?(float)args[1]:0;
        float f1 = args.Length >= 1?(float)args[0]:0;

        if(args.Length < 2) f2 = f1;
        
        this.i_down = (int)((f1<f2)?f1:f2);
        this.i_up = (int)((f1>f2)?f1:f2);
        
	    this.i_count = this.i_down;

        AddInlet(_list,new Symbol("bound"));
        AddInlet(ref step);

        AddOutlet(_float);
        AddOutlet(_bang);
    }

	// this function MUST exist
	private static void Setup(Counter obj)
	{
	    AddMethod(obj.Bang);
        AddMethod(0,"reset",obj.Reset);
        AddMethod(0,"set",obj.Set);
        AddMethod(0,"bound",obj.Bound);
	}

    protected void Bang() 
    {
    
        float f = this.i_count;
        int step = (int)this.step;
        this.i_count += step;
        
        if(this.i_down-this.i_up != 0) {
            if(step > 0 && this.i_count > this.i_up) {
                this.i_count = this.i_down;
                Outlet(1);
            }
            else if(this.i_count < this.i_down) {
                this.i_count = this.i_up;
                Outlet(1);
            }
        }
        
        Outlet(0,f);   
    }

    protected void Reset() 
    { 
        this.i_count = this.i_down;
    }

    protected void Set(float f) 
    { 
        this.i_count = (int)f;
    }

    protected void Bound(Atom[] args) 
    { 
        float f1 = (float)args[0];
        float f2 = (float)args[1];
        
        this.i_down = (int)((f1<f2)?f1:f2);
        this.i_up = (int)((f1>f2)?f1:f2);
    }
}
