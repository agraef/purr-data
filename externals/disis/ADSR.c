#include "ADSR.h"

t_float stk_ADSR_tick(t_stk_ADSR *x)
{
    switch (x->state)
    {
        case ATTACK:
            x->value += x->attackRate;
            if (x->value >= x->target)
            {
                x->value = x->target;
                x->target = x->sustainLevel;
                x->state = DECAY;
            }
            break;

        case DECAY:
            if (x->value > x->sustainLevel)
            {
                x->value -= x->decayRate;
                if (x->value <= x->sustainLevel)
                {
                    x->value = x->sustainLevel;
                    x->state = SUSTAIN;
                }
            }
            else
            {
                x->value += x->decayRate; // attack target < sustain level
                if (x->value >= x->sustainLevel)
                {
                    x->value = x->sustainLevel;
                    x->state = SUSTAIN;
                }
            }
            break;

        case RELEASE:
            x->value -= x->releaseRate;
            if (x->value <= 0.0)
            {
                x->value = 0.0;
                x->state = IDLE;
            }
    }
    return x->value;
}

void stk_ADSR_init(t_stk_ADSR *x)
{
  x->target = 0.0;
  x->value = 0.0;
  x->attackRate = 0.001;
  x->decayRate = 0.001;
  x->releaseRate = 0.005;
  x->releaseTime = -1.0;
  x->sustainLevel = 0.5;
  x->state = IDLE;
  x->sampleRate = 44100;
}

t_env_state stk_ADSR_getState(t_stk_ADSR *x)
{
    return x->state;
}

void stk_ADSR_sampleRateChanged(t_stk_ADSR *x, t_float newRate,
    t_float oldRate)
{
    x->attackRate = oldRate * x->attackRate / newRate;
    x->decayRate = oldRate * x->decayRate / newRate;
    x->releaseRate = oldRate * x->releaseRate / newRate;
}

void stk_ADSR_setSampleRate(t_stk_ADSR *x, t_float newRate)
{
    x->sampleRate = newRate;
}

void stk_ADSR_keyOn(t_stk_ADSR *x)
{
    if (x->target <= 0.0) x->target = 1.0;
    x->state = ATTACK;
}

void stk_ADSR_keyOff(t_stk_ADSR *x)
{
    x->target = 0.0;
    x->state = RELEASE;

    // FIXED October 2010 - Nick Donaldson
    // Need to make release rate relative to current value!!
    // Only update if we have set a TIME rather than a RATE,
    // in which case releaseTime will be -1
    if (x->releaseTime > 0.0)
	  x->releaseRate = x->value / (x->releaseTime * x->sampleRate);
}

void stk_ADSR_setAttackRate(t_stk_ADSR *x, t_float rate)
{
    if (rate < 0.0)
        fprintf(stderr, "stk_ADSR_setAttackRate: argument must be >= 0.0!");
    x->attackRate = rate;
}

void stk_ADSR_setAttackTarget(t_stk_ADSR *x, t_float target)
{
    if (target < 0.0)
    {
        fprintf(stderr, "ADSR::setAttackTarget: negative target not allowed!");
    }
    x->target = target;
}

void stk_ADSR_setDecayRate(t_stk_ADSR *x, t_float rate)
{
    if (rate < 0.0)
        fprintf(stderr, "ADSR::setDecayRate: negative rates not allowed!");
    x->decayRate = rate;
}

void stk_ADSR_setSustainLevel(t_stk_ADSR *x, t_float level)
{
    if (level < 0.0)
        fprintf(stderr, "ADSR::setSustainLevel: negative level not allowed!");
    x->sustainLevel = level;
}

void stk_ADSR_setReleaseRate(t_stk_ADSR *x, t_float rate)
{
    if (rate < 0.0)
        fprintf(stderr, "ADSR::setReleaseRate: negative rates not allowed!");
    x->releaseRate = rate;

    // Set to negative value so we don't update the release rate on keyOff()
    x->releaseTime = -1.0;
}

void stk_ADSR_setAttackTime(t_stk_ADSR *x, t_float time)
{
    if (time <= 0.0)
        fprintf(stderr,
            "ADSR::setAttackTime: negative or zero times not allowed!");
    x->attackRate = 1.0 / (time * x->sampleRate);
}

void stk_ADSR_setDecayTime(t_stk_ADSR *x, t_float time)
{
    if (time <= 0.0)
        fprintf(stderr,
            "ADSR::setDecayTime: negative or zero times not allowed!");
    x->decayRate = (1.0 - x->sustainLevel) / (time * x->sampleRate);
}

void stk_ADSR_setReleaseTime(t_stk_ADSR *x, t_float time)
{
    if (time <= 0.0)
        fprintf(stderr,
            "ADSR::setReleaseTime: negative or zero times not allowed!");
    x->releaseRate = x->sustainLevel / (time * x->sampleRate);
    x->releaseTime = time;
}

void stk_ADSR_setAllTimes(t_stk_ADSR *x, t_float aTime, t_float dTime,
    t_float sLevel, t_float rTime)
{
    stk_ADSR_setAttackTime(x, aTime);
    stk_ADSR_setSustainLevel(x, sLevel);
    stk_ADSR_setDecayTime(x, dTime);
    stk_ADSR_setReleaseTime(x, rTime);
}

void stk_ADSR_setTarget(t_stk_ADSR *x, t_float target)
{
    if (target < 0.0)
        fprintf(stderr, "ADSR::setTarget: negative target not allowed!");
    x->target = target;

    stk_ADSR_setSustainLevel(x, x->target);
    if (x->value < x->target) x->state = ATTACK;
    if (x->value > x->target) x->state = DECAY;
}

void stk_ADSR_setValue(t_stk_ADSR *x, t_float value)
{
    x->state = SUSTAIN;
    x->target = value;
    x->value = value;
    stk_ADSR_setSustainLevel(x, value);
}
