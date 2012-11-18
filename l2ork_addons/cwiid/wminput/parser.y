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
#include <stdarg.h>
#include <stdio.h>
#include <linux/input.h>
#include "conf.h"
#include "util.h"

int yylex(void);
void yyerror(char const *, ...);

extern struct conf *cur_conf;
%}

%union {
	int Int;
	float Float;
	char *String;
}

%error-verbose
%locations

%token <Int> INT ON_OFF WM_BTN NC_BTN CC_BTN BTN_ACTION AXIS ABS_AXIS_ACTION
             REL_AXIS_ACTION
%token <Float> FLOAT
%token <String> ID
%token WM_RUMBLE PLUGIN

%start conf_list

%type <Int> sign pointer
%%

conf_list:
		/* empty */
	|	conf_list conf_line
;

conf_line:
		'\n'
	|	conf_item '\n'
;

conf_item:
		WM_RUMBLE '=' ON_OFF
			{ conf_ff(cur_conf, $3); }
	|	WM_BTN '=' BTN_ACTION
			{ conf_button(cur_conf, CONF_WM, $1, $3); }
	|	NC_BTN '=' BTN_ACTION
			{ conf_button(cur_conf, CONF_NC, $1, $3); }
	|	CC_BTN '=' BTN_ACTION
			{ conf_button(cur_conf, CONF_CC, $1, $3); }
	|	AXIS '=' sign pointer ABS_AXIS_ACTION
			{ conf_axis(cur_conf, $1, CONF_ABS, $5, $3 | $4); }
	|	AXIS '=' sign REL_AXIS_ACTION
			{ conf_axis(cur_conf, $1, CONF_REL, $4, $3); }
	|	PLUGIN ID '.' ID '=' BTN_ACTION
			{ conf_plugin_button(cur_conf, $2, $4, $6); }
	|	PLUGIN ID '.' ID '=' sign pointer ABS_AXIS_ACTION
			{ conf_plugin_axis(cur_conf, $2, $4, CONF_ABS, $8, $6 | $7); }
	|	PLUGIN ID '.' ID '=' sign REL_AXIS_ACTION
			{ conf_plugin_axis(cur_conf, $2, $4, CONF_REL, $7, $6); }
	|	PLUGIN ID '.' ID '=' INT
			{ conf_plugin_param_int(cur_conf, $2, $4, $6); }
	|	PLUGIN ID '.' ID '=' FLOAT
			{ conf_plugin_param_float(cur_conf, $2, $4, $6); }
;

sign:
		/* empty */
			{ $$ = 0; }
	|	'-'	{ $$ = CONF_INVERT; }
;

pointer:
		/* empty */
			{ $$ = 0; }
	|	'~'	{ $$ = CONF_POINTER; }
%%

void yyerror(char const *s, ...)
{
    va_list ap;

    va_start(ap, s);
    wminput_err("%s: line %d, column %d:", cur_conf->current_config_filename,
	            yylloc.first_line, yylloc.first_column);
    wminput_err((char *)s, ap);
    va_end(ap);
}

