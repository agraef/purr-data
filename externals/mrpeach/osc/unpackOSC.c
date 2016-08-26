/* unpackOSC is like dumpOSC but outputs a list consisting of a single symbol for the path  */
/* and a list of floats and/or symbols for the data, and adds an outlet for a time delay. */
/* This allows for the separation of the protocol and its transport. */
/* Started by Martin Peach 20060420 */
/* dumpOSC.c header follows: */
/*
Written by Matt Wright and Adrian Freed, The Center for New Music and
Audio Technologies, University of California, Berkeley.  Copyright (c)
1992,93,94,95,96,97,98,99,2000,01,02,03,04 The Regents of the University of
California (Regents).

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

/*

    dumpOSC.c
    server that displays OpenSoundControl messages sent to it
    for debugging client udp and UNIX protocol

    by Matt Wright, 6/3/97
    modified from dumpSC.c, by Matt Wright and Adrian Freed

    version 0.2: Added "-silent" option a.k.a. "-quiet"

    version 0.3: Incorporated patches from Nicola Bernardini to make
    things Linux-friendly.  Also added ntohl() in the right places
    to support little-endian architectures.

    to-do:

    More robustness in saying exactly what's wrong with ill-formed
    messages.  (If they don't make sense, show exactly what was
    received.)

    Time-based features: print time-received for each packet

    Clean up to separate OSC parsing code from socket/select stuff

    pd: branched from http://www.cnmat.berkeley.edu/OpenSoundControl/src/dumpOSC/dumpOSC.c
    -------------
    -- added pd functions
    -- socket is made differently than original via pd mechanisms
    -- tweaks for Win32    www.zeggz.com/raf	13-April-2002
    -- the OSX changes from cnmat didnt make it here yet but this compiles
        on OSX anyway.

*/
//define DEBUG
#include "packingOSC.h"


static t_class *unpackOSC_class;

typedef struct _unpackOSC
{
    t_object    x_obj;
    t_outlet    *x_data_out;
    t_outlet    *x_delay_out;
    int         x_bundle_flag;/* non-zero if we are processing a bundle */
    int         x_recursion_level;/* number of times we reenter unpackOSC_list */
    int         x_abort_bundle;/* non-zero if unpackOSC_list is not well formed */
    int         x_reentry_count;
} t_unpackOSC;

void unpackOSC_setup(void);
static void *unpackOSC_new(void);
static void unpackOSC_free(t_unpackOSC *x);
static void unpackOSC_list(t_unpackOSC *x, t_symbol *s, int argc, t_atom *argv);
static int unpackOSC_path(t_atom *data_at, char *path);
static void unpackOSC_Smessage(t_atom *data_at, int *data_atc, void *v, int n);
static void unpackOSC_PrintTypeTaggedArgs(t_atom *data_at, int *data_atc, void *v, int n);
static void unpackOSC_PrintHeuristicallyTypeGuessedArgs(t_atom *data_at, int *data_atc, void *v, int n, int skipComma);
static char *unpackOSC_DataAfterAlignedString(char *string, char *boundary);
static int unpackOSC_IsNiceString(char *string, char *boundary);
static t_float unpackOSC_DeltaTime(OSCTimeTag tt);

static void *unpackOSC_new(void)
{
    t_unpackOSC *x;
    x = (t_unpackOSC *)pd_new(unpackOSC_class);
    x->x_data_out = outlet_new(&x->x_obj, &s_list);
    x->x_delay_out = outlet_new(&x->x_obj, &s_float);
    x->x_bundle_flag = 0;
    x->x_recursion_level = 0;
    x->x_abort_bundle = 0;
    return (x);
}

static void unpackOSC_free(t_unpackOSC *x)
{
}

void unpackOSC_setup(void)
{
    unpackOSC_class = class_new(gensym("unpackOSC"),
        (t_newmethod)unpackOSC_new, (t_method)unpackOSC_free,
        sizeof(t_unpackOSC), 0, 0);
    class_addlist(unpackOSC_class, (t_method)unpackOSC_list);
}

