/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3669 $
$LastChangedDate: 2009-03-05 18:34:39 -0500 (Thu, 05 Mar 2009) $
$LastChangedBy: thomas $
*/

/*! \file flsimd.cpp
    \brief flext SIMD support functions

    If FLEXT_USE_SIMD is defined at compilation, SIMD instructions are used wherever feasible.
    If used with MSVC++ 6 the "Processor Pack" must be installed.

    If FLEXT_USE_IPP is defined the Intel Performance Package is used.
*/

#include "flext.h"
#include <cstring>

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
#endif

#ifdef FLEXT_USE_IPP
#include <ipps.h>
#endif

#ifdef FLEXT_USE_SIMD
    #ifdef _MSC_VER
        // include MSVC SIMD header files
        #include <mmintrin.h> // MMX
        #include <xmmintrin.h> // SSE
        #include <emmintrin.h> // SSE2
        #include <mm3dnow.h> // 3DNow!
    #elif defined(__APPLE__)  && defined(__VEC__)
        #ifdef __MWERKS__
            #if FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH
                #include <sys/sysctl.h> 
                #include <vDSP.h>
            #else
                #include <Gestalt.h> 
            #endif
    
            #pragma altivec_model on

            #include <altivec.h>
            #include <vectorOps.h>
        #elif defined(__GNUC__)
            #include <sys/sysctl.h> 
            #include <vecLib/vecLib.h>
        #endif
    #endif

#endif // FLEXT_USE_SIMD

#include "flpushns.h"

static unsigned long setsimdcaps();

/*! \brief Holds SIMD capability flags
    \internal
*/
unsigned long flext::simdcaps = setsimdcaps();

unsigned long flext::GetSIMDCapabilities() { return simdcaps; }


#ifdef FLEXT_USE_SIMD

#if FLEXT_CPU == FLEXT_CPU_IA32 || FLEXT_CPU == FLEXT_CPU_X86_64

#define _CPU_FEATURE_MMX    0x0001
#define _CPU_FEATURE_SSE    0x0002
#define _CPU_FEATURE_SSE2   0x0004
#define _CPU_FEATURE_3DNOW  0x0008

typedef struct _processor_info {
    int family;                         // family of the processor
                                        // e.g. 6 = Pentium-Pro architecture
    int model;                          // model of processor
                                        // e.g. 1 = Pentium-Pro for family = 6
    int stepping;                       // processor revision number
    int feature;                        // processor feature
                                        // (same as return value from _cpuid)
    int os_support;                     // does OS Support the feature?
    int checks;                         // mask of checked bits in feature
                                        // and os_support fields
} _p_info;

// These are the bit flags that get set on calling cpuid
// with register eax set to 1
#define _MMX_FEATURE_BIT        0x00800000
#define _SSE_FEATURE_BIT        0x02000000
#define _SSE2_FEATURE_BIT       0x04000000

// This bit is set when cpuid is called with
// register set to 80000001h (only applicable to AMD)
#define _3DNOW_FEATURE_BIT      0x80000000

#ifdef _MSC_VER
static int IsCPUID()
{
    __try {
        _asm {
            xor eax, eax
            cpuid
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return 1;
}

static int _os_support(int feature)
{
    __try {
        switch (feature) {
        case _CPU_FEATURE_SSE:
            __asm {
                xorps xmm0, xmm0        // executing SSE instruction
            }
            break;
        case _CPU_FEATURE_SSE2:
            __asm {
                xorpd xmm0, xmm0        // executing SSE2 instruction
            }
            break;
        case _CPU_FEATURE_3DNOW:
            __asm {
                pfrcp mm0, mm0          // executing 3DNow! instruction
                emms
            }
            break;
        case _CPU_FEATURE_MMX:
            __asm {
                pxor mm0, mm0           // executing MMX instruction
                emms
            }
            break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
            return 0;
        }
        return 0;
    }
    return 1;
}

static int _cpuid (_p_info *pinfo)
{
    DWORD dwStandard = 0;
    DWORD dwFeature = 0;
    DWORD dwMax = 0;
    DWORD dwExt = 0;
    int feature = 0;
    int os_support = 0;
    union {
        struct {
            DWORD dw0;
            DWORD dw1;
            DWORD dw2;
        } s;
    } Ident;

    if (!IsCPUID()) {
        return 0;
    }

    _asm {
        push ebx
        push ecx
        push edx

        // get the vendor string
        xor eax, eax
        cpuid
        mov dwMax, eax
        mov Ident.s.dw0, ebx
        mov Ident.s.dw1, edx
        mov Ident.s.dw2, ecx

        // get the Standard bits
        mov eax, 1
        cpuid
        mov dwStandard, eax
        mov dwFeature, edx

        // get AMD-specials
        mov eax, 80000000h
        cpuid
        cmp eax, 80000000h
        jc notamd
        mov eax, 80000001h
        cpuid
        mov dwExt, edx

notamd:
        pop ecx
        pop ebx
        pop edx
    }

    if (dwFeature & _MMX_FEATURE_BIT) {
        feature |= _CPU_FEATURE_MMX;
        if (_os_support(_CPU_FEATURE_MMX))
            os_support |= _CPU_FEATURE_MMX;
    }
    if (dwExt & _3DNOW_FEATURE_BIT) {
        feature |= _CPU_FEATURE_3DNOW;
        if (_os_support(_CPU_FEATURE_3DNOW))
            os_support |= _CPU_FEATURE_3DNOW;
    }
    if (dwFeature & _SSE_FEATURE_BIT) {
        feature |= _CPU_FEATURE_SSE;
        if (_os_support(_CPU_FEATURE_SSE))
            os_support |= _CPU_FEATURE_SSE;
    }
    if (dwFeature & _SSE2_FEATURE_BIT) {
        feature |= _CPU_FEATURE_SSE2;
        if (_os_support(_CPU_FEATURE_SSE2))
            os_support |= _CPU_FEATURE_SSE2;
    }

    if (pinfo) {
        memset(pinfo, 0, sizeof(_p_info));

        pinfo->os_support = os_support;
        pinfo->feature = feature;
        pinfo->family = (dwStandard >> 8) & 0xF; // retrieve family
        if (pinfo->family == 15) {               // retrieve extended family
            pinfo->family |= (dwStandard >> 16) & 0xFF0;
        }
        pinfo->model = (dwStandard >> 4) & 0xF;  // retrieve model
        if (pinfo->model == 15) {                // retrieve extended model
            pinfo->model |= (dwStandard >> 12) & 0xF;
        }
        pinfo->stepping = (dwStandard) & 0xF;    // retrieve stepping

        pinfo->checks = _CPU_FEATURE_MMX |
                        _CPU_FEATURE_SSE |
                        _CPU_FEATURE_SSE2 |
                        _CPU_FEATURE_3DNOW;
    }

    return feature;
}

inline bool IsVectorAligned(const void *where) 
{
    return (reinterpret_cast<size_t>(where)&(__alignof(__m128)-1)) == 0;
}

inline bool VectorsAligned(const void *v1,const void *v2) 
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2))
        &(__alignof(__m128)-1)
    ) == 0; 
}

