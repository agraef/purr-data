/* 
OSC - OpenSoundControl networking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

/*
Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 1998,99,2000,01,02,03,04
The Regents of the University of California (Regents).  

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

The OSC webpage is http://cnmat.cnmat.berkeley.edu/OpenSoundControl
*/

#include "osc.h"

#include <vector>

namespace OSC {

class OSCException
    : std::runtime_error
{
public:
    OSCException(const char *why): std::runtime_error(why) {}
};


static bool PatternMatch (const char *  pattern, const char * test);

/* we know that pattern[0] == '[' and test[0] != 0 */
static bool MatchBrackets (const char *pattern, const char *test) 
{
    bool result;
    bool negated = false;
    const char *p = pattern;

    if (pattern[1] == 0)
        throw OSCException("Unterminated [ in pattern");

    if (pattern[1] == '!') {
        negated = true;
        p++;
    }

    while (*p != ']') {
        if (*p == 0)
            throw OSCException("Unterminated [ in pattern");

        if (p[1] == '-' && p[2] != 0) {
            if (test[0] >= p[0] && test[0] <= p[2]) {
                result = !negated;
                goto advance;
            }
        }
        if (p[0] == test[0]) {
            result = !negated;
            goto advance;
        }
        p++;
    }

    result = negated;

advance:

    if (!result)
        return false;

    while (*p != ']') {
        if (*p == 0)
            throw OSCException("Unterminated [ in pattern");

        p++;
    }

    return PatternMatch (p+1,test+1);
}

static bool MatchList (const char *pattern, const char *test) 
{
    const char *restOfPattern, *tp = test;

    for(restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) {
        if (*restOfPattern == 0)
            throw OSCException("Unterminated { in pattern");
    }

    restOfPattern++; /* skip close curly brace */
    pattern++; /* skip open curly brace */

    for(;;) {
        if (*pattern == ',') {
            if (PatternMatch (restOfPattern, tp))
                return true;
            else {
                tp = test;
                ++pattern;
            }
        } 
        else if (*pattern == '}')
            return PatternMatch (restOfPattern, tp);
        else if (*pattern == *tp) {
            ++pattern;
            ++tp;
        } 
        else {
            tp = test;
            while (*pattern != ',' && *pattern != '}')
                pattern++;

            if (*pattern == ',')
                pattern++;
        }
    }
}

static bool PatternMatch (const char *  pattern, const char * test) 
{
    if (pattern == 0 || pattern[0] == 0)
        return test[0] == 0;

    if (test[0] == 0) {
        if (pattern[0] == '*')
            return PatternMatch (pattern+1,test);
        else
            return false;
    }

    switch (pattern[0]) {
        case 0      : return test[0] == 0;
        case '?'    : return PatternMatch (pattern + 1, test + 1);
        case '*'    : 
            if (PatternMatch (pattern+1, test))
                return true;
            else
                return PatternMatch (pattern, test+1);
        case ']'    :
            throw OSCException("Spurious ] in pattern");
        case '}'    :
            throw OSCException("Spurious } in pattern");
        case '['    :
            return MatchBrackets (pattern,test);
        case '{'    :
            return MatchList (pattern,test);
        case '\\'   :  
            if (pattern[1] == 0)
                return test[0] == 0;
            else if (pattern[1] == test[0])
                return PatternMatch (pattern+2,test+1);
            else
                return false;
        default     :
            if (pattern[0] == test[0])
                return PatternMatch (pattern+1,test+1);
            else
                return false;
    }
}


#undef FLEXT_ATTRIBUTES
#define FLEXT_ATTRIBUTES 0

class Route
    : public flext_base
{
    FLEXT_HEADER(Route,flext_base)

public:
    Route(int argc,const t_atom *argv)
    {
        AddInAnything();

        tests.reserve(argc);
        for(int i = 0; i < argc; ++i)
            if(IsSymbol(argv[i]))
                tests.push_back(GetSymbol(argv[i]));
            else
                throw std::runtime_error("Arguments must be symbols");

        AddOutList(argc);
        AddOutAnything("not matched");
    }

protected:
    typedef std::vector<Symbol> Tests;
    Tests tests;

    virtual bool CbMethodResort(int inlet,Symbol sym,int argc,const t_atom *argv)
    {
		const char *dst = GetString(sym);
		if(*dst != '/')
			return false;

		int i;
        for(i = 0; i < tests.size(); ++i)
            if(PatternMatch(dst,GetString(tests[i]))) {
                ToOutList(i,argc,argv);
                return true;
            }

        ToOutAnything(i,sym,argc,argv);
        return true;
    }
};

FLEXT_LIB_V("osc.route, osc",Route)

} // namespace