/* unpackOSC_list expects an OSC packet in the form of a list of floats on [0..255] */
static void unpackOSC_list(t_unpackOSC *x, t_symbol *s, int argc, t_atom *argv) 
{
    int             size, messageLen, i, j;
    char            *messageName, *args, *buf;
    OSCTimeTag      tt;
    t_atom          data_at[MAX_MESG] ={{0}};/* symbols making up the path + payload */
    int             data_atc = 0;/* number of symbols to be output */
    char            raw[MAX_MESG];/* bytes making up the entire OSC message */
    int             raw_c;/* number of bytes in OSC message */

#ifdef DEBUG
    printf(">>> unpackOSC_list: %d bytes, abort=%d, reentry_count %d recursion_level %d\n",
        argc, x->x_abort_bundle, x->x_reentry_count, x->x_recursion_level);
#endif
    if(x->x_abort_bundle) return; /* if backing quietly out of the recursive stack */
    x->x_reentry_count++;
    if ((argc%4) != 0)
    {
        post("unpackOSC: Packet size (%d) not a multiple of 4 bytes: dropping packet", argc);
        goto unpackOSC_list_out;
    }
    if(argc > MAX_MESG)
    {
        post("unpackOSC: Packet size (%d) greater than max (%d). Change MAX_MESG and recompile if you want more.", argc, MAX_MESG);
        goto unpackOSC_list_out;
    }
    /* copy the list to a byte buffer, checking for bytes only */
    for (i = 0; i < argc; ++i)
    {
        if (argv[i].a_type == A_FLOAT)
        {
            j = (int)argv[i].a_w.w_float;
/*          if ((j == argv[i].a_w.w_float) && (j >= 0) && (j <= 255)) */
/* this can miss bytes between 128 and 255 because they are interpreted somewhere as negative */
/* , so change to this: */
            if ((j == argv[i].a_w.w_float) && (j >= -128) && (j <= 255))
            {
                raw[i] = (char)j;
            }
            else
            {
                post("unpackOSC: Data out of range (%d), dropping packet", argv[i].a_w.w_float);
                goto unpackOSC_list_out;
            }
        }
        else
        {
            post("unpackOSC: Data not float, dropping packet");
            goto unpackOSC_list_out;
        }
    }
    raw_c = argc;
    buf = raw;

    if ((argc >= 8) && (strncmp(buf, "#bundle", 8) == 0))
    { /* This is a bundle message. */
#ifdef DEBUG
        printf("unpackOSC: bundle msg:\n");
#endif

        if (argc < 16)
        {
            post("unpackOSC: Bundle message too small (%d bytes) for time tag", argc);
            goto unpackOSC_list_out;
        }

        x->x_bundle_flag = 1;

        /* Print the time tag */
#ifdef DEBUG
        printf("unpackOSC bundle timetag: [ %x.%0x\n", ntohl(*((uint32_t *)(buf+8))),
            ntohl(*((uint32_t *)(buf+12))));
#endif
/* convert the timetag into a millisecond delay from now */
        tt.seconds = ntohl(*((uint32_t *)(buf+8)));
        tt.fraction = ntohl(*((uint32_t *)(buf+12)));
        /* pd can use a delay in milliseconds */
        outlet_float(x->x_delay_out, unpackOSC_DeltaTime(tt));
        /* Note: if we wanted to actually use the time tag as a little-endian
          64-bit int, we'd have to word-swap the two 32-bit halves of it */

        i = 16; /* Skip "#group\0" and time tag */

        while(i < argc)
        {
            size = ntohl(*((int *) (buf + i)));
            if ((size % 4) != 0)
            {
                post("unpackOSC: Bad size count %d in bundle (not a multiple of 4)", size);
                goto unpackOSC_list_out;
            }
            if ((size + i + 4) > argc)
            {
                post("unpackOSC: Bad size count %d in bundle (only %d bytes left in entire bundle)",
                    size, argc-i-4);
                goto unpackOSC_list_out;
            }

            /* Recursively handle element of bundle */
            x->x_recursion_level++;
#ifdef DEBUG
            printf("unpackOSC: bundle depth %d\n", x->x_recursion_level);
#endif
            if (x->x_recursion_level > MAX_BUNDLE_NESTING)
            {
                post("unpackOSC: bundle depth %d exceeded", MAX_BUNDLE_NESTING);
                x->x_abort_bundle = 1;/* we need to back out of the recursive stack*/
                goto unpackOSC_list_out;
            }
#ifdef DEBUG
            printf("unpackOSC: bundle calling unpackOSC_list(x=%p, s=%s, size=%d, argv[%d]=%p)\n",
              x, s->s_name, size, i+4, &argv[i+4]);
#endif
            unpackOSC_list(x, s, size, &argv[i+4]);
            i += 4 + size;
        }

        if (i != argc)
        {
            post("unpackOSC: This can't happen");
        }

        x->x_bundle_flag = 0; /* end of bundle */
#ifdef DEBUG
        printf("unpackOSC: bundle end ] depth is %d\n", x->x_recursion_level);
#endif

    } 
    else if ((argc == 24) && (strcmp(buf, "#time") == 0))
    {
        post("unpackOSC: Time message: %s\n :).\n", buf);
        goto unpackOSC_list_out;
    }
    else
    { /* This is not a bundle message or a time message */

        messageName = buf;
#ifdef DEBUG
        printf("unpackOSC: message name string: %s length %d\n", messageName, raw_c);
#endif
        args = unpackOSC_DataAfterAlignedString(messageName, buf+raw_c);
        if (args == 0)
        {
            post("unpackOSC: Bad message name string: Dropping entire message.");
            goto unpackOSC_list_out;
        }
        messageLen = args-messageName;
        /* put the OSC path into a single symbol */
        data_atc = unpackOSC_path(data_at, messageName); /* returns 1 if path OK, else 0  */
        if (data_atc == 1)
        {
#ifdef DEBUG
            printf("unpackOSC_list calling unpackOSC_Smessage: message length %d\n", raw_c-messageLen);
#endif
            unpackOSC_Smessage(data_at, &data_atc, (void *)args, raw_c-messageLen);
            if (0 == x->x_bundle_flag)
                outlet_float(x->x_delay_out, 0); /* no delay for message not in a bundle */
        }
    }
    if (data_atc >= 1)
        outlet_anything(x->x_data_out, atom_getsymbol(data_at), data_atc-1, data_at+1);
    data_atc = 0;
    x->x_abort_bundle = 0;
unpackOSC_list_out:
    x->x_recursion_level = 0;
    x->x_reentry_count--;
}