inline bool VectorsAligned(const void *v1,const void *v2,const void *v3) 
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2)|reinterpret_cast<size_t>(v3))
        &(__alignof(__m128)-1)
    ) == 0; 
}

inline bool VectorsAligned(const void *v1,const void *v2,const void *v3,const void *v4)
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2)|reinterpret_cast<size_t>(v3)|reinterpret_cast<size_t>(v4))
        &(__alignof(__m128)-1)
    ) == 0; 
}

#else
// not MSVC
static int _cpuid (_p_info *pinfo)
{
    if(pinfo) memset(pinfo,0,sizeof *pinfo);
    return 0;
}
#endif

#endif


/*! \brief Determine SIMD capabilities
    \internal
*/
static unsigned long setsimdcaps()
{
    unsigned long simdflags = flext::simd_none;
#if FLEXT_CPU == FLEXT_CPU_IA32 || FLEXT_CPU == FLEXT_CPU_X86_64
    _p_info cpuinfo;
    int feature = _cpuid(&cpuinfo);
    if(cpuinfo.os_support&_CPU_FEATURE_MMX) simdflags += flext::simd_mmx;
    if(cpuinfo.os_support&_CPU_FEATURE_3DNOW) simdflags += flext::simd_3dnow;
    if(cpuinfo.os_support&_CPU_FEATURE_SSE) simdflags += flext::simd_sse;
    if(cpuinfo.os_support&_CPU_FEATURE_SSE2) simdflags += flext::simd_sse2;
#elif defined(__APPLE__) && defined(__VEC__) 
    #if FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH

    int selectors[2] = { CTL_HW, HW_VECTORUNIT }; 
    int hasVectorUnit = 0; 
    size_t length = sizeof(hasVectorUnit); 
    int error = sysctl(selectors, 2, &hasVectorUnit, &length, NULL, 0); 

    if(!error && hasVectorUnit != 0) simdflags += flext::simd_altivec; 
        
    #else

    long cpuAttributes; 
    Boolean hasAltiVec = false; 
    OSErr err = Gestalt( gestaltPowerPCProcessorFeatures, &cpuAttributes ); 

    if( noErr == err ) 
    if(( 1 << gestaltPowerPCHasVectorInstructions) & cpuAttributes) simdflags += flext::simd_altivec; 

    #endif
#endif
    return simdflags;
}


#if (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64)  && defined(__VEC__)

/* functions for misaligned vector data - taken from the Altivec tutorial of Ian Ollmann, Ph.D. */

//! Load a vector from an unaligned location in memory
inline vector unsigned char LoadUnaligned( vector unsigned char *v )
{
    vector unsigned char permuteVector = vec_lvsl( 0, (int*) v );
    vector unsigned char low = vec_ld( 0, v );
    vector unsigned char high = vec_ld( 15, v );
    return vec_perm( low, high, permuteVector );
}

/*
//! Store a vector to an unaligned location in memory
inline void StoreUnaligned( vector unsigned char v, vector unsigned char *where)
{
    // Load the surrounding area
    vector unsigned char low = vec_ld( 0, where );
    vector unsigned char high = vec_ld( 16, where );
    // Prepare the constants that we need
    vector unsigned char permuteVector = vec_lvsr( 0, (int*) where );

    vector unsigned char oxFF = (vector unsigned char)vec_splat_s8( -1 );
    vector unsigned char ox00 = (vector unsigned char)vec_splat_s8( 0 );
    // Make a mask for which parts of the vectors to swap out
    vector unsigned char mask = vec_perm( ox00, oxFF, permuteVector );
    // Right rotate our input data
    v = vec_perm( v, v, permuteVector );
    // Insert our data into the low and high vectors
    low = vec_sel( v, low, mask );
    high = vec_sel( high, v, mask );
    // Store the two aligned result vectors
    vec_st( low, 0, where );
    vec_st( high, 16, where );
}
*/

inline vector float LoadUnaligned(const float *v )
{
    return (vector float)LoadUnaligned((vector unsigned char *)v);
}

/*
inline void StoreUnaligned( vector float v,float *where)
{
    return StoreUnaligned((vector unsigned char)v,(vector unsigned char *)where);
}
*/

inline bool IsVectorAligned(const void *where) 
{
    return (reinterpret_cast<size_t>(where)&(sizeof(vector float)-1)) == 0; 
}

inline bool VectorsAligned(const void *v1,const void *v2) 
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2))
        &(sizeof(vector float)-1)
    ) == 0; 
}

inline bool VectorsAligned(const void *v1,const void *v2,const void *v3) 
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2)|reinterpret_cast<size_t>(v3))
        &(sizeof(vector float)-1)
    ) == 0; 
}

inline bool VectorsAligned(const void *v1,const void *v2,const void *v3,const void *v4)
{
    return (
        (reinterpret_cast<size_t>(v1)|reinterpret_cast<size_t>(v2)|reinterpret_cast<size_t>(v3)|reinterpret_cast<size_t>(v4))
        &(sizeof(vector float)-1)
    ) == 0; 
}

inline vector float LoadValue(const float &f)
{
    return vec_splat(IsVectorAligned(&f)?vec_ld(0,(vector float *)&f):LoadUnaligned(&f),0);
}
#endif


#else // FLEXT_USE_SIMD
static unsigned long setsimdcaps() { return 0; }
#endif // FLEXT_USE_SIMD


