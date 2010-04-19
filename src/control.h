/*    dtmfcmd is a program to detect dtmf tones and launch commands accordingly
 *    Copyright (C) 2009  Tom Hacohen (tom@stosb.com)
 *
 *    This file is a part of dtmfcmd   
 * 
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#define MAX_COMMAND_LENGTH 30
#define MAX_LINE_LENGTH 100

typedef GQueue rules_list;

/**
 * enum rule_type - the type of the rule
 * @RULE_TYPE_LAUNCH:	if this is a launcher rule.
 * @RULE_TYPE_IDENTIFY:	if this is an identify rule.
 * @RULE_TYPE_EXIT:	if this rule will exit the the program.
 *
 * the type of the rule (see struct rule_record)
 * launch must be first and 0, others must be > 0 
 */
enum rule_type {
	RULE_TYPE_LAUNCH=0,
	RULE_TYPE_IDENTIFY,
	RULE_TYPE_EXIT
};

/**
 * struct rule_record - a basic rule_record
 * @launch:	the command to launch.
 * @code:	the rule's code.
 * @level:      the level of the rule.
 * @type:       the type of the rule (see enum rule_type)
 *
 * a basic rule record.
 * when the code is matched it'll either launch the command
 * in launch or do something complying the type of the rule.
 */
struct rule_record {
	char *launch;
	char *code;
	int level;
	enum rule_type type;
};

int
launch_command(rules_list *rules, const char * command);

int
load_rules (const char *filename, rules_list *rules);

void
rules_list_init (rules_list **rules);

void
rules_list_free (rules_list *rules);

int 
get_access_level(void);

void
set_access_level(int level);

