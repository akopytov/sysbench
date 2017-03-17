/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2017 Alexey Kopytov <akopytov@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>
#include <stdbool.h>

#include "sb_list.h"

/* Helper option declaration macros */
#define SB_OPT(n, d, v, t)                   \
  { .name = (n),                             \
    .desc = (d),                             \
    .type = SB_ARG_TYPE_##t,                 \
    .value = (v) }

#define SB_OPT_END { .type = SB_ARG_TYPE_NULL }

/* Option types definition */

typedef enum
{
  SB_ARG_TYPE_NULL,
  SB_ARG_TYPE_BOOL,
  SB_ARG_TYPE_INT,
  SB_ARG_TYPE_SIZE,
  SB_ARG_TYPE_DOUBLE,
  SB_ARG_TYPE_STRING,
  SB_ARG_TYPE_LIST,
  SB_ARG_TYPE_FILE,
  SB_ARG_TYPE_MAX
} sb_arg_type_t;

/* Option validation function */
typedef bool sb_opt_validate_t(const char *, const char *);

/* Test option definition */
typedef struct
{
  const char         *name;
  const char         *desc;
  const char         *value;
  sb_arg_type_t      type;
  sb_opt_validate_t  *validate;
} sb_arg_t;

typedef struct
{
  char            *data;
  char            ignore;

  sb_list_item_t  listitem;
} value_t;

typedef struct
{
  char            *name;
  sb_arg_type_t   type;
  sb_list_t       values;
  char            ignore;

  sb_opt_validate_t *validate;

  sb_list_item_t  listitem;
} option_t;
  
/* Initilize options library */
int sb_options_init(void);

/* Release resource allocated by the options library */
int sb_options_done(void);

/* Register set of command line arguments */
int sb_register_arg_set(sb_arg_t *set);

/* Set value 'value' of type 'type' for option 'name' */
option_t *set_option(const char *name, const char *value, sb_arg_type_t type);

/* Find option specified by 'name' */
option_t *sb_find_option(const char *name);

/* Print list of options specified by 'opts' */
void sb_print_options(sb_arg_t *opts);

int sb_get_value_flag(const char *name);

int sb_get_value_int(const char *name);

unsigned long long sb_get_value_size(const char *name);

double sb_get_value_double(const char *name);

char *sb_get_value_string(const char *name);

sb_list_t *sb_get_value_list(const char *name);

char *sb_print_value_size(char *buf, unsigned int buflen, double value);

int sb_opt_to_flag(option_t *);

int sb_opt_to_int(option_t *);

unsigned long long sb_opt_to_size(option_t *);

double sb_opt_to_double(option_t *);

char *sb_opt_to_string(option_t *);

sb_list_t *sb_opt_to_list(option_t *);

bool sb_opt_copy(const char *to, const char *from);

sb_list_item_t *sb_options_enum_start(void);

sb_list_item_t *sb_options_enum_next(sb_list_item_t *, option_t **);

value_t *new_value(void);

option_t *new_option(void);

void free_values(sb_list_t *);

void free_options(sb_list_t *);

value_t *add_value(sb_list_t *, const char *);

value_t *find_value(sb_list_t *, const char *);

option_t *add_option(sb_list_t *, const char *);

option_t *find_option(sb_list_t *, const char *);

int remove_value(sb_list_t *, char *);

int remove_option(sb_list_t *, char *);
    
sb_list_t *read_config(FILE *, sb_list_t *);

int write_config(FILE *, sb_list_t *);

#endif /* OPTIONS_H */

