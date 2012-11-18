/* Copyright (C) 2007 L. Donnie Smith <wiimote@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added param rules
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"
#include "util.h"
#include "y.tab.h"

#define step                              			\
    do {                                            \
        yylloc.first_line   = yylloc.last_line;     \
        yylloc.first_column = yylloc.last_column;   \
        yylloc.last_column += strlen(yytext);       \
    } while(0)

#define step_nl										\
	do {											\
		yylloc.first_line = yylloc.last_line;		\
		yylloc.first_column = yylloc.last_column;	\
		yylloc.last_line++;							\
		yylloc.last_column = 0;						\
	} while (0)

extern struct conf *cur_conf;
%}

WM			Wiimote\.
NC			Nunchuk\.
CC			Classic\.
PLUGIN		Plugin\.
ID			[[:alnum:]_][[:alnum:]_-]*
ACTION		(KEY|BTN|ABS|REL)_[[:alnum:]_]+
FILENAME	[[:alnum:]_.-]+
INT			[0-9]+
FLOAT		[0-9]+(\.[0-9]*)?([eE][+/-]?[0-9]+)?
BLANK		[[:blank:]]+

%option nounput
%x inc postinc
%%
^include		{
					yylloc.first_line = yylloc.last_line;
					yylloc.first_column = yylloc.last_column;
					yylloc.last_line++;
					yylloc.last_column = strlen(yytext) -1;
					BEGIN(inc);
				}
<inc>{BLANK}	{ step; }
<inc>{FILENAME}	{
					FILE *file;
					step;
					if ((file = conf_push_config(cur_conf, yytext, &yylloc))) {
						yypush_buffer_state(yy_create_buffer(file,
						                                     YY_BUF_SIZE));
						yylloc.first_line = yylloc.last_line = 0;
						yylloc.first_column = yylloc.last_column = 0;
					}
					BEGIN(INITIAL);
				}
<<EOF>>			{
					yypop_buffer_state();
					conf_pop_config(cur_conf, &yylloc);
					if (YY_CURRENT_BUFFER) {
						BEGIN(postinc);
					}
					else {
						yyterminate();
					}
				}
<postinc>\n		{ step_nl; BEGIN(INITIAL); }

#[^\n]*			{ step; }
<*>{BLANK}		{ step; }
[-\.=~]			{ step; return *yytext; }
\n				{ step_nl; return '\n'; }

{WM}Rumble		{ step; return WM_RUMBLE; }

{WM}Up			{ step;	yylval.Int = CONF_WM_BTN_UP;		return WM_BTN; }
{WM}Down		{ step;	yylval.Int = CONF_WM_BTN_DOWN;		return WM_BTN; }
{WM}Left		{ step;	yylval.Int = CONF_WM_BTN_LEFT;		return WM_BTN; }
{WM}Right		{ step;	yylval.Int = CONF_WM_BTN_RIGHT;		return WM_BTN; }
{WM}A			{ step;	yylval.Int = CONF_WM_BTN_A; 		return WM_BTN; }
{WM}B			{ step;	yylval.Int = CONF_WM_BTN_B;			return WM_BTN; }
{WM}Minus		{ step;	yylval.Int = CONF_WM_BTN_MINUS;		return WM_BTN; }
{WM}Plus		{ step;	yylval.Int = CONF_WM_BTN_PLUS;		return WM_BTN; }
{WM}Home		{ step;	yylval.Int = CONF_WM_BTN_HOME;		return WM_BTN; }
{WM}1			{ step;	yylval.Int = CONF_WM_BTN_1;			return WM_BTN; }
{WM}2			{ step;	yylval.Int = CONF_WM_BTN_2;			return WM_BTN; }

{NC}C			{ step;	yylval.Int = CONF_NC_BTN_C;			return NC_BTN; }
{NC}Z			{ step;	yylval.Int = CONF_NC_BTN_Z;			return NC_BTN; }

{CC}Up			{ step;	yylval.Int = CONF_CC_BTN_UP;		return CC_BTN; }
{CC}Down		{ step;	yylval.Int = CONF_CC_BTN_DOWN;		return CC_BTN; }
{CC}Left		{ step;	yylval.Int = CONF_CC_BTN_LEFT;		return CC_BTN; }
{CC}Right		{ step;	yylval.Int = CONF_CC_BTN_RIGHT;		return CC_BTN; }
{CC}Minus		{ step;	yylval.Int = CONF_CC_BTN_MINUS;		return CC_BTN; }
{CC}Plus		{ step;	yylval.Int = CONF_CC_BTN_PLUS;		return CC_BTN; }
{CC}Home		{ step;	yylval.Int = CONF_CC_BTN_HOME;		return CC_BTN; }
{CC}A			{ step;	yylval.Int = CONF_CC_BTN_A;			return CC_BTN; }
{CC}B			{ step;	yylval.Int = CONF_CC_BTN_B;			return CC_BTN; }
{CC}X			{ step;	yylval.Int = CONF_CC_BTN_X;			return CC_BTN; }
{CC}Y			{ step;	yylval.Int = CONF_CC_BTN_Y;			return CC_BTN; }
{CC}ZL			{ step;	yylval.Int = CONF_CC_BTN_ZL;		return CC_BTN; }
{CC}ZR			{ step;	yylval.Int = CONF_CC_BTN_ZR;		return CC_BTN; }
{CC}L			{ step;	yylval.Int = CONF_CC_BTN_L;			return CC_BTN; }
{CC}R			{ step;	yylval.Int = CONF_CC_BTN_R;			return CC_BTN; }


{WM}Dpad\.X		{ step;	yylval.Int = CONF_WM_AXIS_DPAD_X;	return AXIS; }
{WM}Dpad\.Y		{ step;	yylval.Int = CONF_WM_AXIS_DPAD_Y;	return AXIS; }

{NC}Stick\.X	{ step;	yylval.Int = CONF_NC_AXIS_STICK_X;	return AXIS; }
{NC}Stick\.Y	{ step;	yylval.Int = CONF_NC_AXIS_STICK_Y; 	return AXIS; }

{CC}Dpad\.X		{ step;	yylval.Int = CONF_CC_AXIS_DPAD_X;	return AXIS; }
{CC}Dpad\.Y		{ step;	yylval.Int = CONF_CC_AXIS_DPAD_Y;	return AXIS; }
{CC}LStick\.X	{ step;	yylval.Int = CONF_CC_AXIS_L_STICK_X;return AXIS; }
{CC}LStick\.Y	{ step;	yylval.Int = CONF_CC_AXIS_L_STICK_Y;return AXIS; }
{CC}RStick\.X	{ step;	yylval.Int = CONF_CC_AXIS_R_STICK_X;return AXIS; }
{CC}RStick\.Y	{ step;	yylval.Int = CONF_CC_AXIS_R_STICK_Y;return AXIS; }
{CC}LAnalog		{ step;	yylval.Int = CONF_CC_AXIS_L;		return AXIS; }
{CC}RAnalog		{ step;	yylval.Int = CONF_CC_AXIS_R;		return AXIS; }

{PLUGIN}		{ step; return PLUGIN; }

On				{ step; yylval.Int = -1;	return ON_OFF; }
Off				{ step; yylval.Int =  0;	return ON_OFF; }

{INT}			{ step; yylval.Int = atoi(yytext); return INT; }
{FLOAT}			{ step; yylval.Float = atof(yytext); return FLOAT; }

{ACTION}		{
					step;
					yylval.Int = lookup_action(yytext);
					if (yylval.Int == -1) {
						wminput_err("unknown identifier %s",
						            yytext);
					}
					else {
						switch (*yytext) {
						case 'K':
						case 'B':
							return BTN_ACTION;
							break;
						case 'A':
							return ABS_AXIS_ACTION;
							break;
						case 'R':
							return REL_AXIS_ACTION;
							break;
						}
					}
				}

{ID}			{
					step;
					yylval.String = malloc(strlen(yytext) + 1);
					if (!yylval.String) {
						wminput_err("error on malloc");
					}
					else {
						strcpy(yylval.String, yytext);
						return ID;
					}
				}

.				{ step;	wminput_err("unexpected character %c", *yytext); }
%%

int yywrap()
{
	return -1;
}

