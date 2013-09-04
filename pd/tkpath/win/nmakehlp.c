/* ----------------------------------------------------------------------------
 * nmakehlp.c --
 *
 *	This is used to fix limitations within nmake and the environment.
 *
 * Copyright (c) 2002 by David Gravereaux.
 * Copyright (c) 2003 by Patrick Thoyts
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * ----------------------------------------------------------------------------
 * RCS: @(#) $Id$
 * ----------------------------------------------------------------------------
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <shlwapi.h>
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "kernel32.lib")
#pragma comment (lib, "shlwapi.lib")
#include <stdio.h>
#if defined(_M_IA64) || defined(_M_AMD64)
#pragma comment(lib, "bufferoverflowU")
#endif

/* protos */

int		CheckForCompilerFeature(const char *option);
int		CheckForLinkerFeature(const char *option);
int		IsIn(const char *string, const char *substring);
int		GrepForDefine(const char *file, const char *string);
int		GetVersionFromHeader(const char *tclh, const char *tkh);
DWORD WINAPI	ReadFromPipe(LPVOID args);

/* globals */

#define CHUNK	25
#define STATICBUFFERSIZE    1000
typedef struct {
    HANDLE pipe;
    char buffer[STATICBUFFERSIZE];
} pipeinfo;

pipeinfo Out = {INVALID_HANDLE_VALUE, '\0'};
pipeinfo Err = {INVALID_HANDLE_VALUE, '\0'};

/*
 * exitcodes: 0 == no, 1 == yes, 2 == error
 */

int
main (int argc, char *argv[])
{
    char msg[300];
    DWORD dwWritten;
    int chars;

    /*
     * Make sure children (cl.exe and link.exe) are kept quiet.
     */

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    /*
     * Make sure the compiler and linker aren't effected by the outside world.
     */

    SetEnvironmentVariable("CL", "");
    SetEnvironmentVariable("LINK", "");

    if (argc > 1 && *argv[1] == '-') {
	switch (*(argv[1]+1)) {
	case 'c':
	    if (argc != 3) {
		chars = wnsprintf(msg, sizeof(msg)-1,
		        "usage: %s -c <compiler option>\n"
			"Tests for whether cl.exe supports an option\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    }
	    return CheckForCompilerFeature(argv[2]);
	case 'l':
	    if (argc != 3) {
		chars = wnsprintf(msg, sizeof(msg) - 1,
	       		"usage: %s -l <linker option>\n"
			"Tests for whether link.exe supports an option\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    }
	    return CheckForLinkerFeature(argv[2]);
	case 'f':
	    if (argc == 2) {
		chars = wnsprintf(msg, sizeof(msg) - 1,
			"usage: %s -f <string> <substring>\n"
			"Find a substring within another\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    } else if (argc == 3) {
		/*
		 * If the string is blank, there is no match.
		 */

		return 0;
	    } else {
		return IsIn(argv[2], argv[3]);
	    }
	case 'v':
	    if (argc != 4) {
		chars = wnsprintf(msg, sizeof(msg) - 1,
		    "usage: %s -v <tcl.h> <tk.h>\n"
		    "Search for versions from the tcl and tk headers.",
		    argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
		    &dwWritten, NULL);
		return 0;
	    }
	    return GetVersionFromHeader(argv[2], argv[3]);
	}
    }
    chars = wnsprintf(msg, sizeof(msg) - 1,
	    "usage: %s -c|-l|-f ...\n"
	    "This is a little helper app to equalize shell differences between WinNT and\n"
	    "Win9x and get nmake.exe to accomplish its job.\n",
	    argv[0]);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars, &dwWritten, NULL);
    return 2;
}

int
CheckForCompilerFeature(
    const char *option)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DWORD threadID, n;
    char msg[300];
    BOOL ok;
    HANDLE hProcess, h, pipeThreads[2];
    char cmdline[256];

    hProcess = GetCurrentProcess();

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdInput = INVALID_HANDLE_VALUE;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    /*
     * Create a non-inheritible pipe.
     */

    CreatePipe(&Out.pipe, &h, &sa, 0);

    /*
     * Dupe the write side, make it inheritible, and close the original.
     */

    DuplicateHandle(hProcess, h, hProcess, &si.hStdOutput, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Same as above, but for the error side.
     */

    CreatePipe(&Err.pipe, &h, &sa, 0);
    DuplicateHandle(hProcess, h, hProcess, &si.hStdError, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Base command line (use nmake environment)
     */

    n = GetEnvironmentVariable("CC", cmdline, 255);
    cmdline[n] = 0;
    if (n == 0)
	lstrcpy(cmdline, "cl.exe");

    strncat(cmdline, " -nologo -c -TC -Zs -X ", 255);

    /*
     * Append our option for testing
     */

    lstrcat(cmdline, option);

    /*
     * Filename to compile, which exists, but is nothing and empty.
     */

    lstrcat(cmdline, " .\\nul");

    ok = CreateProcess(
	    NULL,	    /* Module name. */
	    cmdline,	    /* Command line. */
	    NULL,	    /* Process handle not inheritable. */
	    NULL,	    /* Thread handle not inheritable. */
	    TRUE,	    /* yes, inherit handles. */
	    DETACHED_PROCESS, /* No console for you. */
	    NULL,	    /* Use parent's environment block. */
	    NULL,	    /* Use parent's starting directory. */
	    &si,	    /* Pointer to STARTUPINFO structure. */
	    &pi);	    /* Pointer to PROCESS_INFORMATION structure. */

    if (!ok) {
	DWORD err = GetLastError();
	int chars = wnsprintf(msg, sizeof(msg) - 1,
		"Tried to launch: \"%s\", but got error [%u]: ", cmdline, err);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|
		FORMAT_MESSAGE_MAX_WIDTH_MASK, 0L, err, 0, (LPVOID)&msg[chars],
		(300-chars), 0);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, lstrlen(msg), &err,NULL);
	return 2;
    }

    /*
     * Close our references to the write handles that have now been inherited.
     */

    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    WaitForInputIdle(pi.hProcess, 5000);
    CloseHandle(pi.hThread);

    /*
     * Start the pipe reader threads.
     */

    pipeThreads[0] = CreateThread(NULL, 0, ReadFromPipe, &Out, 0, &threadID);
    pipeThreads[1] = CreateThread(NULL, 0, ReadFromPipe, &Err, 0, &threadID);

    /*
     * Block waiting for the process to end.
     */

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    /*
     * Wait for our pipe to get done reading, should it be a little slow.
     */

    WaitForMultipleObjects(2, pipeThreads, TRUE, INFINITE);
    CloseHandle(pipeThreads[0]);
    CloseHandle(pipeThreads[1]);

#ifdef _DEBUG
    {
	DWORD err = 0;
	lstrcat(cmdline, "\n");
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), cmdline, 
	    lstrlen(cmdline), &err, NULL);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), Out.buffer, 
	    lstrlen(Out.buffer), &err,NULL);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), Err.buffer,
	    lstrlen(Err.buffer), &err,NULL);
    }
