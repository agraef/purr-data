#include <flext.h>

extern "C" {
#include "Wacom.h"
#include "WacomHelperFuncs.h"
#include "TAEHelpers.h"
}

#include <set>
#include <map>

#define EVTIME 0.005

typedef struct LongRect {
   long	top;
   long	left;
   long	bottom;
   long	right;
} LongRect;


class wacom
	: public flext_base
{
	friend class Tablet;

	FLEXT_HEADER_S(wacom,flext_base,Setup)
	
public:

	wacom(int index);
	~wacom();
	
	static void Setup(t_classid c);

protected:
	void CbProximity(const TabletProximityRec &m)
	{
		t_atom lst[3];
		SetInt(lst[0],m.deviceID);  /* system-assigned unique device ID - matches to deviceID field in tablet event */
		SetBool(lst[1],m.enterProximity != 0);  /* non-zero = entering; zero = leaving */
		SetInt(lst[2],m.pointerType);       /* type of pointing device - enum to be defined */
		ToOutAnything(GetOutAttr(),sym_proximity,3,lst);
	}

	void CbPoint(const TabletPointRec &m)
	{
		t_atom lst[10];
		SetInt(lst[0],m.deviceID);  /* system-assigned unique device ID - matches to deviceID field in tablet event */
		SetInt(lst[1],m.absX);  /* absolute x coordinate in tablet space at full tablet resolution */
		SetInt(lst[2],m.absY);                   /* absolute y coordinate in tablet space at full tablet resolution */
		SetInt(lst[3],m.absZ);                   /* absolute z coordinate in tablet space at full tablet resolution */
		SetInt(lst[4],m.buttons);                /* one bit per button - bit 0 is first button - 1 = closed */
		SetFloat(lst[5],m.pressure/(float)((1<<16)-1));               /* scaled pressure value; MAXPRESSURE=(2^16)-1, MINPRESSURE=0 */
		SetFloat(lst[6],m.tiltX/(float)((1<<15)-1));                  /* scaled tilt x value; range is -((2^15)-1) to (2^15)-1 (-32767 to 32767) */
		SetFloat(lst[7],m.tiltY/(float)((1<<15)-1));                  /* scaled tilt y value; range is -((2^15)-1) to (2^15)-1 (-32767 to 32767) */
		SetFloat(lst[8],m.rotation/64.f);  // degrees  /* Fixed-point representation of device rotation in a 10.6 format */
		SetFloat(lst[9],m.tangentialPressure/(float)((1<<15)-1));  /* tangential pressure on the device; range same as tilt */
		ToOutAnything(GetOutAttr(),sym_point,10,lst);
	}

private:
	Tablet *tablet;

	static const t_symbol *sym_proximity,*sym_point;

	static void CheckDriverVersion();
	static void InstallHandlers();
	
	static void timerfunc(void *);
	static Timer timer;

	static int maxtablets;
	static bool gTabletDriver475OrHigher;

	static pascal OSStatus HandleTabletProximity(EventHandlerCallRef inCallRef,EventRef inEvent, void * );
	static pascal OSStatus HandleTabletPointer(EventHandlerCallRef inCallRef,EventRef inEvent, void * );
	static pascal OSStatus HandleMouseEvents(EventHandlerCallRef inCallRef,EventRef inEvent, void * );
	
	void mg_index(int &ix);
	void mg_dims(AtomList &dims);
	
	FLEXT_CALLGET_I(mg_index);
	FLEXT_CALLGET_V(mg_dims);
};

flext::Timer wacom::timer;
bool wacom::gTabletDriver475OrHigher = false;
int wacom::maxtablets = 0;
const t_symbol *wacom::sym_proximity = NULL,*wacom::sym_point = NULL;

class Tablet
{
public:
	static Tablet *New(int index,wacom *obj)
	{
		Tablets::iterator it = tablets.find(index);
		Tablet *tb;
		if(it == tablets.end())
			tablets[index] = tb = new Tablet(index);
		else
			tb = it->second;
		tb->Push(obj);
		return tb;
	}

	static bool Free(Tablet *tb,wacom *obj)
	{
		if(!tb->Pop(obj)) {
			// no more references to tablet
			tablets.erase(tb->index);
			delete tb;
		}
	}
	
	int Index() const { return index; }
	