void flext::CopySamples(t_sample *dst,const t_sample *src,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4)
        ippsCopy_32f((const float *)src,(float *)dst,cnt); 
    else if(sizeof(t_sample) == 8)
        ippsCopy_64f((const double *)src,(double *)dst,cnt); 
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision

        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,dword ptr [src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]
        }

        if(IsVectorAligned(src)) {
            if(IsVectorAligned(dst)) {
                // aligned src, aligned dst
                __asm {
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ecx,[n]
loopaa:
                    prefetcht0 [eax+64]
                    prefetcht0 [eax+96]
                    movaps  xmm0,xmmword ptr[eax]
                    movaps  xmmword ptr[edx],xmm0
                    movaps  xmm1,xmmword ptr[eax+4*4]
                    movaps  xmmword ptr[edx+4*4],xmm1
                    movaps  xmm2,xmmword ptr[eax+8*4]
                    movaps  xmmword ptr[edx+8*4],xmm2
                    movaps  xmm3,xmmword ptr[eax+12*4]
                    movaps  xmmword ptr[edx+12*4],xmm3

                    add     eax,16*4
                    add     edx,16*4
                    loop    loopaa
                }
            }
            else {
                // aligned src, unaligned dst
                __asm {
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ecx,[n]
loopau:
                    prefetcht0 [eax+64]
                    prefetcht0 [eax+96]
                    movaps  xmm0,xmmword ptr[eax]
                    movups  xmmword ptr[edx],xmm0
                    movaps  xmm1,xmmword ptr[eax+4*4]
                    movups  xmmword ptr[edx+4*4],xmm1
                    movaps  xmm2,xmmword ptr[eax+8*4]
                    movups  xmmword ptr[edx+8*4],xmm2
                    movaps  xmm3,xmmword ptr[eax+12*4]
                    movups  xmmword ptr[edx+12*4],xmm3

                    add     eax,16*4
                    add     edx,16*4
                    loop    loopau
                }
            }
        }
        else {
            if(IsVectorAligned(dst)) {
                // unaligned src, aligned dst
                __asm {
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ecx,[n]
loopua:
                    prefetcht0 [eax+64]
                    prefetcht0 [eax+96]
                    movups  xmm0,xmmword ptr[eax]
                    movaps  xmmword ptr[edx],xmm0
                    movups  xmm1,xmmword ptr[eax+4*4]
                    movaps  xmmword ptr[edx+4*4],xmm1
                    movups  xmm2,xmmword ptr[eax+8*4]
                    movaps  xmmword ptr[edx+8*4],xmm2
                    movups  xmm3,xmmword ptr[eax+12*4]
                    movaps  xmmword ptr[edx+12*4],xmm3

                    add     eax,16*4
                    add     edx,16*4
                    loop    loopua
                }
            }
            else {
                // unaligned src, unaligned dst
                __asm {
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ecx,[n]
loopuu:
                    prefetcht0 [eax+64]
                    prefetcht0 [eax+96]
                    movups  xmm0,xmmword ptr[eax]
                    movups  xmmword ptr[edx],xmm0
                    movups  xmm1,xmmword ptr[eax+4*4]
                    movups  xmmword ptr[edx+4*4],xmm1
                    movups  xmm2,xmmword ptr[eax+8*4]
                    movups  xmmword ptr[edx+8*4],xmm2
                    movups  xmm3,xmmword ptr[eax+12*4]
                    movups  xmmword ptr[edx+12*4],xmm3

                    add     eax,16*4
                    add     edx,16*4
                    loop    loopuu
                }
            }
        }

        src += n<<4,dst += n<<4;
zero:   
        while(cnt--) *(dst++) = *(src++); 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VECTOROPS__)
    if(true) {
        int n = cnt>>2,n4 = n<<2;
        vScopy(n4,(vector float *)src,(vector float *)dst);
        cnt -= n4,src += n4,dst += n4;
        while(cnt--) *(dst++) = *(src++); 
    }
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;
        while(n--) {
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
            dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
            src += 8,dst += 8;
        }
        while(cnt--) *(dst++) = *(src++); 
    }
#endif
}

#if defined(FLEXT_USE_SIMD) && (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
// because of some frame code Altivec stuff should be in seperate functions....

static const vector float zero = (vector float)(0);

static void SetAltivec(t_sample *dst,int cnt,t_sample s)
{
    vector float svec = LoadValue(s);
    int n = cnt>>4;
    cnt -= n<<4;

    while(n--) {
        vec_st(svec,0,dst);
        vec_st(svec,16,dst);
        vec_st(svec,32,dst);
        vec_st(svec,48,dst);
        dst += 16;
    }

    while(cnt--) *(dst++) = s; 
}

static void MulAltivec(t_sample *dst,const t_sample *src,t_sample op,int cnt) 
{
    const vector float arg = LoadValue(op);
    int n = cnt>>4;
    cnt -= n<<4;

    for(; n--; src += 16,dst += 16) {
        vector float a1 = vec_ld( 0,src);
        vector float a2 = vec_ld(16,src);
        vector float a3 = vec_ld(32,src);
        vector float a4 = vec_ld(48,src);
        
        a1 = vec_madd(a1,arg,zero);
        a2 = vec_madd(a2,arg,zero);
        a3 = vec_madd(a3,arg,zero);
        a4 = vec_madd(a4,arg,zero);

        vec_st(a1, 0,dst);
        vec_st(a2,16,dst);
        vec_st(a3,32,dst);
        vec_st(a4,48,dst);
    }

    while(cnt--) *(dst++) = *(src++)*op; 
}

static void MulAltivec(t_sample *dst,const t_sample *src,const t_sample *op,int cnt) 
{
    int n = cnt>>4;
    cnt -= n<<4;
    
    for(; n--; src += 16,op += 16,dst += 16) {
        vector float a1 = vec_ld( 0,src),b1 = vec_ld( 0,op);
        vector float a2 = vec_ld(16,src),b2 = vec_ld(16,op);
        vector float a3 = vec_ld(32,src),b3 = vec_ld(32,op);
        vector float a4 = vec_ld(48,src),b4 = vec_ld(48,op);
        
        a1 = vec_madd(a1,b1,zero);
        a2 = vec_madd(a2,b2,zero);
        a3 = vec_madd(a3,b3,zero);
        a4 = vec_madd(a4,b4,zero);

        vec_st(a1, 0,dst);
        vec_st(a2,16,dst);
        vec_st(a3,32,dst);
        vec_st(a4,48,dst);
    }
    while(cnt--) *(dst++) = *(src++) * *(op++); 
}

static void AddAltivec(t_sample *dst,const t_sample *src,t_sample op,int cnt) 
{
    const vector float arg = LoadValue(op);
    int n = cnt>>4;
    cnt -= n<<4;

    for(; n--; src += 16,dst += 16) {
        vector float a1 = vec_ld( 0,src);
        vector float a2 = vec_ld(16,src);
        vector float a3 = vec_ld(32,src);
        vector float a4 = vec_ld(48,src);
        
        a1 = vec_add(a1,arg);
        a2 = vec_add(a2,arg);
        a3 = vec_add(a3,arg);
        a4 = vec_add(a4,arg);

        vec_st(a1, 0,dst);
        vec_st(a2,16,dst);
        vec_st(a3,32,dst);
        vec_st(a4,48,dst);
    }

    while(cnt--) *(dst++) = *(src++)+op; 
}

static void AddAltivec(t_sample *dst,const t_sample *src,const t_sample *op,int cnt) 
{
    int n = cnt>>4;
    cnt -= n<<4;
    
    for(; n--; src += 16,op += 16,dst += 16) {
        vector float a1 = vec_ld( 0,src),b1 = vec_ld( 0,op);
        vector float a2 = vec_ld(16,src),b2 = vec_ld(16,op);
        vector float a3 = vec_ld(32,src),b3 = vec_ld(32,op);
        vector float a4 = vec_ld(48,src),b4 = vec_ld(48,op);
        
        a1 = vec_add(a1,b1);
        a2 = vec_add(a2,b2);
        a3 = vec_add(a3,b3);
        a4 = vec_add(a4,b4);

        vec_st(a1, 0,dst);
        vec_st(a2,16,dst);
        vec_st(a3,32,dst);
        vec_st(a4,48,dst);
    }
    while(cnt--) *(dst++) = *(src++) + *(op++); 
}

static void ScaleAltivec(t_sample *dst,const t_sample *src,t_sample opmul,t_sample opadd,int cnt) 
{
    const vector float argmul = LoadValue(opmul);
    const vector float argadd = LoadValue(opadd);
    int n = cnt>>4;
    cnt -= n<<4;

    for(; n--; src += 16,dst += 16) {
        vec_st(vec_madd(vec_ld( 0,src),argmul,argadd), 0,dst);
        vec_st(vec_madd(vec_ld(16,src),argmul,argadd),16,dst);
        vec_st(vec_madd(vec_ld(32,src),argmul,argadd),32,dst);
        vec_st(vec_madd(vec_ld(48,src),argmul,argadd),48,dst);
    }

    while(cnt--) *(dst++) = *(src++)*opmul+opadd; 
}