#endif

    /*
     * Look for the commandline warning code in both streams.
     *  - in MSVC 6 & 7 we get D4002, in MSVC 8 we get D9002.
     */

    return !(strstr(Out.buffer, "D4002") != NULL
             || strstr(Err.buffer, "D4002") != NULL
             || strstr(Out.buffer, "D9002") != NULL
             || strstr(Err.buffer, "D9002") != NULL);
}

int
CheckForLinkerFeature(
    const char *option)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DWORD threadID;
    char msg[300];
    BOOL ok;
    HANDLE hProcess, h, pipeThreads[2];
    char cmdline[100];

    hProcess = GetCurrentProcess();

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdInput = INVALID_HANDLE_VALUE;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    /*
     * Create a non-inheritible pipe.
     */

    CreatePipe(&Out.pipe, &h, &sa, 0);

    /*
     * Dupe the write side, make it inheritible, and close the original.
     */

    DuplicateHandle(hProcess, h, hProcess, &si.hStdOutput, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Same as above, but for the error side.
     */

    CreatePipe(&Err.pipe, &h, &sa, 0);
    DuplicateHandle(hProcess, h, hProcess, &si.hStdError, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Base command line.
     */

    lstrcpy(cmdline, "link.exe -nologo ");

    /*
     * Append our option for testing.
     */

    lstrcat(cmdline, option);

    ok = CreateProcess(
	    NULL,	    /* Module name. */
	    cmdline,	    /* Command line. */
	    NULL,	    /* Process handle not inheritable. */
	    NULL,	    /* Thread handle not inheritable. */
	    TRUE,	    /* yes, inherit handles. */
	    DETACHED_PROCESS, /* No console for you. */
	    NULL,	    /* Use parent's environment block. */
	    NULL,	    /* Use parent's starting directory. */
	    &si,	    /* Pointer to STARTUPINFO structure. */
	    &pi);	    /* Pointer to PROCESS_INFORMATION structure. */

    if (!ok) {
	DWORD err = GetLastError();
	int chars = wnsprintf(msg, sizeof(msg) - 1,
		"Tried to launch: \"%s\", but got error [%u]: ", cmdline, err);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|
		FORMAT_MESSAGE_MAX_WIDTH_MASK, 0L, err, 0, (LPVOID)&msg[chars],
		(300-chars), 0);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, lstrlen(msg), &err,NULL);
	return 2;
    }

    /*
     * Close our references to the write handles that have now been inherited.
     */

    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    WaitForInputIdle(pi.hProcess, 5000);
    CloseHandle(pi.hThread);

    /*
     * Start the pipe reader threads.
     */

    pipeThreads[0] = CreateThread(NULL, 0, ReadFromPipe, &Out, 0, &threadID);
    pipeThreads[1] = CreateThread(NULL, 0, ReadFromPipe, &Err, 0, &threadID);

    /*
     * Block waiting for the process to end.
     */

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    /*
     * Wait for our pipe to get done reading, should it be a little slow.
     */

    WaitForMultipleObjects(2, pipeThreads, TRUE, 500);
    CloseHandle(pipeThreads[0]);
    CloseHandle(pipeThreads[1]);

    /*
     * Look for the commandline warning code in the stderr stream.
     */

    return !(IsIn(Out.buffer, "LNK1117") ||
	     IsIn(Err.buffer, "LNK1117") ||
	     IsIn(Out.buffer, "LNK4044") ||
	     IsIn(Err.buffer, "LNK4044"));
}

