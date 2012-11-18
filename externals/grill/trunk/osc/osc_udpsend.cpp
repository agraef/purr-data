/* 
OSC - OpenSoundControl networking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.

$LastChangedRevision: 1773 $
$LastChangedDate: 2007-11-15 06:04:08 -0500 (Thu, 15 Nov 2007) $
$LastChangedBy: thomas $
*/

#include "osc.h"

// protocol
#include "ip/UdpSocket.h"

namespace OSC {

class UdpSend
    : public flext_base
{
    FLEXT_HEADER_S(UdpSend,flext_base,Setup)

public:
    UdpSend(int argc,const t_atom *argv)
        : socket(NULL)
        , host(NULL),port(0)
        , bundle(0)
		, autosend(true)
		, buffer(NULL),bufsz(0)
		, packet(NULL)
    {
        if(argc) {
            host = GetASymbol(*argv++,sym__);
            --argc;
        }
        if(argc && CanbeInt(*argv))
            port = GetAInt(*argv),--argc;
        if(argc)
            throw std::runtime_error("Invalid creation arguments");

        AddInAnything();
		
		ms_buffer(1024);
    }
	
	virtual bool Finalize()
	{
		if(!flext_base::Finalize()) return false;
        UpdateSocket();
		return true;
	}

	virtual ~UdpSend()
	{
        if(socket) delete socket;
		if(packet) delete packet;
		if(buffer) delete[] buffer;
	}

    void ms_port(int p) { if(port != p) { port = p; UpdateSocket(); } }

    void ms_host(Symbol h) 
	{ 
		if(h == sym__) h = NULL; 
		if(host != h) { 
			host = h; 
			UpdateSocket(); 
		} 
	}

    void mg_host(Symbol &h) { h = host?host:sym__; }

	void ms_buffer(int sz)
	{
		if(sz <= 0) 
			throw std::runtime_error("Buffer size must be > 0");
			
		m_reset();
		if(bufsz == sz) return;	
		
		if(buffer) delete[] buffer;
		
		buffer = new char [bufsz = sz];
		packet = new osc::OutboundPacketStream(buffer,bufsz);
	}

	void m_reset()
	{
		bundle = 0;
		if(packet) packet->Clear();
	}
	
	void m_send(bool s = false)
	{
		FLEXT_ASSERT(packet);
		while(bundle) { --bundle; *packet << osc::EndBundle; }
		Send(s);
	}

	void m_bang() { m_send(true); }

    void m_bundleopen(int argc,const t_atom *argv) 
    {
        double t = 0;
        if(argc--) t += GetAFloat(*argv++);
        if(argc--) t += GetAFloat(*argv++);
        osc::uint64 timetag = GetTimetag(t);

        FLEXT_ASSERT(packet);
		++bundle;

        if(timetag <= 1)
            // here immediate timetag can also be 0... but do OSC a favor
    		*packet << osc::BeginBundleImmediate;
        else
		    *packet << osc::BeginBundle(timetag);
    }

    void m_bundleclose() 
    {
		if(!bundle) 
			throw std::runtime_error("No open bundle");

		FLEXT_ASSERT(packet);
		--bundle;
		*packet << osc::EndBundle;
		
		if(!bundle && autosend) Send(true);
	}

    std::string Convert(const char *txt,const char *var,int &argc,const t_atom *&argv)
    {
        FLEXT_ASSERT(txt);

        std::string ret;
        for(;;) {
            if(var)
                ret.append(txt,var-txt);
            else {
                ret.append(txt);
                break;
            }

            char typetag = var[1];
            // part of a symbol
            switch(typetag) {
                case osc::INT32_TYPE_TAG: {
                    char tmp[10];
                    int z = (argc--)?GetAInt(*argv++):0;
					sprintf(tmp,"%i",z);
                    ret += tmp;
                    break;
                }
                case osc::CHAR_TYPE_TAG: {
                    Symbol s = (argc--)?GetASymbol(*argv++):NULL;
                    ret += (s?*GetString(s):'\0');
                    break;
                }
                case osc::SYMBOL_TYPE_TAG:
                case osc::STRING_TYPE_TAG: {
                    Symbol s = (argc--)?GetASymbol(*argv++):NULL;
                    ret += (s?GetString(s):"");
                    break;
                }
                case '%':
                    ret += '%';
                    break;
                default:
                    post("%s %s - Type tag %s not handled in string",thisName(),GetString(thisTag()),typetag);
            }

            if(!typetag) break;

            // find next one
            txt = var+2;
            var = strchr(txt,'%');
        }

        return ret;
    }

    virtual bool CbMethodResort(int inlet,const t_symbol *sym,int argc,const t_atom *argv)
    {
		const char *dst = GetString(sym);
		if(*dst != '/')
			return false;
		
		FLEXT_ASSERT(packet);

        {
            // treat destination string
            const char *var = strchr(dst,'%');
            *packet << osc::BeginMessage(var?Convert(dst,var,argc,argv).c_str():dst);
        }

        while(argc) {
            if(IsSymbol(*argv)) {
                const char *hdr = GetString(*argv++); --argc;

                const char *var = strchr(hdr,'%');
                if(var) {
                    // variable found in string
                    char typetag = var[1];
                    if(!typetag) 
                        // % is only character
                        *packet << "%";
                    else if(hdr != var || var[2])
                        // variable not in front, or string more than 2 chars long -> substitute variables
                        *packet << Convert(hdr,var,argc,argv).c_str();
                    else {
                        // standalone
                        switch(typetag) {
                            case osc::TRUE_TYPE_TAG: *packet << true; break;
                            case osc::FALSE_TYPE_TAG: *packet << false; break;
                            case osc::NIL_TYPE_TAG: *packet << osc::Nil; break;
                            case osc::INFINITUM_TYPE_TAG: *packet << osc::Infinitum; break;
                            case osc::INT32_TYPE_TAG: {
                                osc::int32 z = (argc--)?GetAInt(*argv++):0;
                                *packet << z;
                                break;
                            }
                            case osc::FLOAT_TYPE_TAG: {
                                float z = (argc--)?GetAFloat(*argv++):0;
                                *packet << z;
                                break;
                            }
                            case osc::CHAR_TYPE_TAG: {
                                Symbol s = (argc--)?GetASymbol(*argv++):NULL;
                                *packet << (s?*GetString(s):'\0');
                                break;
                            }
                            case osc::RGBA_COLOR_TYPE_TAG: {
                                osc::uint32 r = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 g = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 b = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 a = (argc--)?(GetAInt(*argv++)&0xff):0;
                                *packet << osc::RgbaColor((r<<24)+(g<<16)+(b<<8)+a);
                                break;
                            }
                            case osc::MIDI_MESSAGE_TYPE_TAG: {
                                osc::uint32 channel = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 status = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 data1 = (argc--)?(GetAInt(*argv++)&0xff):0;
                                osc::uint32 data2 = (argc--)?(GetAInt(*argv++)&0xff):0;
                                *packet << osc::MidiMessage((channel<<24)+(status<<16)+(data1<<8)+data2);
                                break;
                            }
                            case osc::INT64_TYPE_TAG: {
                                osc::int64 z = 0;
                                if(argc--) z += GetAInt(*argv++);
                                if(argc--) z += GetAInt(*argv++);
                                *packet << z;
                                break;
                            }
                            case osc::TIME_TAG_TYPE_TAG: {
                                double z = 0;
                                if(argc--) z += GetAFloat(*argv++);
                                if(argc--) z += GetAFloat(*argv++);
                                *packet << osc::TimeTag(GetTimetag(z));
                                break;
                            }
                            case osc::DOUBLE_TYPE_TAG: {
                                double z = 0;
                                if(argc--) z += GetAFloat(*argv++);
                                if(argc--) z += GetAFloat(*argv++);
                                *packet << z;
                                break;
                            }
                            case osc::STRING_TYPE_TAG: {
                                Symbol s = (argc--)?GetASymbol(*argv++):NULL;
                                *packet << (s?GetString(s):"");
                                break;
                            }
                            case osc::SYMBOL_TYPE_TAG: {
                                Symbol s = (argc--)?GetASymbol(*argv++):NULL;
                                *packet << osc::Symbol(s?GetString(s):"");
                                break;
                            }
                            case osc::BLOB_TYPE_TAG:
                                post("%s %s - Blob type not supported",thisName(),GetString(thisTag()));
                                break;
                            default:
                                post("%s %s - Unknown type tag %s",thisName(),GetString(thisTag()),typetag);
                        }
                    }
                }
                else
                    *packet << osc::Symbol(hdr);
            }
            else if(CanbeFloat(*argv))
                *packet << GetAFloat(*argv++),--argc;
            else {
                post("%s %s - Invalid atom type",thisName(),GetString(thisTag()));
                ++argv,--argc;
            }
        }
        *packet << osc::EndMessage;

        if(!bundle && autosend) Send(true);

        return true;
    }

protected:
    UdpTransmitSocket *socket;
    Symbol host;
    int port;
    int bundle;
	bool autosend;
	char *buffer;
	int bufsz;
    osc::OutboundPacketStream *packet;

    void Send(bool clear)
    {
		FLEXT_ASSERT(packet);
		FLEXT_ASSERT(buffer);
		FLEXT_ASSERT(!bundle);
	
        if(LIKELY(socket)) {
            socket->Send( packet->Data(), packet->Size() );
            if(clear) packet->Clear();
        }
        else
            post("%s %s - Socket not initialized",thisName(),GetString(thisTag()));
    }

    void UpdateSocket()
    {
		if(Initing()) return;
	
        if(socket) { delete socket; socket = NULL; }

        if(host && port) {
			try {
				socket = new UdpTransmitSocket(IpEndpointName(GetString(host),port));
			}
			catch(...) {
				t_atom atom; SetSymbol(atom,sym_socket);
				ToSysAnything(GetOutAttr(),sym_error,1,&atom);
			}
        }
    }

    FLEXT_CALLVAR_S(mg_host,ms_host)
    FLEXT_ATTRGET_I(port)
    FLEXT_CALLSET_I(ms_port)
    FLEXT_ATTRGET_I(bufsz)
    FLEXT_CALLSET_I(ms_buffer)
	FLEXT_ATTRVAR_B(autosend)

    FLEXT_CALLBACK(m_reset)
    FLEXT_CALLBACK(m_bang)
    FLEXT_CALLBACK(m_send)
    FLEXT_CALLBACK_V(m_bundleopen)
    FLEXT_CALLBACK(m_bundleclose)

	static Symbol sym_error,sym_socket;

    static void Setup(t_classid c)
    {
		sym_error = MakeSymbol("error");
		sym_socket = MakeSymbol("socket");

        FLEXT_CADDATTR_VAR(c,"host",mg_host,ms_host);
        FLEXT_CADDATTR_VAR(c,"port",port,ms_port);
        FLEXT_CADDATTR_VAR(c,"buffer",bufsz,ms_buffer);
        FLEXT_CADDATTR_VAR1(c,"auto",autosend);

        FLEXT_CADDMETHOD(c,0,m_bang);
        FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
        FLEXT_CADDMETHOD_(c,0,"send",m_send);
        FLEXT_CADDMETHOD_(c,0,"[",m_bundleopen);
        FLEXT_CADDMETHOD_(c,0,"]",m_bundleclose);
    }
};

Symbol UdpSend::sym_error,UdpSend::sym_socket;

FLEXT_LIB_V("osc.udpsend, osc",UdpSend)

} // namespace
