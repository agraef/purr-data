#ifdef PD
#include "m_pd.h"
#else
#include "ext.h"
#endif
#include "hidio.h"

#define DEBUG(x)
//#define DEBUG(x) x 


/*==============================================================================
 * "codes" for elements
 *==============================================================================
 */

/* absolute axes (joysticks, gamepads, tablets, etc.) */
static char *absolute_strings[ABSOLUTE_ARRAY_MAX] = {
	"x","y","z","rx","ry","rz","slider","dial",
	"wheel","hatswitch","absolute_10","absolute_11","absolute_12","absolute_13",
	"absolute_14","absolute_15"
};

/* keys (keyboards, keypads) */
static char *key_strings[KEY_ARRAY_MAX] = {
	"key_0","errorrollover","postfail","errorundefined","a","b","c","d","e","f",
    "g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y",
    "z","1_key","2_key","3_key","4_key","5_key","6_key","7_key","8_key","9_key",
    "0_key","enter","escape","deleteorbackspace","tab","spacebar","hyphen",
    "equalsign","openbracket","closebracket","backslash","nonuspound",
    "semicolon","quote","graveaccentandtilde","comma","period","slash",
    "capslock","F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
    "printscreen","scrolllock","pause","insert","home","pageup","deleteforward",
    "end","pagedown","rightarrow","leftarrow","downarrow","uparrow",
    "keypad_numlock","keypad_slash","keypad_asterisk","keypad_hyphen",
    "keypad_plus","keypad_enter","keypad_1","keypad_2","keypad_3","keypad_4",
    "keypad_5","keypad_6","keypad_7","keypad_8","keypad_9","keypad_0",
    "keypad_period","nonusbackslash","application","power","keypad_equalsign",
    "F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
    "execute","help","menu","select","stop","again","undo","cut","copy","paste",
    "find","mute","volumeup","volumedown","lockingcapslock","lockingnumlock",
    "lockingscrolllock","keypad_comma","keypad_equalsignas400","international1",
    "international2","international3","international4","international5",
    "international6","international7","international8","international9","lang1",
    "lang2","lang3","lang4","lang5","lang6","lang7","lang8","lang9",
    "alternateerase","sysreqorattention","cancel","clear","prior","return",
    "separator","out","oper","clearoragain","crselorprops","exsel","key_165",
    "key_166","key_167","key_168","key_169","key_170","key_171","key_172",
    "key_173","key_174","key_175","key_176","key_177","key_178","key_179",
    "key_180","key_181","key_182","key_183","key_184","key_185","key_186",
    "key_187","key_188","key_189","key_190","key_191","key_192","key_193",
    "key_194","key_195","key_196","key_197","key_198","key_199","key_200",
    "key_201","key_202","key_203","key_204","key_205","key_206","key_207",
    "key_208","key_209","key_210","key_211","key_212","key_213","key_214",
    "key_215","key_216","key_217","key_218","key_219","key_220","key_221",
    "key_222","key_223","leftcontrol","leftshift","leftalt","leftgui",
    "rightcontrol","rightshift","rightalt","rightgui","key_232","key_233",
    "key_234","key_235","key_236","key_237","key_238","key_239","key_240",
    "key_241","key_242","key_243","key_244","key_245","key_246","key_247",
    "key_248","key_249","key_250","key_251","key_252","key_253","key_254",
    "key_255"
};


/* LEDs (keyboards, gamepads, etc.) */
static char *led_strings[LED_ARRAY_MAX] = {
	"numlock","capslock","scrolllock","compose","kana","power","shift",
	"donotdisturb","mute","toneenable","highcutfilter","lowcutfilter",
	"equalizerenable","soundfieldon","surroundon","repeat","stereo",
	"samplingratedetect","spinning","cav","clv","recordingformatdetect",
	"offhook","ring","messagewaiting","datamode","batteryoperation","batteryok",
	"batterylow","speaker","headset","hold","microphone","coverage","nightmode",
	"sendcalls","callpickup","conference","standby","cameraon","cameraoff",
	"online","offline","busy","ready","paperout","paperjam","remote","forward",
	"reverse","stop","rewind","fastforward","play","pause","record","error",
	"usage","usageinuseindicator","usagemultimodeindicator","indicatoron",
	"indicatorflash","indicatorslowblink","indicatorfastblink","indicatoroff",
	"flashontime","slowblinkontime","slowblinkofftime","fastblinkontime",
	"fastblinkofftime","usageindicatorcolor","indicatorred","indicatorgreen",
	"indicatoramber","genericindicator","systemsuspend","externalpowerconnected"
};