#if 1
DWORD WINAPI
ReadFromPipe(
    LPVOID args)
{
    pipeinfo *pi = (pipeinfo *) args;
    char *lastBuf = pi->buffer;
    DWORD dwRead;
    BOOL ok;

  again:
    if (lastBuf - pi->buffer + CHUNK > STATICBUFFERSIZE) {
	CloseHandle(pi->pipe);
	return (DWORD)-1;
    }
    ok = ReadFile(pi->pipe, lastBuf, CHUNK, &dwRead, 0L);
    if (!ok || dwRead == 0) {
	CloseHandle(pi->pipe);
	return 0;
    }
    lastBuf += dwRead;
    goto again;

    return 0;  /* makes the compiler happy */
}
#else
DWORD WINAPI
ReadFromPipe (LPVOID args)
{
    pipeinfo *pi = (pipeinfo *) args;
    char *lastBuf = pi->buffer;
    DWORD dwRead;
    BOOL ok;

again:
    ok = ReadFile(pi->pipe, lastBuf, 25, &dwRead, 0L);
    if (!ok || dwRead == 0) {
        CloseHandle(pi->pipe);
        return 0;
    }
    lastBuf += dwRead;
    goto again;

    return 0;  /* makes the compiler happy */
}
#endif

int
IsIn (const char *string, const char *substring)
{
    return (strstr(string, substring) != NULL);
}

static double
ReadVersionFromHeader(const char *file, const char *macro)
{
    double d = 0.0;
    CHAR szBuffer[100];
    LPSTR p;
    DWORD cbBuffer = 100;
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
	while (fgets(szBuffer, cbBuffer, fp) != NULL) {
	    if ((p = strstr(szBuffer, macro)) != NULL) {
		while (*p && !isdigit(*p)) ++p;
		d = strtod(p, NULL);
		break;
	    }
	}
	fclose(fp);
    }
    return d;
}

int
GetVersionFromHeader(const char *tclh, const char *tkh)
{
    double dTcl = 0.0, dTk = 0.0;
    
    if (tclh != NULL)
	dTcl = ReadVersionFromHeader(tclh, "TCL_VERSION");
    if (tkh != NULL)
	dTk = ReadVersionFromHeader(tkh, "TK_VERSION");

    if (dTcl > 0 || dTk > 0) {
	FILE *ofp = fopen("version.vc", "w");
	if (dTcl > 0)
	    fprintf(ofp, "TCL_DOTVERSION\t= %0.1f\nTCL_VERSION\t= %u\n",
		    dTcl, (int)(dTcl * 10.0));
	if (dTk > 0)
	    fprintf(ofp, "TK_DOTVERSION\t= %0.1f\nTK_VERSION\t= %u\n",
		    dTk, (int)(dTk * 10.0));
	fclose(ofp);
	return 0;
    }
    return 1;
}
