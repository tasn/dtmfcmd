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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#include "control.h"
#include "general_functions.h"

#define IDENTIFY_STRING "identify"
#define EXIT_STRING "exit"


static int access_level = 0;

/**
 * get_access_level() - returns the current access level
 *
 * returns the current access level
 */
int 
get_access_level(void)
{
	return access_level;
}

/**
 * set_access_level() - sets the current access level
 * @level:      The level to set.
 *
 * sets the access_level
 */
void
set_access_level(int level)
{
	access_level = level;
}

/**
 * launch_command() - launches the app matching the command.
 * @rules:	A pointer to the rules to be matched to (assumed not null)
 * @command:	The command to match.
 *
 * On success it launches the app matching with the command
 * or if it complies to a special command (see enum rule_type
 * in control.h) it does the matching operation.
 * returns 0 on success -ERR on error.
 */
int
launch_command(rules_list *rules, const char *command)
{
	int n = g_queue_get_length(rules);
	struct rule_record *rule;
	int i;
	int found_command = 0;
	int ret_val = 0;
	
	for (i=0; i<n; i++) {
		rule = g_queue_peek_nth(rules, i);
		if (0 == strcmp(rule->code, command)) {
			/* correct command */
			
			/*check if it's one of the special commands */
			if (rule->type > 0) {
				if (rule->type == RULE_TYPE_IDENTIFY) {
					set_access_level(rule->level);
				}
				ret_val = rule->type;
				goto end;
			}
			else if(get_access_level() >= rule->level) {
				found_command = 1; /*mark that we launched something */
				printf("\nCode: %s Launching %s\n",rule->code, rule->launch);

				if (fork_exec(rule->launch)) {
					printf("Failed to launch, fork err!\n");
					ret_val = -2;
					goto end;
				}
	
			}
			else {
				/* access denied */
				ret_val = -2;
				goto end;
			}
		}
	}

	if (!found_command) {
		ret_val = -1; /* mark that we didn't launch anything */
	}
end:
	return ret_val;
}

static void
rule_record_init (struct rule_record *rule)
{
	rule->launch = NULL;
	rule->code = NULL;
	rule->level = 0;
	rule->type = RULE_TYPE_LAUNCH;
}

static void
rule_free (struct rule_record *rule)
{
	free (rule->launch);
	free (rule->code);
	free (rule);
}

/**
 * rules_list_init() - inits a pointer to a rules_list.
 * @rules:      A pointer to a pointer to a rules_list.
 *
 * initializes rules.
 */
void
rules_list_init (rules_list **rules)
{
	*rules = g_queue_new();
}

/**
 * rules_list_free() - frees the rules_list pointed by rules.
 * @rules:      A pointer to a rules_list.
 *
 * frees all the rules rules.
 */
void
rules_list_free (rules_list *rules)
{
	int n = g_queue_get_length(rules);
	struct rule_record *rule;
	int i;
	
	for (i=0; i<n; i++) {
		rule = g_queue_peek_nth(rules, i);
		rule_free(rule);
	}
	g_queue_free(rules);
}

/**
 * extract_rule_line() - load the rule record with the rule in the line
 * @line:	The config line to be extracted
 * @rule:	The rule record the will be loaded with the rule
 *
 * Extracts a rule from a single rule line.
 * returns 0 on success -ERR on error.
 */
