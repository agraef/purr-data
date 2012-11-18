/* 
OSC - OpenSoundControl networking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1775 $
$LastChangedDate: 2007-12-11 09:17:04 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#include "osc.h"

namespace OSC {

template <typename T>
class Stub
{
public:
	Stub(T *p): ptr(p),refs(1) {}
	~Stub() { FLEXT_ASSERT(!refs); }
	
	Stub &operator ++() { ++refs; return *this; }
	bool operator --() { return --refs != 0; }
	
	T *operator *() { FLEXT_ASSERT(refs); return ptr; }
protected:
	T *ptr;
	int refs;
};

template <typename T>
class Socket
    : public UdpReceiveSocket
{
public:
    Socket(T *p,const IpEndpointName& localEndpoint ): UdpReceiveSocket(localEndpoint),ptr(p),refs(1) {}
	~Socket() { FLEXT_ASSERT(!refs); }

	Socket &operator ++() { ++refs; return *this; }
	bool operator --() { return --refs != 0; }
	
	T *operator *() { FLEXT_ASSERT(refs); return ptr; }

protected:
	T *ptr;
    int refs;
};

class UdpRecv
    : public flext_base
    , public osc::OscPacketListener
{
    FLEXT_HEADER_S(UdpRecv,flext_base,Setup)

public:
    UdpRecv(int p)
        : bufsz(0),buffer(NULL)
        , port(p)
        , socket(NULL)
	{
        AddInAnything();
        AddOutAnything("OSC message");
        AddOutList("timetag");

        ms_buffer(1024);
    }

	virtual bool Finalize()
	{
		if(!flext_base::Finalize()) return false;
        UpdateSocket();
		return true;
	}

    virtual ~UdpRecv()
    {
        if(socket && !--*socket) delete socket;
		if(buffer) delete[] buffer;	
    }

    void ms_port(int p) { if(port != p) { port = p; UpdateSocket(); } }

	void ms_buffer(int sz)
	{
		if(sz <= 0) 
			throw std::runtime_error("Buffer size must be > 0");
			
		if(bufsz == sz) return;	
		
		if(buffer) delete[] buffer;	
		buffer = new char [bufsz = sz];
	}

    virtual void ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint, osc::uint64 timetag )
    {
        AtomListStatic<32> args(m.ArgumentCount());
        osc::ReceivedMessageArgumentIterator it = m.ArgumentsBegin();
        for(int i = 0; i < args.Count(); ++i,++it) {
            const osc::ReceivedMessageArgument &arg = *it;
            if(arg.IsBool())
                SetBool(args[i],arg.AsBoolUnchecked());
            else if(arg.IsChar()) {
                char c[2];
                c[0] = arg.AsCharUnchecked();
                c[1] = 0;
                SetString(args[i],c);
            }
            else if(arg.IsDouble()) {
                // should we use double precision?
                SetFloat(args[i],(float)arg.AsDoubleUnchecked());
            }
            else if(arg.IsFloat()) {
                SetFloat(args[i],arg.AsFloatUnchecked());
            }
            else if(arg.IsInt32()) {
                // not a good idea to convert that to a PD float
                // should we use double precision?
                SetInt(args[i],arg.AsInt32Unchecked());
            }
            else if(arg.IsInt64()) {
                // not a good idea to convert that to a PD float
                // should we use double precision?
                SetInt(args[i],(int)arg.AsInt64Unchecked());
            }
            else if(arg.IsString()) {
                SetString(args[i],arg.AsStringUnchecked());
            }
            else if(arg.IsSymbol()) {
                SetString(args[i],arg.AsSymbolUnchecked());
            }
            else {
                post("%s %c - OSC type can't be converted",thisName(),arg.TypeTag());
                return;
            }
        }

        t_atom at[2];
        SetTimetag(at,timetag <= 1?0:timetag);
        ToSysList(1,2,at);

        ToSysAnything(0,MakeSymbol(m.AddressPattern()),args);
    }

protected:
    int bufsz;
	char *buffer;

    static t_int Idle(t_int *data)
    {
        Socket<UdpRecv> *socket = (Socket<UdpRecv> *)data[0];
		if(!--*socket) {
			delete socket;
			return 0;
		}
        else
            ++*socket;

        UdpRecv *th = **socket;
		IpEndpointName remoteEndpoint;
		int size = socket->ReceiveFrom( remoteEndpoint,th->buffer,th->bufsz);
		if(size > 0)
			th->ProcessPacket(th->buffer, size, remoteEndpoint );

        return 2;
    }

    void UpdateSocket()
    {
		if(Initing()) return;
	
        if(socket) {
            if(!--*socket) 
                delete socket; 
            socket = NULL; 
        }

        if(port) {
			try {
                socket = new Socket<UdpRecv>(this,IpEndpointName(IpEndpointName::ANY_ADDRESS,port));
                ++*socket;
				t_int data = (t_int)socket;
				sys_callback(Idle,&data,1);
			}
			catch(...) {
				t_atom atom; SetSymbol(atom,sym_socket);
				ToSysAnything(GetOutAttr(),sym_error,1,&atom);
			}
        }
    }

    int port;
    Socket<UdpRecv> *socket;

    FLEXT_ATTRGET_I(port)
    FLEXT_CALLSET_I(ms_port)
    FLEXT_ATTRGET_I(bufsz)
    FLEXT_CALLSET_I(ms_buffer)

	static Symbol sym_error,sym_socket;

    static void Setup(t_classid c)
    {
		sym_error = MakeSymbol("error");
		sym_socket = MakeSymbol("socket");

        FLEXT_CADDATTR_VAR(c,"port",port,ms_port);
        FLEXT_CADDATTR_VAR(c,"buffer",bufsz,ms_buffer);		
    }
};

Symbol UdpRecv::sym_error,UdpRecv::sym_socket;

FLEXT_LIB_1("osc.udprecv, osc",UdpRecv,int0)

} // namespace