static void ScaleAltivec(t_sample *dst,const t_sample *src,t_sample opmul,const t_sample *add,int cnt) 
{
    const vector float argmul = LoadValue(opmul);
    int n = cnt>>4;
    cnt -= n<<4;

    for(; n--; src += 16,dst += 16,add += 16) {
        vec_st(vec_madd(vec_ld( 0,src),argmul,vec_ld( 0,add)), 0,dst);
        vec_st(vec_madd(vec_ld(16,src),argmul,vec_ld(16,add)),16,dst);
        vec_st(vec_madd(vec_ld(32,src),argmul,vec_ld(32,add)),32,dst);
        vec_st(vec_madd(vec_ld(48,src),argmul,vec_ld(48,add)),48,dst);
    }

    while(cnt--) *(dst++) = *(src++) * opmul + *(add++); 
}

static void ScaleAltivec(t_sample *dst,const t_sample *src,const t_sample *mul,const t_sample *add,int cnt) 
{
    int n = cnt>>4;
    cnt -= n<<4;

    for(; n--; src += 16,dst += 16,mul += 16,add += 16) {
        vec_st(vec_madd(vec_ld( 0,src),vec_ld( 0,mul),vec_ld( 0,add)), 0,dst);
        vec_st(vec_madd(vec_ld(16,src),vec_ld(16,mul),vec_ld(16,add)),16,dst);
        vec_st(vec_madd(vec_ld(32,src),vec_ld(32,mul),vec_ld(32,add)),32,dst);
        vec_st(vec_madd(vec_ld(48,src),vec_ld(48,mul),vec_ld(48,add)),48,dst);
    }

    while(cnt--) *(dst++) = *(src++) * *(mul++) + *(add++); 
}
#endif

void flext::SetSamples(t_sample *dst,int cnt,t_sample s) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4)
        ippsSet_32f((float)s,(float *)dst,cnt); 
    else if(sizeof(t_sample) == 8)
        ippsSet_64f((double)s,(double *)dst,cnt); 
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision

        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            movss   xmm0,xmmword ptr [s]
            shufps  xmm0,xmm0,0
        }

        if(IsVectorAligned(dst)) {
            // aligned version
            __asm {
                mov     ecx,[n]
                mov     edx,dword ptr [dst]
loopa:
                movaps  xmmword ptr[edx],xmm0
                movaps  xmmword ptr[edx+4*4],xmm0
                movaps  xmmword ptr[edx+8*4],xmm0
                movaps  xmmword ptr[edx+12*4],xmm0

                add     edx,16*4
                loop    loopa
            }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,[n]
                mov     edx,dword ptr [dst]
loopu:
                movups  xmmword ptr[edx],xmm0
                movups  xmmword ptr[edx+4*4],xmm0
                movups  xmmword ptr[edx+8*4],xmm0
                movups  xmmword ptr[edx+12*4],xmm0

                add     edx,16*4
                loop    loopu
            }
        }

        dst += n<<4;
zero:
        while(cnt--) *(dst++) = s; 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && IsVectorAligned(dst)) 
        SetAltivec(dst,cnt,s);
    else
#endif
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;
        while(n--) {
            dst[0] = dst[1] = dst[2] = dst[3] = dst[4] = dst[5] = dst[6] = dst[7] = s;
            dst += 8;
        }
        
        while(cnt--) *(dst++) = s; 
    }
#endif
}


void flext::MulSamples(t_sample *dst,const t_sample *src,t_sample op,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsMulC_32f((const float *)src,(float)op,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsMulC_64f((const double *)src,(double)op,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        __m128 a = _mm_load1_ps(&op);

        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,dword ptr [src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]

            movss   xmm0,xmmword ptr [op]
            shufps  xmm0,xmm0,0
        }

        if(VectorsAligned(src,dst)) {
            // aligned version
            __asm {
                mov     ecx,[n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopa:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movaps  xmm1,xmmword ptr[eax]
                mulps   xmm1,xmm0
                movaps  xmmword ptr[edx],xmm1

                movaps  xmm2,xmmword ptr[eax+4*4]
                mulps   xmm2,xmm0
                movaps  xmmword ptr[edx+4*4],xmm2

                movaps  xmm3,xmmword ptr[eax+8*4]
                mulps   xmm3,xmm0
                movaps  xmmword ptr[edx+8*4],xmm3

                movaps  xmm4,xmmword ptr[eax+12*4]
                mulps   xmm4,xmm0
                movaps  xmmword ptr[edx+12*4],xmm4

                add     eax,16*4
                add     edx,16*4
                loop    loopa
            }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,[n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopu:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movups  xmm1,xmmword ptr[eax]
                mulps   xmm1,xmm0
                movups  xmmword ptr[edx],xmm1

                movups  xmm2,xmmword ptr[eax+4*4]
                mulps   xmm2,xmm0
                movups  xmmword ptr[edx+4*4],xmm2

                movups  xmm3,xmmword ptr[eax+8*4]
                mulps   xmm3,xmm0
                movups  xmmword ptr[edx+8*4],xmm3

                movups  xmm4,xmmword ptr[eax+12*4]
                mulps   xmm4,xmm0
                movups  xmmword ptr[edx+12*4],xmm4

                add     eax,16*4
                add     edx,16*4
                loop    loopu
            }
        }

        src += n<<4,dst += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++)*op; 
    }
    else
#elif defined(__APPLE__) && defined(__VDSP__)
    if(true) {
        vDSP_vsmul(src,1,&op,dst,1,cnt);
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,dst)) 
        MulAltivec(dst,src,op,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;

        if(src == dst) {
            while(n--) {
                dst[0] *= op; dst[1] *= op; dst[2] *= op; dst[3] *= op; 
                dst[4] *= op; dst[5] *= op; dst[6] *= op; dst[7] *= op; 
                dst += 8;
            }
            while(cnt--) *(dst++) *= op; 
        }
        else {
            while(n--) {
                dst[0] = src[0]*op; dst[1] = src[1]*op; 
                dst[2] = src[2]*op; dst[3] = src[3]*op; 
                dst[4] = src[4]*op; dst[5] = src[5]*op; 
                dst[6] = src[6]*op; dst[7] = src[7]*op; 
                src += 8,dst += 8;
            }
            while(cnt--) *(dst++) = *(src++)*op; 
        }
    }
#endif
}