static int
extract_rule_line(const char line[], struct rule_record *rule)
{
	int ret_val=0;
	regex_t re_ruleline;
	int matches_number = 3;
	regmatch_t matches[3 + 1]; /* add 1 for the whole regex */
	int start, length; /* positions to copy from */
	char *tmp_buf = NULL;
	
	if (0 != regcomp(&re_ruleline, 
			 "^\\s*([0-9]+)" /* code */
			 "\\s*=\\s*([0-9]+)\\s*:\\s*" /* level */
			 "\"([^\"]+)\"\\s*$", /* launch*/			 
			 REG_EXTENDED)) {

		/* error compiling regex */
		fprintf(stderr, "Can't compile regex\n");
		ret_val = -2;
		goto end;
	}
	
	if (0 != regexec(&re_ruleline, line, matches_number+1, matches, 0)) {
		ret_val = -1;		
		goto clean_1;
	}
	
	start = matches[1].rm_so;
	length = matches[1].rm_eo - start;
	rule->code = malloc(length+1);
	if (rule->code == NULL) {
		fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
		ret_val = -1;
		goto clean_1;
	}
	strncpy(rule->code, &line[start], length);
	rule->code[length]='\0';
	
	start = matches[2].rm_so;
	length = matches[2].rm_eo - start;
	tmp_buf = malloc(length+1);
	if (tmp_buf == NULL) {
		fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
		ret_val = -1;
		goto clean_code;
	}	
	strncpy(tmp_buf, &line[start], length);
	tmp_buf[length] = '\0';
	rule->level = atoi(tmp_buf);
	free(tmp_buf);
	
	start = matches[3].rm_so;
	length = matches[3].rm_eo - start;
	rule->launch = malloc(length+1);
	if (rule->code == NULL) {
		fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
		ret_val = -1;
		goto clean_code;
	}	
	strncpy(rule->launch, &line[start], length);
	rule->launch[length]='\0';
	
	if (0 == strcmp(rule->launch, IDENTIFY_STRING)) {
		rule->type = RULE_TYPE_IDENTIFY;
	}
	else if (0 == strcmp(rule->launch, EXIT_STRING)) {
		rule->type = RULE_TYPE_EXIT;
	}
	else {
		rule->type = RULE_TYPE_LAUNCH;
	}

	
clean_1:
	regfree(&re_ruleline);
end:
	return ret_val;
/* Error handling */

clean_code:
	free(rule->launch);

/* after cleaning after the errors, clean everything and exit the function */
	goto clean_1;
}

/**
 * load_rules() - load all the rules in the file filename to rules
 * @filename:	The rules filename
 * @rules:	A pointer to a rules list
 *
 * Loads all the rules in the file filename to rules.
 * returns 0 on success -ERR on error.
 */
int
load_rules (const char *filename, rules_list *rules)
{
	FILE *file;
	int success;
	int ret_val = 0;
	char line[MAX_LINE_LENGTH];
	struct rule_record *rule;
	regex_t re_ignore;
	regex_t re_include;
	int matches_number = 1;
	regmatch_t matches[1 + 1]; /* add 1 for the whole regex */
		
	if (0 != regcomp(&re_ignore, "^\\s*(#.*)?$", 
			 REG_EXTENDED)) {
		/* error compiling regex */
		fprintf(stderr, "Can't compile regex\n");
		ret_val = -4;
		goto finish;
	}
	
	
	if (0 != regcomp(&re_include, "^\\s*#include\\s*\"([^\"]+)\"\\s*$", 
			 REG_EXTENDED)) {
		/* error compiling regex */
		fprintf(stderr, "Can't compile regex\n");
		ret_val = -4;
		goto clean_regex_ignore;
	}
	
	if ((file = fopen(filename, "r")) == NULL) {
		/* error opening file */
		int errval = errno;
		fprintf(stderr, "Error opening file %s (%d)\n", filename, errval);
		if (errval == ENOENT) {
			ret_val = -1;
			goto clean_regex;
		}
		else {
			ret_val = -2;
			goto clean_regex;
		}
	}
	
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
		if (0 == regexec(&re_include, line, matches_number+1, matches, 0)) {
			/*include the config*/
			int start = matches[1].rm_so;
			int length = matches[1].rm_eo - start;
			char *include_file = malloc (length + 1);
			if (include_file == NULL) {
				fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
				ret_val = -3;
				goto clean_all;
			}
			strncpy(include_file, &line[start], length);
			include_file[length] = '\0';
			
			if (load_rules(include_file, rules)) {
				fprintf(stderr, "Error opening included config file (%s)\n",
						include_file);
			}
			
			free(include_file);
			continue; /* done handling this line */
		}
		
		if (0 == regexec(&re_ignore, line, 0, NULL, 0))
			continue; /* ignore this line */
		
		rule = malloc(sizeof (struct rule_record));
		if (rule == NULL) {
			fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
			ret_val = -3;
			goto clean_all;
		}
		
		rule_record_init(rule);
		success = extract_rule_line(line, rule);

		if (success == 0) {
			g_queue_push_tail(rules, rule);		
		}
		else {
			/* ignore bad config lines */
			fprintf(stderr, "Ignored bad config line: %s\n",line);
			free(rule);
		}
	}

clean_all:	
	fclose(file);
clean_regex:
	regfree(&re_include);
clean_regex_ignore:
	regfree(&re_ignore);
finish:
	return ret_val;
}
