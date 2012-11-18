/*
=============================================================================
COPYRIGHT (C) 2009 Thomas Grill (gr@grrrr.org)

This code implements a flext-based external for Pure Data or Max/MSP
interfacing to the Percussa audiocubes SDK.
=============================================================================
*/

#include <flext.h>
#include "libCube2.h"

// anonymous namespace
namespace { 

class audiocube
    : public flext_base
{
    FLEXT_HEADER_S(audiocube,flext_base,setup);
    
public:
    audiocube()
        : init(false)
    {
        AddInAnything();
        AddOutAnything();

        FLEXT_ADDTIMER(tmr,timer);
        // delayed installation of callback
        tmr.Now();
    }
    
    ~audiocube()
    {
        CubeRemoveEventCallback(st_callback, this);
    }
    
    void m_setcolor(int cube,float r,float g,float b)
    {
        CubeColor c = {r*255,g*255,b*255};
        CubeSetColor(cube,c);
    }

    void m_getcolor(int cube) const
    {
        t_atom at[4];
        CubeColor c = CubeGetColor(cube);
        SetInt(at[0],cube);
        SetFloat(at[1],c.red/255.);
        SetFloat(at[2],c.green/255.);
        SetFloat(at[3],c.blue/255.);
        ToOutAnything(0,sym_color,4,at);
    }

    void m_getsensor(int cube,int face) const
    {
        t_atom at[3];
        SetInt(at[0],cube);
        SetInt(at[1],face);
        SetFloat(at[2],CubeSensorValueRaw(cube,face));
        ToOutAnything(0,sym_sensor,3,at);
    }

    void m_getsensors(int cube) const
    {
        t_atom at[5];
        SetInt(at[0],cube);
        for(int f = 0; f < 4; ++f)
            SetFloat(at[1+f],CubeSensorValueRaw(cube,f));
        ToOutAnything(0,sym_sensors,5,at);
    }

protected:
    Timer tmr;
    bool init;
    
    static const t_symbol *sym_attach,*sym_detach,*sym_topology,*sym_color,*sym_sensors,*sym_sensor;

    static void setup(t_classid c)
    {
        post("audiocube external loaded");
        post("(c)2009 Thomas Grill");

        sym_attach = MakeSymbol("attach");
        sym_detach = MakeSymbol("detach");
        sym_topology = MakeSymbol("topology");
        sym_color = MakeSymbol("color");
        sym_sensor = MakeSymbol("sensor");
        sym_sensors = MakeSymbol("sensors");
        
        FLEXT_CADDMETHOD_4(c,0,sym_color,m_setcolor,int,float,float,float);
        FLEXT_CADDMETHOD_(c,0,"getcolor",m_getcolor);
        FLEXT_CADDMETHOD_II(c,0,"getsensor",m_getsensor);
        FLEXT_CADDMETHOD_I(c,0,"getsensors",m_getsensors);
    }
    
    // caution: this is called from a secondary thread!
    void callback(int cube,unsigned int event,unsigned int param)
    {
        const t_symbol *sym = NULL;
        AtomListStatic<64> at;
        
        switch(event) {
            case CUBE_EVENT_USB_ATTACHED:
                sym = sym_attach; 
                at(1);
                SetInt(at[0],cube);
                break;
                
            case CUBE_EVENT_USB_DETACHED:
                sym = sym_detach; 
                at(1);
                SetInt(at[0],cube);
                break;
                
            case CUBE_EVENT_TOPOLOGY_UPDATE: {
                int topology_length;
                unsigned char topology[CUBE_MAX_TOPOLOGY_TABLE_SIZE];
                topology_length = CubeTopology(topology);

                sym = sym_topology;
                at(topology_length*4);
                for(int i = 0; i < topology_length; ++i) {
                    SetInt(at[0+i*4],topology[i*2+0]/16);
                    SetInt(at[1+i*4],topology[i*2+0]%16);
                    SetInt(at[2+i*4],topology[i*2+1]/16);
                    SetInt(at[3+i*4],topology[i*2+1]%16);
                }
                
                if(!init) {
                    // TODO: do some initialization stuff (attach flag, color, sensors etc.)
                    init = true;
                }
                
                break;
            }
            
            case CUBE_EVENT_SENSOR_UPDATE: 
                sym = sym_sensor; 
                at(3);
                SetInt(at[0],cube);
                SetInt(at[1],param); 
                SetFloat(at[2],CubeSensorValue(cube,param));
                break;
                
            case CUBE_EVENT_COLOR_CHANGED: {
                sym = sym_color; 
                at(4);
                SetInt(at[0],cube);
                CubeColor c = CubeGetColor(cube);
                SetFloat(at[1],c.red/255.);
                SetFloat(at[2],c.green/255.);
                SetFloat(at[3],c.blue/255.);
                break;
            }
            
            default: 
                FLEXT_ASSERT(false);
        }
        
        if(sym)
            // pass over to main thread
            ToQueueAnything(0,sym,at.Count(),at.Atoms());
    }

    static void st_callback(void *th,int cube,unsigned int event,unsigned int param)
    {
        FLEXT_ASSERT(th);
        static_cast<audiocube *>(th)->callback(cube,event,param);
    }

    void timer(void *)
    {
        // set callback
        CubeSetEventCallback(st_callback, this);
    }

    FLEXT_CALLBACK_4(m_setcolor,int,float,float,float)
    FLEXT_CALLBACK_I(m_getcolor)
    FLEXT_CALLBACK_II(m_getsensor)
    FLEXT_CALLBACK_I(m_getsensors)
    FLEXT_CALLBACK_T(timer);
};

const t_symbol *audiocube::sym_attach,*audiocube::sym_detach,*audiocube::sym_topology,*audiocube::sym_color,*audiocube::sym_sensor,*audiocube::sym_sensors;

FLEXT_NEW("audiocube",audiocube)

} // namespace