void flext::MulSamples(t_sample *dst,const t_sample *src,const t_sample *op,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsMul_32f((const float *)src,(const float *)op,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsMul_32f((const double *)src,(const double *)op,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,[src]
            mov     ebx,[op]
            prefetcht0 [eax+0]
            prefetcht0 [ebx+0]
            prefetcht0 [eax+32]
            prefetcht0 [ebx+32]
        }

        if(VectorsAligned(src,dst)) {
            if(IsVectorAligned(op)) {
                __asm {
                    mov     ecx,[n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopaa:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movaps  xmm0,xmmword ptr[eax]
                    movaps  xmm1,xmmword ptr[ebx]
                    mulps   xmm0,xmm1
                    movaps  xmmword ptr[edx],xmm0

                    movaps  xmm2,xmmword ptr[eax+4*4]
                    movaps  xmm3,xmmword ptr[ebx+4*4]
                    mulps   xmm2,xmm3
                    movaps  xmmword ptr[edx+4*4],xmm2

                    movaps  xmm4,xmmword ptr[eax+8*4]
                    movaps  xmm5,xmmword ptr[ebx+8*4]
                    mulps   xmm4,xmm5
                    movaps  xmmword ptr[edx+8*4],xmm4

                    movaps  xmm6,xmmword ptr[eax+12*4]
                    movaps  xmm7,xmmword ptr[ebx+12*4]
                    mulps   xmm6,xmm7
                    movaps  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopaa
                }
            }
            else {
                __asm {
                    mov     ecx,[n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopau:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movaps  xmm0,xmmword ptr[eax]
                    movups  xmm1,xmmword ptr[ebx]
                    mulps   xmm0,xmm1
                    movaps  xmmword ptr[edx],xmm0

                    movaps  xmm2,xmmword ptr[eax+4*4]
                    movups  xmm3,xmmword ptr[ebx+4*4]
                    mulps   xmm2,xmm3
                    movaps  xmmword ptr[edx+4*4],xmm2

                    movaps  xmm4,xmmword ptr[eax+8*4]
                    movups  xmm5,xmmword ptr[ebx+8*4]
                    mulps   xmm4,xmm5
                    movaps  xmmword ptr[edx+8*4],xmm4

                    movaps  xmm6,xmmword ptr[eax+12*4]
                    movups  xmm7,xmmword ptr[ebx+12*4]
                    mulps   xmm6,xmm7
                    movaps  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopau
                }
            }
        }
        else {
            if(IsVectorAligned(op)) {
                __asm {
                    mov     ecx,[n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopua:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movups  xmm0,xmmword ptr[eax]
                    movaps  xmm1,xmmword ptr[ebx]
                    mulps   xmm0,xmm1
                    movups  xmmword ptr[edx],xmm0

                    movups  xmm2,xmmword ptr[eax+4*4]
                    movaps  xmm3,xmmword ptr[ebx+4*4]
                    mulps   xmm2,xmm3
                    movups  xmmword ptr[edx+4*4],xmm2

                    movups  xmm4,xmmword ptr[eax+8*4]
                    movaps  xmm5,xmmword ptr[ebx+8*4]
                    mulps   xmm4,xmm5
                    movups  xmmword ptr[edx+8*4],xmm4

                    movups  xmm6,xmmword ptr[eax+12*4]
                    movaps  xmm7,xmmword ptr[ebx+12*4]
                    mulps   xmm6,xmm7
                    movups  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopua
                }
            }
            else {
                __asm {
                    mov     ecx,[n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
loopuu:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movups  xmm0,xmmword ptr[eax]
                    movups  xmm1,xmmword ptr[ebx]
                    mulps   xmm0,xmm1
                    movups  xmmword ptr[edx],xmm0

                    movups  xmm2,xmmword ptr[eax+4*4]
                    movups  xmm3,xmmword ptr[ebx+4*4]
                    mulps   xmm2,xmm3
                    movups  xmmword ptr[edx+4*4],xmm2

                    movups  xmm4,xmmword ptr[eax+8*4]
                    movups  xmm5,xmmword ptr[ebx+8*4]
                    mulps   xmm4,xmm5
                    movups  xmmword ptr[edx+8*4],xmm4

                    movups  xmm6,xmmword ptr[eax+12*4]
                    movups  xmm7,xmmword ptr[ebx+12*4]
                    mulps   xmm6,xmm7
                    movups  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopuu
                }
            }
        }

        src += n<<4,dst += n<<4,op += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++) * *(op++); 
    }
    else
#elif defined(__APPLE__) && defined(__VDSP__)
    if(true) {
        vDSP_vmul(src,1,op,1,dst,1,cnt);
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,op,dst)) 
        MulAltivec(dst,src,op,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;

        if(src == dst) {
            while(n--) {
                dst[0] *= op[0]; dst[1] *= op[1]; 
                dst[2] *= op[2]; dst[3] *= op[3]; 
                dst[4] *= op[4]; dst[5] *= op[5]; 
                dst[6] *= op[6]; dst[7] *= op[7]; 
                dst += 8,op += 8;
            }
            while(cnt--) *(dst++) *= *(op++); 
        }
        else {
            while(n--) {
                dst[0] = src[0]*op[0]; dst[1] = src[1]*op[1]; 
                dst[2] = src[2]*op[2]; dst[3] = src[3]*op[3]; 
                dst[4] = src[4]*op[4]; dst[5] = src[5]*op[5]; 
                dst[6] = src[6]*op[6]; dst[7] = src[7]*op[7]; 
                src += 8,dst += 8,op += 8;
            }
            while(cnt--) *(dst++) = *(src++) * *(op++); 
        }
    }
#endif
}


void flext::AddSamples(t_sample *dst,const t_sample *src,t_sample op,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsAddC_32f((const float *)src,(float)op,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsAddC_64f_I((const double *)src,(double)op,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,[src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]

            movss   xmm0,xmmword ptr [op]
            shufps  xmm0,xmm0,0
        }

        if(VectorsAligned(src,dst)) {
            // aligned version
                __asm {
                mov     ecx,[n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopa:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movaps  xmm1,xmmword ptr[eax]
                addps   xmm1,xmm0
                movaps  xmmword ptr[edx],xmm1

                movaps  xmm2,xmmword ptr[eax+4*4]
                addps   xmm2,xmm0
                movaps  xmmword ptr[edx+4*4],xmm2

                movaps  xmm3,xmmword ptr[eax+8*4]
                addps   xmm3,xmm0
                movaps  xmmword ptr[edx+8*4],xmm3

                movaps  xmm4,xmmword ptr[eax+12*4]
                addps   xmm4,xmm0
                movaps  xmmword ptr[edx+12*4],xmm4
 
                add     eax,16*4
                add     edx,16*4
                loop    loopa
           }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,[n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopu:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movups  xmm1,xmmword ptr[eax]
                addps   xmm1,xmm0
                movups  xmmword ptr[edx],xmm1

                movups  xmm2,xmmword ptr[eax+4*4]
                addps   xmm2,xmm0
                movups  xmmword ptr[edx+4*4],xmm2

                movups  xmm3,xmmword ptr[eax+8*4]
                addps   xmm3,xmm0
                movups  xmmword ptr[edx+8*4],xmm3

                movups  xmm4,xmmword ptr[eax+12*4]
                addps   xmm4,xmm0
                movups  xmmword ptr[edx+12*4],xmm4

                add     eax,16*4
                add     edx,16*4
                loop    loopu
            }
        }
        src += n<<4,dst += n<<4,op += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++)+op; 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,dst)) 
        AddAltivec(dst,src,op,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;

        if(src == dst) {
            while(n--) {
                dst[0] += op; dst[1] += op; dst[2] += op; dst[3] += op; 
                dst[4] += op; dst[5] += op; dst[6] += op; dst[7] += op; 
                dst += 8;
            }
            while(cnt--) *(dst++) += op; 
        }
        else {
            while(n--) {
                dst[0] = src[0]+op; dst[1] = src[1]+op; 
                dst[2] = src[2]+op; dst[3] = src[3]+op; 
                dst[4] = src[4]+op; dst[5] = src[5]+op; 
                dst[6] = src[6]+op; dst[7] = src[7]+op; 
                src += 8,dst += 8;
            }
            while(cnt--) *(dst++) = *(src++)+op; 
        }
    }
#endif
}


void flext::AddSamples(t_sample *dst,const t_sample *src,const t_sample *op,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsAdd_32f((const float *)src,(const float *)op,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsAdd_64f((const double *)src,(const double *)op,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // Prefetch cache
        __asm {
            mov     eax,dword ptr [src]
            mov     ebx,dword ptr [op]
            prefetcht0 [eax]
            prefetcht0 [ebx]
            prefetcht0 [eax+32]
            prefetcht0 [ebx+32]
        }

        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        if(VectorsAligned(src,dst)) {
            if(IsVectorAligned(op)) {
                __asm {
                    mov     ecx,dword ptr [n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopaa:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movaps  xmm0,xmmword ptr[eax]
                    movaps  xmm1,xmmword ptr[ebx]
                    addps   xmm0,xmm1
                    movaps  xmmword ptr[edx],xmm0

                    movaps  xmm2,xmmword ptr[eax+4*4]
                    movaps  xmm3,xmmword ptr[ebx+4*4]
                    addps   xmm2,xmm3
                    movaps  xmmword ptr[edx+4*4],xmm2

                    movaps  xmm4,xmmword ptr[eax+8*4]
                    movaps  xmm5,xmmword ptr[ebx+8*4]
                    addps   xmm4,xmm5
                    movaps  xmmword ptr[edx+8*4],xmm4

                    movaps  xmm6,xmmword ptr[eax+12*4]
                    movaps  xmm7,xmmword ptr[ebx+12*4]
                    addps   xmm6,xmm7
                    movaps  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopaa
                }
            }
            else {
                __asm {
                    mov     ecx,dword ptr [n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopau:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movaps  xmm0,xmmword ptr[eax]
                    movups  xmm1,xmmword ptr[ebx]
                    addps   xmm0,xmm1
                    movaps  xmmword ptr[edx],xmm0

                    movaps  xmm2,xmmword ptr[eax+4*4]
                    movups  xmm3,xmmword ptr[ebx+4*4]
                    addps   xmm2,xmm3
                    movaps  xmmword ptr[edx+4*4],xmm2

                    movaps  xmm4,xmmword ptr[eax+8*4]
                    movups  xmm5,xmmword ptr[ebx+8*4]
                    addps   xmm4,xmm5
                    movaps  xmmword ptr[edx+8*4],xmm4

                    movaps  xmm6,xmmword ptr[eax+12*4]
                    movups  xmm7,xmmword ptr[ebx+12*4]
                    addps   xmm6,xmm7
                    movaps  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopau
                }
            }
        }
        else {
            if(IsVectorAligned(op)) {
                __asm {
                    mov     ecx,dword ptr [n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopua:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movups  xmm0,xmmword ptr[eax]
                    movaps  xmm1,xmmword ptr[ebx]
                    addps   xmm0,xmm1
                    movups  xmmword ptr[edx],xmm0

                    movups  xmm2,xmmword ptr[eax+4*4]
                    movaps  xmm3,xmmword ptr[ebx+4*4]
                    addps   xmm2,xmm3
                    movups  xmmword ptr[edx+4*4],xmm2

                    movups  xmm4,xmmword ptr[eax+8*4]
                    movaps  xmm5,xmmword ptr[ebx+8*4]
                    addps   xmm4,xmm5
                    movups  xmmword ptr[edx+8*4],xmm4

                    movups  xmm6,xmmword ptr[eax+12*4]
                    movaps  xmm7,xmmword ptr[ebx+12*4]
                    addps   xmm6,xmm7
                    movups  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopua
                }
            }
            else {
                __asm {
                    mov     ecx,dword ptr [n]
                    mov     eax,dword ptr [src]
                    mov     edx,dword ptr [dst]
                    mov     ebx,dword ptr [op]
    loopuu:
                    prefetcht0 [eax+64]
                    prefetcht0 [ebx+64]
                    prefetcht0 [eax+96]
                    prefetcht0 [ebx+96]

                    movups  xmm0,xmmword ptr[eax]
                    movups  xmm1,xmmword ptr[ebx]
                    addps   xmm0,xmm1
                    movups  xmmword ptr[edx],xmm0

                    movups  xmm2,xmmword ptr[eax+4*4]
                    movups  xmm3,xmmword ptr[ebx+4*4]
                    addps   xmm2,xmm3
                    movups  xmmword ptr[edx+4*4],xmm2

                    movups  xmm4,xmmword ptr[eax+8*4]
                    movups  xmm5,xmmword ptr[ebx+8*4]
                    addps   xmm4,xmm5
                    movups  xmmword ptr[edx+8*4],xmm4

                    movups  xmm6,xmmword ptr[eax+12*4]
                    movups  xmm7,xmmword ptr[ebx+12*4]
                    addps   xmm6,xmm7
                    movups  xmmword ptr[edx+12*4],xmm6

                    add     eax,16*4
                    add     ebx,16*4
                    add     edx,16*4
                    loop    loopuu
                }
            }
        }

        src += n<<4,dst += n<<4,op += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++) + *(op++); 
    }
    else
#elif defined(__APPLE__) && defined(__VDSP__)
    if(true) {
        vDSP_vadd(src,1,op,1,dst,1,cnt);
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,op,dst))
        AddAltivec(dst,src,op,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;

        if(dst == src) {
            while(n--) {
                dst[0] += op[0]; dst[1] += op[1]; 
                dst[2] += op[2]; dst[3] += op[3]; 
                dst[4] += op[4]; dst[5] += op[5]; 
                dst[6] += op[6]; dst[7] += op[7]; 
                dst += 8,op += 8;
            }
            while(cnt--) *(dst++) += *(op++); 
        }
        else {
            while(n--) {
                dst[0] = src[0]+op[0]; dst[1] = src[1]+op[1]; 
                dst[2] = src[2]+op[2]; dst[3] = src[3]+op[3]; 
                dst[4] = src[4]+op[4]; dst[5] = src[5]+op[5]; 
                dst[6] = src[6]+op[6]; dst[7] = src[7]+op[7]; 
                src += 8,dst += 8,op += 8;
            }
            while(cnt--) *(dst++) = *(src++) + *(op++); 
        }
    }
#endif
}


void flext::ScaleSamples(t_sample *dst,const t_sample *src,t_sample opmul,t_sample opadd,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsMulC_32f((const float *)src,(float)opmul,(float *)dst,cnt); 
        ippsAddC_32f_I((float)opadd,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsMulC_64f((const double *)src,(double)opmul,(double *)dst,cnt); 
        ippsAddC_64f_I((double)opadd,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,dword ptr [src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]

            movss   xmm0,xmmword ptr [opadd]
            shufps  xmm0,xmm0,0
            movss   xmm1,xmmword ptr [opmul]
            shufps  xmm1,xmm1,0
        }

        if(VectorsAligned(src,dst)) {
            // aligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopa:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movaps  xmm2,xmmword ptr[eax]
                mulps   xmm2,xmm1
                addps   xmm2,xmm0
                movaps  xmmword ptr[edx],xmm2

                movaps  xmm3,xmmword ptr[eax+4*4]
                mulps   xmm3,xmm1
                addps   xmm3,xmm0
                movaps  xmmword ptr[edx+4*4],xmm3

                movaps  xmm4,xmmword ptr[eax+8*4]
                mulps   xmm4,xmm1
                addps   xmm4,xmm0
                movaps  xmmword ptr[edx+8*4],xmm4

                movaps  xmm5,xmmword ptr[eax+12*4]
                mulps   xmm5,xmm1
                addps   xmm5,xmm0
                movaps  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                loop    loopa
            }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
loopu:
                prefetcht0 [eax+64]
                prefetcht0 [eax+96]

                movups  xmm2,xmmword ptr[eax]
                mulps   xmm2,xmm1
                addps   xmm2,xmm0
                movups  xmmword ptr[edx],xmm2

                movups  xmm3,xmmword ptr[eax+4*4]
                mulps   xmm3,xmm1
                addps   xmm3,xmm0
                movups  xmmword ptr[edx+4*4],xmm3

                movups  xmm4,xmmword ptr[eax+8*4]
                mulps   xmm4,xmm1
                addps   xmm4,xmm0
                movups  xmmword ptr[edx+8*4],xmm4

                movups  xmm5,xmmword ptr[eax+12*4]
                mulps   xmm5,xmm1
                addps   xmm5,xmm0
                movups  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                loop    loopu
            }
        }

        src += n<<4,dst += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++)*opmul+opadd; 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,dst)) 
        ScaleAltivec(dst,src,opmul,opadd,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;
        while(n--) {
            dst[0] = src[0]*opmul+opadd; dst[1] = src[1]*opmul+opadd; 
            dst[2] = src[2]*opmul+opadd; dst[3] = src[3]*opmul+opadd; 
            dst[4] = src[4]*opmul+opadd; dst[5] = src[5]*opmul+opadd; 
            dst[6] = src[6]*opmul+opadd; dst[7] = src[7]*opmul+opadd; 
            src += 8,dst += 8;
        }
        while(cnt--) *(dst++) = *(src++)*opmul+opadd; 
    }