static int unpackOSC_path(t_atom *data_at, char *path)
{
    int i;

    if (path[0] != '/')
    {
        for (i = 0; i < 16; ++i) if ('\0' == path[i]) break;
        path[i] = '\0';
        post("unpackOSC: Path doesn't begin with \"/\", dropping message");
        return 0;
    }
    for (i = 1; i < MAX_MESG; ++i)
    {
        if (path[i] == '\0')
        { /* the end of the path: turn path into a symbol */
            SETSYMBOL(data_at, gensym(path));
            return 1;
        }
    }
    post("unpackOSC: Path too long, dropping message");
    return 0;
}
#define SMALLEST_POSITIVE_FLOAT 0.000001f

static void unpackOSC_Smessage(t_atom *data_at, int *data_atc, void *v, int n)
{
    char   *chars = v;

    if (n != 0)
    {
        if (chars[0] == ',')
        {
            if (chars[1] != ',')
            {
#ifdef DEBUG
                printf("unpackOSC_Smessage calling unpackOSC_PrintTypeTaggedArgs: message length %d\n", n);
#endif
                /* This message begins with a type-tag string */
                unpackOSC_PrintTypeTaggedArgs(data_at, data_atc, v, n);
            }
            else
            {
#ifdef DEBUG
                printf("unpackOSC_Smessage calling unpackOSC_PrintHeuristicallyTypeGuessedArgs: message length %d, skipComma 1\n", n);
#endif
                /* Double comma means an escaped real comma, not a type string */
                unpackOSC_PrintHeuristicallyTypeGuessedArgs(data_at, data_atc, v, n, 1);
            }
        }
        else
        {
#ifdef DEBUG
            printf("unpackOSC_Smessage calling unpackOSC_PrintHeuristicallyTypeGuessedArgs: message length %d, skipComma 0\n", n);
#endif
            unpackOSC_PrintHeuristicallyTypeGuessedArgs(data_at, data_atc, v, n, 0);
        }
    }
}