	int Left() const { return mappingRect.left; }
	int Right() const { return mappingRect.right; }
	int Top() const { return mappingRect.top; }
	int Bottom() const { return mappingRect.bottom; }
	
	static void Send(const TabletProximityRec &msg)
	{
		int index = msg.systemTabletID;
		
		if(msg.enterProximity) {
			// remember deviceID
			devices[msg.deviceID] = index;
		}
		else {
			// clear deviceID
			Devices::iterator dit = devices.find(msg.deviceID);
			if(dit == devices.end()) return; // device already out of reach
			devices.erase(dit);
		}
		
		FLEXT_ASSERT(tablets.find(index) != tablets.end());
		Tablet *t = tablets[index];
		for(Objects::iterator it = t->objects.begin(); it != t->objects.end(); ++it)
			(*it)->CbProximity(msg);
	}

	static void Send(const TabletPointRec &msg)
	{
		// find device in proximity map
		FLEXT_ASSERT(devices.find(msg.deviceID) != devices.end());
		int index = devices[msg.deviceID];

		Tablets::iterator tit = tablets.find(index);
		if(tit == tablets.end()) return ; // PD objects destroyed in the meantime
		Tablet *t = tit->second;
		for(Objects::iterator it = t->objects.begin(); it != t->objects.end(); ++it)
			(*it)->CbPoint(msg);
	}
			
protected:

	Tablet(int ix)
		: index(ix)
	{
#ifdef FLEXT_DEBUG
		post("wacom: Allocating tablet %i",index);
#endif
	
		OSErr err = CreateWacomContextForTablet(index, &context);
		FLEXT_ASSERT(err == noErr);

		err = GetData_ofSize_ofType_ofContext_ForAttribute(&mappingRect,sizeof(mappingRect),typeLongRectangle,context,pContextMapTabletOutputArea);
		FLEXT_ASSERT(err == noErr);

		Boolean movesCursor = FALSE;  // detach tablet from cursor - TODO: let user decide
		err = SetData_ofSize_ofType_ofContext_ForAttribute(&movesCursor, sizeof(Boolean),typeBoolean, context,pContextMovesSystemCursor);
		FLEXT_ASSERT(err == noErr);
	}

	~Tablet()
	{
		FLEXT_ASSERT(objects.size() == 0);

#ifdef FLEXT_DEBUG
		post("wacom: Freeing tablet %i",index);
#endif

		DestoryWacomContext(context);
	}
	
	void Push(wacom *obj) 
	{
		FLEXT_ASSERT(objects.find(obj) == objects.end());
		objects.insert(obj);
	}
	
	bool Pop(wacom *obj)
	{
		FLEXT_ASSERT(objects.find(obj) != objects.end());
		objects.erase(obj);
		return objects.size() != 0;
	}
	
private:
	int index;
	UInt32 context;
	LongRect mappingRect;

	typedef std::map<int,Tablet *> Tablets;
	static Tablets tablets;
	
	typedef std::set<wacom *> Objects;
	Objects objects;

	// remembering a device that has proximity to a tablet
	typedef std::map<UInt16,int> Devices;
	static Devices devices;
};

Tablet::Tablets Tablet::tablets;
Tablet::Devices Tablet::devices;



wacom::wacom(int index)
	: tablet(NULL)
{
	if(index == 0) index = 1;
	if(index <= maxtablets)
		tablet = Tablet::New(index,this);
	else {
		error("Tablet index exceeds number of tablets");
		InitProblem();
	}
}

wacom::~wacom()
{
	if(tablet) Tablet::Free(tablet,this);
}

void wacom::mg_index(int &ix) { ix = tablet?tablet->Index():-1; }

void wacom::mg_dims(AtomList &dims) 
{ 
	if(tablet) { 
		dims(4); 
		SetInt(dims[0],tablet->Left());
		SetInt(dims[1],tablet->Right());
		SetInt(dims[2],tablet->Top());
		SetInt(dims[3],tablet->Bottom());
	}
}