#endif
}

void flext::ScaleSamples(t_sample *dst,const t_sample *src,t_sample opmul,const t_sample *opadd,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsMulC_32f((const float *)src,(float)opmul,(float *)dst,cnt); 
        ippsAdd_32f_I((float *)opadd,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsMulC_64f((const double *)src,(double)opmul,(double *)dst,cnt); 
        ippsAdd_64f_I((double *)opadd,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,dword ptr [src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]

            movss   xmm0,xmmword ptr [opmul]
            shufps  xmm0,xmm0,0
        }

        if(VectorsAligned(src,dst,opadd)) {
            // aligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
                mov     ebx,dword ptr [opadd]
loopa:
                prefetcht0 [eax+64]
                prefetcht0 [ebx+64]
                prefetcht0 [eax+96]
                prefetcht0 [ebx+96]

                movaps  xmm2,xmmword ptr[eax]
                movaps  xmm1,xmmword ptr[ebx]
                mulps   xmm2,xmm0
                addps   xmm2,xmm1
                movaps  xmmword ptr[edx],xmm2

                movaps  xmm3,xmmword ptr[eax+4*4]
                movaps  xmm1,xmmword ptr[ebx+4*4]
                mulps   xmm3,xmm0
                addps   xmm3,xmm1
                movaps  xmmword ptr[edx+4*4],xmm3

                movaps  xmm4,xmmword ptr[eax+8*4]
                movaps  xmm1,xmmword ptr[ebx+8*4]
                mulps   xmm4,xmm0
                addps   xmm4,xmm1
                movaps  xmmword ptr[edx+8*4],xmm4

                movaps  xmm5,xmmword ptr[eax+12*4]
                movaps  xmm1,xmmword ptr[ebx+12*4]
                mulps   xmm5,xmm0
                addps   xmm5,xmm1
                movaps  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                add     ebx,16*4
                loop    loopa
            }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
                mov     ebx,dword ptr [opadd]
loopu:
                prefetcht0 [eax+64]
                prefetcht0 [ebx+64]
                prefetcht0 [eax+96]
                prefetcht0 [ebx+96]

                movups  xmm2,xmmword ptr[eax]
                movups  xmm1,xmmword ptr[ebx]
                mulps   xmm2,xmm0
                addps   xmm2,xmm1
                movups  xmmword ptr[edx],xmm2

                movups  xmm3,xmmword ptr[eax+4*4]
                movups  xmm1,xmmword ptr[ebx+4*4]
                mulps   xmm3,xmm0
                addps   xmm3,xmm1
                movups  xmmword ptr[edx+4*4],xmm3

                movups  xmm4,xmmword ptr[eax+8*4]
                movups  xmm1,xmmword ptr[ebx+8*4]
                mulps   xmm4,xmm0
                addps   xmm4,xmm1
                movups  xmmword ptr[edx+8*4],xmm4

                movups  xmm5,xmmword ptr[eax+12*4]
                movups  xmm1,xmmword ptr[ebx+12*4]
                mulps   xmm5,xmm0
                addps   xmm5,xmm1
                movups  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                add     ebx,16*4
                loop    loopu
            }
        }

        src += n<<4,dst += n<<4,opadd += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++) * opmul + *(opadd++); 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,dst,opadd)) 
        ScaleAltivec(dst,src,opmul,opadd,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;
        if(dst == opadd) {
            while(n--) {
                dst[0] += src[0]*opmul; dst[1] += src[1]*opmul; 
                dst[2] += src[2]*opmul; dst[3] += src[3]*opmul; 
                dst[4] += src[4]*opmul; dst[5] += src[5]*opmul; 
                dst[6] += src[6]*opmul; dst[7] += src[7]*opmul; 
                src += 8,dst += 8;
            }
            while(cnt--) *(dst++) += *(src++)*opmul; 
        }
        else {
            while(n--) {
                dst[0] = src[0]*opmul+opadd[0]; dst[1] = src[1]*opmul+opadd[1]; 
                dst[2] = src[2]*opmul+opadd[2]; dst[3] = src[3]*opmul+opadd[3]; 
                dst[4] = src[4]*opmul+opadd[4]; dst[5] = src[5]*opmul+opadd[5]; 
                dst[6] = src[6]*opmul+opadd[6]; dst[7] = src[7]*opmul+opadd[7]; 
                src += 8,dst += 8,opadd += 8;
            }
            while(cnt--) *(dst++) = *(src++)*opmul+*(opadd++); 
        }
    }