static void unpackOSC_PrintTypeTaggedArgs(t_atom *data_at, int *data_atc, void *v, int n)
{ 
    char    *typeTags, *thisType, *p;
    int     myargc = *data_atc;
    t_atom  *mya = data_at;

    typeTags = v;

    if (!unpackOSC_IsNiceString(typeTags, typeTags+n))
    {
#ifdef DEBUG
            printf("unpackOSC_PrintTypeTaggedArgs not nice string\n");
#endif
        /* No null-termination, so maybe it wasn't a type tag
        string after all */
        unpackOSC_PrintHeuristicallyTypeGuessedArgs(data_at, data_atc, v, n, 0);
        return;
    }

#ifdef DEBUG
    printf("unpackOSC_PrintTypeTaggedArgs calling unpackOSC_DataAfterAlignedString %p to  %p\n", typeTags, typeTags+n);
#endif
    p = unpackOSC_DataAfterAlignedString(typeTags, typeTags+n);
#ifdef DEBUG
    printf("unpackOSC_PrintTypeTaggedArgs p is %p: ", p);
#endif
    if (p == NULL) return; /* malformed message */
    for (thisType = typeTags + 1; *thisType != 0; ++thisType)
    {
        switch (*thisType)
        {
            case 'b': /* blob: an int32 size count followed by that many 8-bit bytes */
            {
                int i, blob_bytes = ntohl(*((int *) p));
#ifdef DEBUG
                printf("blob: %u bytes\n", blob_bytes);
#endif
                p += 4;
                for (i = 0; i < blob_bytes; ++i, ++p, ++myargc)
                    SETFLOAT(mya+myargc,(*(unsigned char *)p));
                while (i%4)
                {
                    ++i;
                    ++p;
                }
                break;
            }
            case 'm': /* MIDI message is the next four bytes*/
            {
                int i;
#ifdef DEBUG
                printf("MIDI: 0x%08X\n", ntohl(*((int *) p)));
#endif
                for (i = 0; i < 4; ++i, ++p, ++myargc)
                    SETFLOAT(mya+myargc, (*(unsigned char *)p));
                break;
            }
            case 'i': case 'r': case 'c':
#ifdef DEBUG
                printf("integer: %d\n", ntohl(*((int *) p)));
#endif
                SETFLOAT(mya+myargc,(signed)ntohl(*((int *) p)));
                myargc++;
                p += 4;
                break;
            case 'f':
            {
                intfloat32 thisif;
                thisif.i = ntohl(*((int *) p));
#ifdef DEBUG
                printf("float: %f\n", thisif.f);
#endif
                SETFLOAT(mya+myargc, thisif.f);
                myargc++;
                p += 4;
                break;
            }
            case 'h': case 't':
#ifdef DEBUG
                printf("[A 64-bit int]\n");
#endif
                post("unpackOSC: PrintTypeTaggedArgs: [A 64-bit int] not implemented");
                p += 8;
                break;
            case 'd':
#ifdef DEBUG
                printf("[A 64-bit float]\n");
#endif
                post("unpackOSC: PrintTypeTaggedArgs: [A 64-bit float] not implemented");
                p += 8;
                break;
            case 's': case 'S':
                if (!unpackOSC_IsNiceString(p, typeTags+n))
                {
                    post("unpackOSC: PrintTypeTaggedArgs: Type tag said this arg is a string but it's not!\n");
                    return;
                }
                else
                {
#ifdef DEBUG
                    printf("string: \"%s\"\n", p);
#endif
                    SETSYMBOL(mya+myargc,gensym(p));
                    myargc++;
                    p = unpackOSC_DataAfterAlignedString(p, typeTags+n);

                }
                break;
            case 'T':
#ifdef DEBUG
                printf("[True]\n");
#endif
                SETFLOAT(mya+myargc,1.);
                myargc++;
                break;
            case 'F':
#ifdef DEBUG
                printf("[False]\n");
#endif
                SETFLOAT(mya+myargc,0.);
                myargc++;
                break;
            case 'N':
#ifdef DEBUG
                printf("[Nil]\n");
#endif
                SETFLOAT(mya+myargc,0.);
                myargc++;
                break;
            case 'I':
#ifdef DEBUG
                printf("[Infinitum]\n");
#endif
                SETSYMBOL(mya+myargc,gensym("INF"));
                myargc++;
                break;
            default:
#ifdef DEBUG
                printf("unpackOSC_PrintTypeTaggedArgs unknown typetag %c (%0X)\n", *thisType, *thisType);
#endif
                post("unpackOSC: PrintTypeTaggedArgs: [Unrecognized type tag %c]", *thisType);
                return;
         }
    }
    *data_atc = myargc;
}

