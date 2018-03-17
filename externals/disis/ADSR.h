#include "m_pd.h"

/* port of stk's ADSR class to C */

/* original class notes */

/***************************************************/
/*! \class ADSR
    \brief STK ADSR envelope class.
    This class implements a traditional ADSR (Attack, Decay, Sustain,
    Release) envelope.  It responds to simple keyOn and keyOff
    messages, keeping track of its state.  The \e state = ADSR::IDLE
    before being triggered and after the envelope value reaches 0.0 in
    the ADSR::RELEASE state.  All rate, target and level settings must
    be non-negative.  All time settings are in seconds and must be
    positive.
    by Perry R. Cook and Gary P. Scavone, 1995--2017.
*/
/***************************************************/

/* ADSR envelope states. */
typedef enum {
    ATTACK,   /* Attack */
    DECAY,    /* Decay */
    SUSTAIN,  /* Sustain */
    RELEASE,  /* Release */
    IDLE      /* Before attack / after release */
} t_env_state;

typedef struct _stk_ADSR {
    t_env_state state;
    t_float value;
    t_float target;
    t_float attackRate;
    t_float decayRate;
    t_float releaseRate;
    t_float releaseTime;
    t_float sustainLevel;
    // Currently setting this is dsp_add routine...
    t_float sampleRate;
} t_stk_ADSR;

/* initialize the struct members to sane values */
void stk_ADSR_init(t_stk_ADSR *x);

/* set the sample rate */
void stk_ADSR_setSampleRate(t_stk_ADSR *x, t_float newRate);

/* Set target = 1, state = ATTACK. */
void stk_ADSR_keyOn(t_stk_ADSR *x);

/* Set target = 0, state = RELEASE. */
void stk_ADSR_keyOff(t_stk_ADSR *x);

/* Set the attack rate (gain / sample). */
void stk_ADSR_setAttackRate(t_stk_ADSR *x, t_float rate);

/* Set the target value for the attack (default = 1.0). */
void stk_ADSR_setAttackTarget(t_stk_ADSR *x, t_float target);

/* Set the decay rate (gain / sample). */
void stk_ADSR_setDecayRate(t_stk_ADSR *x, t_float rate);

/* Set the sustain level. */
void stk_ADSR_setSustainLevel(t_stk_ADSR *x, t_float level);

/* Set the release rate (gain / sample). */
void stk_ADSR_setReleaseRate(t_stk_ADSR *x, t_float rate);

/* Set the attack rate based on a time duration (seconds). */
void stk_ADSR_setAttackTime(t_stk_ADSR *x, t_float time);

/* Set the decay rate based on a time duration (seconds). */
void stk_ADSR_setDecayTime(t_stk_ADSR *x, t_float time);

/* Set the release rate based on a time duration (seconds). */
void stk_ADSR_setReleaseTime(t_stk_ADSR *x, t_float time);

/* Set sustain level and attack, decay, and release time durations (seconds). */
void stk_ADSR_setAllTimes(t_stk_ADSR *x, t_float aTime, t_float dTime,
    t_float sLevel, t_float rTime);

/* Set a sustain target value and attack or decay from current value
    to target. */
void stk_ADSR_setTarget(t_stk_ADSR *x, t_float target);

/* Return the current envelope state (ATTACK, DECAY, SUSTAIN, RELEASE, IDLE).*/
t_env_state stk_ADSR_getState(t_stk_ADSR *x);

/* Set to state = ADSR::SUSTAIN with current and target values of value. */
void stk_ADSR_setValue(t_stk_ADSR *x, t_float value);

/* Compute and return one output sample. */
t_float stk_ADSR_tick(t_stk_ADSR *x);