#endif
}

void flext::ScaleSamples(t_sample *dst,const t_sample *src,const t_sample *opmul,const t_sample *opadd,int cnt) 
{
#ifdef FLEXT_USE_IPP
    if(sizeof(t_sample) == 4) {
        ippsMul_32f((const float *)src,(const float *)opmul,(float *)dst,cnt); 
        ippsAdd_32f_I((const float *)opadd,(float *)dst,cnt); 
    }
    else if(sizeof(t_sample) == 8) {
        ippsMul_64f((const double *)src,(const double *)opmul,(double *)dst,cnt); 
        ippsAdd_64f_I((const double *)opadd,(double *)dst,cnt); 
    }
    else
        ERRINTERNAL();
#else
#ifdef FLEXT_USE_SIMD
#ifdef _MSC_VER
    if(GetSIMDCapabilities()&simd_sse) {
        // single precision
        int n = cnt>>4;
        if(!n) goto zero;
        cnt -= n<<4;

        __asm {
            mov     eax,dword ptr [src]
            prefetcht0 [eax+0]
            prefetcht0 [eax+32]
        }

        if(VectorsAligned(src,dst,opmul,opadd)) {
            // aligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
                mov     esi,dword ptr [opmul]
                mov     ebx,dword ptr [opadd]
loopa:
                prefetcht0 [eax+64]
                prefetcht0 [ebx+64]
                prefetcht0 [esi+64]
                prefetcht0 [eax+96]
                prefetcht0 [ebx+96]
                prefetcht0 [esi+96]

                movaps  xmm2,xmmword ptr[eax]
                movaps  xmm0,xmmword ptr[esi]
                movaps  xmm1,xmmword ptr[ebx]
                mulps   xmm2,xmm0
                addps   xmm2,xmm1
                movaps  xmmword ptr[edx],xmm2

                movaps  xmm3,xmmword ptr[eax+4*4]
                movaps  xmm0,xmmword ptr[esi+4*4]
                movaps  xmm1,xmmword ptr[ebx+4*4]
                mulps   xmm3,xmm0
                addps   xmm3,xmm1
                movaps  xmmword ptr[edx+4*4],xmm3

                movaps  xmm4,xmmword ptr[eax+8*4]
                movaps  xmm0,xmmword ptr[esi+8*4]
                movaps  xmm1,xmmword ptr[ebx+8*4]
                mulps   xmm4,xmm0
                addps   xmm4,xmm1
                movaps  xmmword ptr[edx+8*4],xmm4

                movaps  xmm5,xmmword ptr[eax+12*4]
                movaps  xmm0,xmmword ptr[esi+12*4]
                movaps  xmm1,xmmword ptr[ebx+12*4]
                mulps   xmm5,xmm0
                addps   xmm5,xmm1
                movaps  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                add     ebx,16*4
                add     esi,16*4
                loop    loopa
            }
        }
        else {
            // unaligned version
            __asm {
                mov     ecx,dword ptr [n]
                mov     eax,dword ptr [src]
                mov     edx,dword ptr [dst]
                mov     esi,dword ptr [opmul]
                mov     ebx,dword ptr [opadd]
loopu:
                prefetcht0 [eax+64]
                prefetcht0 [ebx+64]
                prefetcht0 [esi+64]
                prefetcht0 [eax+96]
                prefetcht0 [ebx+96]
                prefetcht0 [esi+96]

                movups  xmm2,xmmword ptr[eax]
                movups  xmm0,xmmword ptr[esi]
                movups  xmm1,xmmword ptr[ebx]
                mulps   xmm2,xmm0
                addps   xmm2,xmm1
                movups  xmmword ptr[edx],xmm2

                movups  xmm3,xmmword ptr[eax+4*4]
                movups  xmm0,xmmword ptr[esi+4*4]
                movups  xmm1,xmmword ptr[ebx+4*4]
                mulps   xmm3,xmm0
                addps   xmm3,xmm1
                movups  xmmword ptr[edx+4*4],xmm3

                movups  xmm4,xmmword ptr[eax+8*4]
                movups  xmm0,xmmword ptr[esi+8*4]
                movups  xmm1,xmmword ptr[ebx+8*4]
                mulps   xmm4,xmm0
                addps   xmm4,xmm1
                movups  xmmword ptr[edx+8*4],xmm4

                movups  xmm5,xmmword ptr[eax+12*4]
                movups  xmm0,xmmword ptr[esi+12*4]
                movups  xmm1,xmmword ptr[ebx+12*4]
                mulps   xmm5,xmm0
                addps   xmm5,xmm1
                movups  xmmword ptr[edx+12*4],xmm5

                add     eax,16*4
                add     edx,16*4
                add     ebx,16*4
                add     esi,16*4
                loop    loopu
            }
        }
        src += n<<4,dst += n<<4,opmul += n<<4,opadd += n<<4;
zero:
        while(cnt--) *(dst++) = *(src++) * *(opmul++) + *(opadd++); 
    }
    else