void wacom::CheckDriverVersion()
{
	OSErr			err = noErr;
	ControlID	theControlID; // ID of a static text box
	TAEObject	theTabletDriverObject;
	NumVersion 	theVerData;
	char			theStr[25];	 // String to set the text box to.

	// Use the Wacom supplied Apple Event helper function to ask the tablet
	// driver for it's version.
	theTabletDriverObject.objectType = cWTDDriver;
	err = GetData_ofSize_ofType_ofTabletObject_ForAttribute(&theVerData,
																			sizeof(theVerData),
																			typeVersion,
																			&theTabletDriverObject,
																			pVersion);
	if(err == noErr)
	{
		// Set the version number string. 
		sprintf(theStr, "%d.%d.%d",
			 theVerData.majorRev,
			 (theVerData.minorAndBugRev/10),
			 theVerData.minorAndBugRev-(((int)(theVerData.minorAndBugRev/10)) * 10));
		
		gTabletDriver475OrHigher = false;
		if ( ( theVerData.majorRev > 4 ) ||
		 ((theVerData.majorRev >= 4) && (theVerData.minorAndBugRev >= 75)) )
		{
			// Set a global flag so that we know we can use 4.7.5 features.
			gTabletDriver475OrHigher = true;
		}
	}
	else
	{
		// Dang, this means that you are running a pre 4.7.5 driver, or
		// running on pre 10.2. That's a bummer.
		gTabletDriver475OrHigher = false;
		sprintf(theStr, "???");
	}

	UInt32 tablets;
	theTabletDriverObject.objectType = cWTDTablet;
	if(CountTabletObjects(&theTabletDriverObject,&tablets) == noErr) 
		maxtablets = tablets;
	
	post("Wacom driver version %s, %i tablets found",theStr,maxtablets);
}


pascal OSStatus wacom::HandleTabletProximity(EventHandlerCallRef inCallRef,EventRef inEvent, void * )
{
	TabletProximityRec msg;

	// Extract the Tablet Proximity reccord from the event.
	if(noErr == GetEventParameter(inEvent, kEventParamTabletProximityRec,
								  typeTabletProximityRec, NULL,
								  sizeof(msg),
								  NULL, (void *)&msg))
	{
		Tablet::Send(msg);

#if 0
		// Lets pick the vendorPointerType apart and fiqure out exactly
		// which transducer is being used. We should probably check make
		// sure that the vendor ID for this proximity event is Wacom's.
		// But hey, why would you run Wacom's sample code on another
		// vendor's tablet? ;)
		UpdateSpecificPointerType(theTabletRecord.vendorPointerType,
											theTabletRecord.tabletID, window);

		// Pick apart the capabilites field and display something a little
		// more readable.
		UpdateCapabilities(theTabletRecord.capabilityMask, window);
#endif
	}

	return noErr;
}

pascal OSStatus wacom::HandleTabletPointer(EventHandlerCallRef inCallRef,EventRef inEvent, void * )
{
	TabletPointRec msg;

	// Extract the tablet Pointer Event. If there is no Tablet Pointer data
	// in this event, then this call will return an error. Just ignore the
	// error and go on. This can occur when a proximity event is embedded in
	// a mouse event and you did not check the mouse event to see which type
	// of tablet event was embedded.
	if(noErr == GetEventParameter(inEvent, kEventParamTabletPointRec,
								  typeTabletPointRec, NULL,
								  sizeof(msg),
								  NULL, (void *)&msg))
	{
		Tablet::Send(msg);
	}
	return noErr;
}



