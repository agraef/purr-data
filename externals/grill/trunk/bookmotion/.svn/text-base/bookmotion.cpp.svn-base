#include <flext.h>
#include "unimotion.h"

class bookmotion
    : public flext_base
{
    FLEXT_HEADER_S(bookmotion,flext_base,setup)
    
public:
    bookmotion()
    {
    	type = detect_sms();
    	if(type == unknown)
            error("tilt: Can't find/identify a Motion Sensor.");
        AddOutInt(3);
    }

    void m_bang() 
    {
        if (type != unknown) {
            int sms_x, sms_y, sms_z;
            if(read_sms_raw(type, &sms_x, &sms_y, &sms_z)) {
                ToOutInt(2,sms_z);
                ToOutInt(1,sms_y);
                ToOutInt(0,sms_x);
            }
        }
    }
    
protected:
    FLEXT_CALLBACK(m_bang);

	int	type;
    
    static void setup(t_classid c)
    {
        FLEXT_CADDBANG(c,0,m_bang);
    }
};

FLEXT_NEW("bookmotion",bookmotion)