#elif (FLEXT_CPU == FLEXT_CPU_PPC || FLEXT_CPU == FLEXT_CPU_PPC64) && defined(__VEC__)
    if(GetSIMDCapabilities()&simd_altivec && VectorsAligned(src,dst,opmul,opadd)) 
        ScaleAltivec(dst,src,opmul,opadd,cnt);
    else
#endif // _MSC_VER
#endif // FLEXT_USE_SIMD
    {
        int n = cnt>>3;
        cnt -= n<<3;
        if(dst == opadd) {
            while(n--) {
                dst[0] += src[0]*opmul[0]; dst[1] += src[1]*opmul[1]; 
                dst[2] += src[2]*opmul[2]; dst[3] += src[3]*opmul[3]; 
                dst[4] += src[4]*opmul[4]; dst[5] += src[5]*opmul[5]; 
                dst[6] += src[6]*opmul[6]; dst[7] += src[7]*opmul[7]; 
                src += 8,dst += 8,opmul += 8;
            }
            while(cnt--) *(dst++) += *(src++) * *(opmul++); 
        }
        else {
            while(n--) {
                dst[0] = src[0]*opmul[0]+opadd[0]; dst[1] = src[1]*opmul[1]+opadd[1]; 
                dst[2] = src[2]*opmul[2]+opadd[2]; dst[3] = src[3]*opmul[3]+opadd[3]; 
                dst[4] = src[4]*opmul[4]+opadd[4]; dst[5] = src[5]*opmul[5]+opadd[5]; 
                dst[6] = src[6]*opmul[6]+opadd[6]; dst[7] = src[7]*opmul[7]+opadd[7]; 
                src += 8,dst += 8,opmul += 8,opadd += 8;
            }
            while(cnt--) *(dst++) = *(src++)* *(opmul++) + *(opadd++); 
        }
    }
#endif
}

#include "flpopns.h"