//////////////////////////////////////////////////////////////////////////////
// HandleMouseEvents
//
//  This is a callback function that the Carbon Event Loop calls whenever the
//  window receives a Mouse Event. Look at the function
//  InstallMouseHandlers() below to see how this call back was installed.
//
//  This function extracts the mouse data from the event and updates the
//  static text boxes in the Mouse Event group of the window. It will also
//  display whether or not this mouse event contains an embedded tablet event.
//  If this mouse event does contain an embedded mouse event, this function
//  will call the tablet pointer or proximity event handler.
//
//////////////////////////////////////////////////////////////////////////////
pascal OSStatus wacom::HandleMouseEvents(EventHandlerCallRef inCallRef,EventRef inEvent, void *userData )
{
#if 0
	HIPoint				aPoint;
	EventMouseButton	aButton;
	char					theStr[25];	 // String to set the text box to.

	// This is the signature of all the static text boxes in the Mouse
	// Event group of the window.
	theControlID.signature = kLastMouseEventClass;

	// Get the mouse location. Notice that we are using a HIPoint here.
	// HiPoints are cool because they are floating points. That means, that you
	// will get the exact location of the cursor down to about 4 decimal places.
	// So if you want sub pixel information, you can use this instead of looking
	// at the absX and absY tablet data and converting it to screen cooridinates.
	if(noErr == GetEventParameter(inEvent, kEventParamMouseLocation,
											typeHIPoint, NULL, sizeof(HIPoint),
											NULL, (void *)&aPoint))
	{
		theControlID.id = kstxtLocX;
		sprintf(theStr, "%5.3f", aPoint.x);
		UpdateStaticTextControl(window, &theControlID, theStr);
		
		theControlID.id = kstxtLocY;
		sprintf(theStr, "%5.3f", aPoint.y);
		UpdateStaticTextControl(window, &theControlID, theStr);
	}
	else 
	{
		// All mouse events should have a mouse loc. But if something goes wrong,
		// make sure the output is pretty.
		theControlID.id = kstxtLocX;
		UpdateStaticTextControl(window, &theControlID, "N/A");

		theControlID.id = kstxtLocY;
		UpdateStaticTextControl(window, &theControlID, "N/A");
	}

	// Get the delta between the last mouse loc and this one.
	// Delta locations aren't all that useful for tablets. But here it is if want it.
	if(noErr == GetEventParameter(inEvent, kEventParamMouseDelta, typeHIPoint,
								  NULL, sizeof(HIPoint), NULL,
								  (void *)&aPoint))
	{
		theControlID.id = kstxtDeltaX;
		sprintf(theStr, "%5.3f", aPoint.x);
		UpdateStaticTextControl(window, &theControlID, theStr);

		theControlID.id = kstxtDeltaY;
		sprintf(theStr, "%5.3f", aPoint.y);
		UpdateStaticTextControl(window, &theControlID, theStr);
	}
	else
	{
		// All mouse events should have a mouse delta. But if something goes wrong,
		// make sure the output is pretty.
		
		theControlID.id = kstxtDeltaX;
		UpdateStaticTextControl(window, &theControlID, "N/A");
		
		theControlID.id = kstxtDeltaY;
		UpdateStaticTextControl(window, &theControlID, "N/A");
	}

	// Get the state of the keyboard modifiers for this mouse event.
	if(noErr == GetEventParameter(inEvent, kEventParamKeyModifiers,
											typeUInt32, NULL, sizeof(UInt32),
											NULL, (void *)&anInt32))
	{
		theControlID.id = kstxtMods;
		sprintf(theStr, "%lu", anInt32);
		UpdateStaticTextControl(window, &theControlID, theStr);
	}
	else
	{
		// All mouse events should have modifiers. But if something goes wrong,
		// make sure the output is pretty.
		theControlID.id = kstxtMods;
		UpdateStaticTextControl(window, &theControlID, "N/A");
	}

	// Get the button that this event is occuring for.
	if(noErr == GetEventParameter(inEvent, kEventParamMouseButton,
											typeMouseButton, NULL,
											sizeof(EventMouseButton), NULL,
											(void *)&aButton))
	{
	theControlID.id = kstxtButtons;
	sprintf(theStr, "%u", aButton);
	UpdateStaticTextControl(window, &theControlID, theStr);
	}
	else
	{
		// There is no button data for Mouse Moves because no buttons are down
		// during a move. That would be drag. For mouse moves, the get will
		// fail, so make the output pretty.
		theControlID.id = kstxtButtons;
		UpdateStaticTextControl(window, &theControlID, "N/A");
	}

	// Get the chord of the mouse buttons. The mouse chord is a 32 bit field that
	// represents the state of all the buttons. (Up to 32 of them!) This is how you
	// check to see if multiple buttons are being pressed during a drag.
	if(noErr == GetEventParameter(inEvent, kEventParamMouseChord,
											typeUInt32, NULL, sizeof(UInt32),
											NULL, (void *)&anInt32))
	{
		theControlID.id = kstxtChord;
		sprintf(theStr, "%lu", anInt32);
		UpdateStaticTextControl(window, &theControlID, theStr);
	}
	else
	{
		// There is no chord data for Mouse Moves because no buttons are down
		// during a move. For mouse moves, the get will fail, so make the output pretty.
		theControlID.id = kstxtChord;
		UpdateStaticTextControl(window, &theControlID, "N/A");
	}
#endif

	// Determine if there is an embedded tablet event in this mouse event.
	UInt32	anInt32;
	if(noErr == GetEventParameter(inEvent, kEventParamTabletEventType,
											typeUInt32, NULL, sizeof(UInt32),
											NULL, (void *)&anInt32))
	{
		// Yes there is one!

		// Embedded tablet events can either be a proximity or pointer event.
		if(anInt32 == kEventTabletPoint)
		{
			// Send the event off to the tablet pointer event handler.
			HandleTabletPointer(inCallRef, inEvent, userData);
		}
		else
		{
			// You can ignore embedded proximity events if running tablet driver version
			// 4.7.5 or higher, because the proximity event will also be sent as a pure
			// tablet proximity event, and we hanve a Handler installed for pure
			// proximity events.
			if ( !gTabletDriver475OrHigher )
			{
				// !@#$, old driver. Manually send the Proximity event to the right place.
				HandleTabletProximity(inCallRef, inEvent, userData);
			}
		}
	}

   return eventNotHandledErr;
}


