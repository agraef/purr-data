/* 
fsplay~ - file and stream player

Copyright (c)2004-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.

$LastChangedRevision: 3599 $
$LastChangedDate: 2008-04-13 07:48:35 -0400 (Sun, 13 Apr 2008) $
$LastChangedBy: thomas $
*/

#ifndef __FSPLAY_H
#define __FSPLAY_H

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#include <string>

#ifdef __GNUC__
typedef long long cnt_t;
#else
typedef __int64 cnt_t;
#endif

class fspformat
{
public:
    virtual ~fspformat() {}

    virtual void ThreadBegin();
    virtual void ThreadEnd();

    virtual int Channels() const = 0;
    virtual float Samplerate() const = 0;
    virtual cnt_t Frames() const = 0;
    virtual cnt_t Read(t_sample *rbuf,cnt_t frames) = 0;
    virtual bool Seek(double pos) = 0;

    static fspformat *New(const std::string &filename);

    typedef fspformat *(*NewHandler)(const std::string &filename);
    typedef bool (*SetupHandler)();

    static bool Add(SetupHandler setupfun);
    static bool Add(NewHandler newfun);

    static void Setup();
};

#endif
