#ifndef __AEffEditor__
#define __AEffEditor__

class AudioEffect;

struct ERect
{
	short top;
	short left;
	short bottom;
	short right;
};

class AEffEditor
{
public:
	AEffEditor (AudioEffect *effect) {this->effect = effect; updateFlag = 0; }
	virtual ~AEffEditor() {}

	virtual long getRect(ERect **rect) {*rect = 0; return 0;}
	virtual long open(void *ptr) {systemWindow = ptr; return 0;}
	virtual void close() {}
	virtual void idle() { if(updateFlag) {updateFlag = 0; update();} }
	
#if MAC
	virtual void draw(ERect *rect) {rect = rect;}
	virtual long mouse(long x, long y) {x = x; y = y; return 0;}
	virtual long key(long keyCode) {keyCode = keyCode; return 0;}
	virtual void top() {}
	virtual void sleep() {}
#endif
	virtual void update() {}
	virtual void postUpdate() {updateFlag = 1;}
	
protected:
	AEffEditor () {};

	AudioEffect *effect;
	void *systemWindow;
	long updateFlag;
};

#endif