static void unpackOSC_PrintHeuristicallyTypeGuessedArgs(t_atom *data_at, int *data_atc, void *v, int n, int skipComma)
{
    int         i;
    int         *ints;
    intfloat32  thisif;
    char        *chars, *string, *nextString;
    int         myargc = *data_atc;
    t_atom*     mya = data_at;

    /* Go through the arguments 32 bits at a time */
    ints = v;
    chars = v;

    for (i = 0; i < n/4; )
    {
        string = &chars[i*4];
        thisif.i = ntohl(ints[i]);
        /* Reinterpret the (potentially byte-reversed) thisif as a float */

        if (thisif.i >= -1000 && thisif.i <= 1000000)
        {
#ifdef DEBUG
            printf("%d ", thisif.i);
#endif
            SETFLOAT(mya+myargc,(t_float) (thisif.i));
            myargc++;
            i++;
        }
        else if (thisif.f >= -1000.f && thisif.f <= 1000000.f &&
            (thisif.f <=0.0f || thisif.f >= SMALLEST_POSITIVE_FLOAT))
        {
#ifdef DEBUG
            printf("%f ",  thisif.f);
#endif
            SETFLOAT(mya+myargc,thisif.f);
            myargc++;
            i++;
        }
        else if (unpackOSC_IsNiceString(string, chars+n))
        {
            nextString = unpackOSC_DataAfterAlignedString(string, chars+n);
#ifdef DEBUG
            printf("\"%s\" ", (i == 0 && skipComma) ? string +1 : string);
#endif
            SETSYMBOL(mya+myargc,gensym(string));
            myargc++;
            i += (nextString-string) / 4;
        }
        else
        {
            /* unhandled .. ;) */
            post("unpackOSC: PrintHeuristicallyTypeGuessedArgs: indeterminate type: 0x%x xx", ints[i]);
            i++;
        }
        *data_atc = myargc;
    }
}

#define STRING_ALIGN_PAD 4

static char *unpackOSC_DataAfterAlignedString(char *string, char *boundary) 
{
    /* The argument is a block of data beginning with a string.  The
        string has (presumably) been padded with extra null characters
        so that the overall length is a multiple of STRING_ALIGN_PAD
        bytes.  Return a pointer to the next byte after the null
        byte(s).  The boundary argument points to the character after
        the last valid character in the buffer---if the string hasn't
        ended by there, something's wrong.

        If the data looks wrong, return 0 */

    int i;

#ifdef DEBUG
    printf("unpackOSC_DataAfterAlignedString boundary - string = %ld\n",  boundary-string);
#endif
    if ((boundary - string) %4 != 0)
    {
        post("unpackOSC: DataAfterAlignedString: bad boundary");
        return 0;
    }

    for (i = 0; string[i] != '\0'; i++)
    {
#ifdef DEBUG
        printf("%0X(%c) ",  string[i], string[i]);
#endif
        if (string + i >= boundary)
        {
            post("unpackOSC: DataAfterAlignedString: Unreasonably long string");
            return 0;
        }
    }

    /* Now string[i] is the first null character */
#ifdef DEBUG
    printf("\nunpackOSC_DataAfterAlignedString first null character at %p\n",  &string[i]);
#endif
     i++;

    for (; (i % STRING_ALIGN_PAD) != 0; i++)
    {
        if (string + i >= boundary)
        {
            post("unpackOSC: DataAfterAlignedString: Unreasonably long string");
            return 0;
        }
        if (string[i] != '\0')
        {
            post("unpackOSC: DataAfterAlignedString: Incorrectly padded string");
            return 0;
        }
    }
#ifdef DEBUG
    printf("unpackOSC_DataAfterAlignedString first non-null character at %p\n",  &string[i]);
#endif
 
    return string+i;
}

