#include <flext.h>

class myext
    : public flext_base
{
    FLEXT_HEADER_S(myext,flext_base,setup);
    
public:
    myext(int argc,t_atom *argv)
    {
        post("creating myext object");

        // initialization stuff...
    }
    
    ~myext()
    {
        post("destroying myext object");

        // destruction stuff...
    }

protected:
    static void setup(t_classid c)
    {
        post("setting up myext class");

        // add methods here...
        FLEXT_CADDBANG(c,0,m_bang);
    }

    void m_bang()
    {
        post("BANG!");
    }

    FLEXT_CALLBACK(m_bang)
};

FLEXT_NEW_V("myext",myext)