void wacom::InstallHandlers()
{
	OSStatus				status;
	// These are the mouse events we want to listen to because they may contain
	// embedded tablet events. If you do not care about tablet data unless a
	// button (or tip) is pressed, and you are using Wacom's Tablet Driver
	// version 4.7.5 or higher, then you can remove the kEventMouseMoved entry.
	// If you are using an older tablet driver then you must listen to all mouse
	// events to get proximity events. In this case, I want to demonstrate getting
	// tablet data from all mouse events, so we am going to listen for
	// kEventMouseMoved events as well.
	EventTypeSpec		mouseEvents[4] = {{ kEventClassMouse, kEventMouseDown		},
													{ kEventClassMouse, kEventMouseUp      },
													{ kEventClassMouse, kEventMouseDragged },
													{ kEventClassMouse, kEventMouseMoved   } };

	// These are pure tablet events. That is, they are not embedded in a mouse event.
	EventTypeSpec	tabletPointerEvent = { kEventClassTablet, kEventTabletPointer };
	EventTypeSpec	tabletProximityEvent = { kEventClassTablet, kEventTabletProximity };

	EventTargetRef target = GetEventMonitorTarget();

	// Install the mouse event handlers
	status = InstallEventHandler(target,
													NewEventHandlerUPP( HandleMouseEvents ),
													sizeof(mouseEvents)/sizeof(EventTypeSpec),
													mouseEvents, NULL, NULL	);
	assert(status == noErr);
	
	// Install the pure Tablet Pointer Event Handler. Generally, you do not need
	// to worry about pure tablet pointer events unless you are doing dual handed
	// input. That is, two transducers on the tablet at the same time.
	status = InstallEventHandler(target,
													NewEventHandlerUPP(HandleTabletPointer),
													1, &tabletPointerEvent, NULL, NULL );
	assert(status == noErr);

	// Install the pure Proximity Event Handler. If you are running 4.7.5 or higher
	// tablet driver, then all proximity events will be generated as pure tablet
	// proximity events caught with this handler. Otherwise, proximity events are
	// embedded in the mouse mouse events.
	status = InstallEventHandler(target,
													NewEventHandlerUPP( HandleTabletProximity ),
													1, &tabletProximityEvent, NULL, NULL	);
	assert(status == noErr);

}

void wacom::timerfunc(void *)
{
	EventTargetRef theTarget = GetEventDispatcherTarget();
	EventTimeout wait = 0; // in seconds
 
    for(;;) {
		EventRef theEvent;
		OSStatus ret = ReceiveNextEvent(0, NULL,wait,true,&theEvent);
		if(ret != noErr) break;

		SendEventToEventTarget (theEvent, theTarget);
		ReleaseEvent(theEvent);
	}
}

void wacom::Setup(t_classid c)
{
	post("wacom - (c) Thomas Grill, 2006-2008");
	CheckDriverVersion();

	timer.SetCallback(timerfunc);
	timer.Periodic(EVTIME);  // start timer

	InstallHandlers();
	
	sym_proximity = MakeSymbol("proximity");
	sym_point = MakeSymbol("point");
	
	FLEXT_CADDATTR_GET(c,"index",mg_index);
	FLEXT_CADDATTR_GET(c,"dims",mg_dims);
}

FLEXT_NEW_1("wacom",wacom,int0)