static int unpackOSC_IsNiceString(char *string, char *boundary) 
{
    /* Arguments same as DataAfterAlignedString().  Is the given "string"
       really an OSC-string?  I.e., is it 
        "A sequence of non-null ASCII characters followed by a null, followed 
        by 0-3 additional null characters to make the total number of bits a 
        multiple of 32"? (OSC 1.0)
        Returns 1 if true, else 0. */

    int i;

    if ((boundary - string) %4 != 0)
    {
        post("unpackOSC: IsNiceString: bad boundary\n");
        return 0;
    }

    /* any non-zero byte will be accepted, so UTF-8 sequences will also be accepted -- not strictly OSC v1.0 */
    for (i = 0; string[i] != '\0'; i++)
        /* if ((!isprint(string[i])) || (string + i >= boundary)) return 0; */ /* only ASCII printable chars */
        /*if ((0==(string[i]&0xE0)) || (string + i >= boundary)) return 0;*/ /* onl;y ASCII space (0x20) and above */
        if (string + i >= boundary) return 0; /* anything non-zero */
    /* If we made it this far, it's a null-terminated sequence of characters
       within the given boundary.  Now we just make sure it's null padded... */

    /* Now string[i] is the first null character */
    i++;
    for (; (i % STRING_ALIGN_PAD) != 0; i++)
        if (string[i] != '\0') return 0;

    return 1;
}

#define SECONDS_FROM_1900_to_1970 2208988800LL /* 17 leap years */
#define TWO_TO_THE_32_OVER_ONE_MILLION 4295LL
#define ONE_MILLION_OVER_TWO_TO_THE_32 0.00023283064365386963

/* return the time difference in milliseconds between an OSC timetag and now */
static t_float unpackOSC_DeltaTime(OSCTimeTag tt)
{
    static double onemillion = 1000000.0f;
#ifdef _WIN32
    static double onethousand = 1000.0f;
#endif /* ifdef _WIN32 */

    if (tt.fraction == 1 && tt.seconds == 0) return 0.0; /* immediate */
    else
    {
        OSCTimeTag ttnow;
        double  ttusec, nowusec, delta;
#ifdef _WIN32
        struct _timeb tb;

        _ftime(&tb); /* find now */
        /* First get the seconds right */
        ttnow.seconds = (unsigned) SECONDS_FROM_1900_to_1970 + (unsigned) tb.time;
        /* find usec in tt */
        ttusec = tt.seconds*onemillion + ONE_MILLION_OVER_TWO_TO_THE_32*tt.fraction;
        nowusec = ttnow.seconds*onemillion + tb.millitm*onethousand;
#else
        struct timeval tv;
        struct timezone tz;

        gettimeofday(&tv, &tz); /* find now */
        /* First get the seconds right */
        ttnow.seconds = (unsigned) SECONDS_FROM_1900_to_1970 + (unsigned) tv.tv_sec;
        /* find usec in tt */
        ttusec = tt.seconds*onemillion + ONE_MILLION_OVER_TWO_TO_THE_32*tt.fraction;
        nowusec = ttnow.seconds*onemillion + tv.tv_usec;
#endif /* ifdef _WIN32 */
        /* subtract now from tt to get delta time */
        /* if (ttusec < nowusec) return 0.0; */
        /*negative delays are all right */
        delta = ttusec - nowusec;
        return (float)(delta*0.001f);
    }
}
/* end of unpackOSC.c */