/* PID, Physical Interface Devices (force feedback joysticks, mice, etc.) */
static char *pid_strings[PID_ARRAY_MAX] = {
	"pid_0","physicalinterfacedevice","pid_2","pid_3","pid_4","pid_5","pid_6","pid_7",
	"pid_8","pid_9","pid_10","pid_11","pid_12","pid_13","pid_14","pid_15",
	"pid_16","pid_17","pid_18","pid_19","pid_20","pid_21","pid_22","pid_23",
	"pid_24","pid_25","pid_26","pid_27","pid_28","pid_29","pid_30","pid_31",
	"normal","seteffectreport","effectblockindex","paramblockoffset","rom_flag",
	"effecttype","constantforce","ramp","customforcedata","pid_41","pid_42",
	"pid_43","pid_44","pid_45","pid_46","pid_47","square","sine","triangle",
	"sawtoothup","sawtoothdown","pid_53","pid_54","pid_55","pid_56","pid_57",
	"pid_58","pid_59","pid_60","pid_61","pid_62","pid_63","spring","damper",
	"inertia","friction","pid_68","pid_69","pid_70","pid_71","pid_72","pid_73",
	"pid_74","pid_75","pid_76","pid_77","pid_78","pid_79","duration",
	"sampleperiod","gain","triggerbutton","triggerrepeatinterval","axesenable",
	"directionenable","direction","typespecificblockoffset","blocktype",
	"setenvelopereport","attacklevel","attacktime","fadelevel","fadetime",
	"setconditionreport","cp_offset","positivecoefficient",
	"negativecoefficient","positivesaturation","negativesaturation","deadband",
	"downloadforcesample","isochcustomforceenable","customforcedatareport",
	"customforcedata","customforcevendordefineddata","setcustomforcereport",
	"customforcedataoffset","samplecount","setperiodicreport","offset",
	"magnitude","phase","period","setconstantforcereport","setrampforcereport",
	"rampstart","rampend","effectoperationreport","effectoperation",
	"opeffectstart","opeffectstartsolo","opeffectstop","loopcount",
	"devicegainreport","devicegain","poolreport","ram_poolsize",
	"rom_poolsize","rom_effectblockcount","simultaneouseffectsmax",
	"poolalignment","poolmovereport","movesource","movedestination",
	"movelength","blockloadreport","pid_138","blockloadstatus",
	"blockloadsuccess","blockloadfull","blockloaderror","blockhandle",
	"blockfreereport","typespecificblockhandle","statereport","pid_147",
	"effectplaying","devicecontrolreport","devicecontrol","dc_enableactuators",
	"dc_disableactuators","dc_stopalleffects","dc_devicereset","dc_devicepause",
	"dc_devicecontinue","pid_157","pid_158","devicepaused","actuatorsenabled",
	"pid_161","pid_162","pid_163","safetyswitch","actuatoroverrideswitch",
	"actuatorpower","startdelay","parameterblocksize","devicemanagedpool",
	"sharedparameterblocks","createneweffectreport","ram_poolavailable",
	"pid_173","pid_174","pid_175","pid_176","pid_177","pid_178","pid_179",
	"pid_180","pid_181","pid_182","pid_183","pid_184","pid_185","pid_186",
	"pid_187","pid_188","pid_189","pid_190","pid_191","pid_192","pid_193",
	"pid_194","pid_195","pid_196","pid_197","pid_198","pid_199","pid_200",
	"pid_201","pid_202","pid_203","pid_204","pid_205","pid_206","pid_207",
	"pid_208","pid_209","pid_210","pid_211","pid_212","pid_213","pid_214",
	"pid_215","pid_216","pid_217","pid_218","pid_219","pid_220","pid_221",
	"pid_222","pid_223","pid_224","pid_225","pid_226","pid_227","pid_228",
	"pid_229","pid_230","pid_231","pid_232","pid_233","pid_234","pid_235",
	"pid_236","pid_237","pid_238","pid_239","pid_240","pid_241","pid_242",
	"pid_243","pid_244","pid_245","pid_246","pid_247","pid_248","pid_249",
	"pid_250","pid_251","pid_252","pid_253","pid_254","pid_255"
};


/* relative axes (mice) */
static char *relative_strings[RELATIVE_ARRAY_MAX] = {
	"x","y","z","rx","ry","rz",
	"hwheel","dial","wheel","misc","relative_10","relative_11",
	"relative_12","relative_13","relative_14","relative_15"
};

/*==============================================================================
 * conversion functions
 *==============================================================================
 */

static void generate_button_symbols(t_symbol *symbols[], unsigned int size)
{
	unsigned int i;
	char string_buffer[MAXPDSTRING];

	for(i = 0; i < size; ++i) 
	{
		sprintf(string_buffer,"button_%d",i);
		symbols[i] = gensym(string_buffer);
	}
}

static void generate_symbols_from_strings(t_symbol *symbols[], char *strings[], 
										  unsigned int size)
{
	unsigned int i;

	for(i = 0; i < size; ++i) symbols[i] = gensym(strings[i]);
}


void generate_event_symbols()
{
	DEBUG(post("generate_event_symbols"););
	generate_button_symbols(button_symbols, BUTTON_ARRAY_MAX);
	DEBUG(post("button"););
	generate_symbols_from_strings(absolute_symbols, absolute_strings, 
								  ABSOLUTE_ARRAY_MAX);
	DEBUG(post("absolute"););
	generate_symbols_from_strings(key_symbols, key_strings, 
								  KEY_ARRAY_MAX);
	DEBUG(post("key"););
	generate_symbols_from_strings(led_symbols, led_strings, 
								  LED_ARRAY_MAX);
	DEBUG(post("led"););
	generate_symbols_from_strings(pid_symbols, pid_strings, 
								  PID_ARRAY_MAX);
	DEBUG(post("pid"););
	generate_symbols_from_strings(relative_symbols, relative_strings, 
								  RELATIVE_ARRAY_MAX);
	DEBUG(post("relative %s",relative_symbols[4]->s_name););
	DEBUG(post("generate_event_symbols"););
}

void generate_type_symbols()
{
	ps_absolute = gensym("absolute");
	ps_button = gensym("button");
	ps_key = gensym("key");
	ps_led = gensym("led");
	ps_pid = gensym("pid");
	ps_relative = gensym("relative");
}
