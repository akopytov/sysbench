/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2018 Alexey Kopytov <akopytov@gmail.com>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _WIN32
# include "sb_win.h"
#endif

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include "sb_options.h"
#include "sysbench.h"

#define VALUE_DELIMITER '='
#define VALUE_SEPARATOR ','
#define COMMENT_CHAR '#'

#define MAXSTRLEN 256

/* Global options list */
static sb_list_t options;

/* List of size modifiers (kilo, mega, giga, tera) */
static const char sizemods[] = "KMGT";

/* Convert dashes to underscores in option names */

static void convert_dashes(char *);

/* Compare option names */

static int opt_name_cmp(const char *, const char *);

/*
  Array of option formats as displayed by sb_print_options(). The order and
  number of elements must match sb_arg_type_t!
*/
static char *opt_formats[] = {
  NULL,			/* SB_ARG_TYPE_NULL */
  "[=on|off]",		/* SB_ARG_TYPE_FLAG */
  "=N",			/* SB_ARG_TYPE_INT */
  "=SIZE",		/* SB_ARG_TYPE_SIZE */
  "=N",			/* SB_ARG_TYPE_DOUBLE */
  "=STRING",    	/* SB_ARG_TYPE_STRING */
  "=[LIST,...]",	/* SB_ARG_TYPE_LIST */
  "=FILENAME"		/* SB_ARG_TYPE_FILE */
};

/* Initialize options library */


int sb_options_init(void)
{
  SB_LIST_INIT(&options);

  return 0;
}

/* Release resource allocated by the options library */

int sb_options_done(void)
{
  free_options(&options);

  return 0;
}


/* Register set of command line arguments */


int sb_register_arg_set(sb_arg_t *set)
{
  unsigned int i;

  for (i=0; set[i].name != NULL; i++)
  {
    option_t * const opt = set_option(set[i].name, set[i].value, set[i].type);
    if (opt == NULL)
      return 1;
    opt->validate = set->validate;
  }

  return 0;
}

option_t *sb_find_option(const char *name)
{
  return find_option(&options, name);
}

static void read_config_file(const char *filename)
{
  /* read config options from file */
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror(filename);
  } else {
    read_config(fp, &options);
    fclose(fp);
  }
}

option_t *set_option(const char *name, const char *value, sb_arg_type_t type)
{
  option_t *opt;
  char     *tmpbuf;
  char     *tmp;

  opt = add_option(&options, name);
  if (opt == NULL)
    return NULL;
  free_values(&opt->values);
  opt->type = type;

  if (opt->validate != NULL && !opt->validate(name, value))
    return NULL;

  if (type != SB_ARG_TYPE_BOOL && (value == NULL || value[0] == '\0'))
    return opt;

  switch (type) {
    case SB_ARG_TYPE_BOOL:
      if (value == NULL || !strcmp(value, "on") ||
          !strcmp(value, "true") || !strcmp(value, "1"))
      {
        add_value(&opt->values, "on");
      }
      else if (strcmp(value, "off") && strcmp(value, "false") &&
               strcmp(value, "0"))
      {
        return NULL;
      }
      break;
    case SB_ARG_TYPE_INT:
    case SB_ARG_TYPE_SIZE:
    case SB_ARG_TYPE_DOUBLE:
    case SB_ARG_TYPE_STRING:
      add_value(&opt->values, value);
      break;
    case SB_ARG_TYPE_LIST:
      if (value == NULL)
        break;

      tmpbuf = strdup(value);
      tmp = tmpbuf;

      for (tmp = strtok(tmp, ","); tmp != NULL; tmp = strtok(NULL, ","))
        add_value(&opt->values, tmp);

      free(tmpbuf);

      break;
    case SB_ARG_TYPE_FILE:
      read_config_file(value);
      break;
    default:
      printf("Unknown argument type: %d", type);
      return NULL;
  }

  return opt;
}


void sb_print_options(sb_arg_t *opts)
{
  unsigned int i;
  unsigned int len;
  unsigned int maxlen;
  char         *fmt;
  
  /* Count the maximum name length */
  for (i = 0, maxlen = 0; opts[i].name != NULL; i++)
  {
    len = strlen(opts[i].name);
    len += (opts[i].type < SB_ARG_TYPE_MAX) ?
      strlen(opt_formats[opts[i].type]) : 8 /* =UNKNOWN */;
    if (len > maxlen)
      maxlen = len;
  }

  for (i = 0; opts[i].name != NULL; i++)
  {
    if (opts[i].type < SB_ARG_TYPE_MAX)
      fmt = opt_formats[opts[i].type];
    else
      fmt = "=UNKNOWN";

    printf("  --%s%-*s%s", opts[i].name,
           (int)(maxlen - strlen(opts[i].name) + 1), fmt,
           opts[i].desc);
    if (opts[i].value != NULL)
      printf(" [%s]", opts[i].value);
    printf("\n");
  }
  printf("\n");
}


int sb_opt_to_flag(option_t *opt)
{
  return !SB_LIST_IS_EMPTY(&opt->values);
}


int sb_get_value_flag(const char *name)
{
  option_t *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  return sb_opt_to_flag(opt);
}


int sb_opt_to_int(option_t *opt)
{
  value_t        *val;
  sb_list_item_t *pos;
  long           res;
  char           *endptr;

  SB_LIST_ONCE(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    res = strtol(val->data, &endptr, 10);
    if (*endptr != '\0' || res > INT_MAX || res < INT_MIN)
    {
      fprintf(stderr, "Invalid value for the '%s' option: '%s'\n",
              opt->name, val->data);
      exit(EXIT_FAILURE);
    }
    return (int) res;
  }

  return 0;
}  


int sb_get_value_int(const char *name)
{
  option_t       *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  return sb_opt_to_int(opt);;
}


unsigned long long sb_opt_to_size(option_t *opt)
{
  value_t             *val;
  sb_list_item_t      *pos;
  unsigned long long  res = 0;
  char                mult = 0;
  int                 rc;
  unsigned int        i, n;
  char                *c;

  SB_LIST_ONCE(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    /*
     * Reimplentation of sscanf(val->data, "%llu%c", &res, &mult), since
     * there is no standard on how to specify long long values
     */
    res = 0;
    for (rc = 0, c = val->data; *c != '\0'; c++)
    {
      if (*c < '0' || *c > '9')
      {
        if (rc == 1)
        {
          rc = 2;
          mult = *c;
        }
        break;
      }
      rc = 1;
      res = res * 10 + *c - '0';
    }

    if (rc == 2)
    {
      for (n = 0; sizemods[n] != '\0'; n++)
        if (toupper(mult) == sizemods[n])
          break;
      if (sizemods[n] != '\0')
      {
        for (i = 0; i <= n; i++)
          res *= 1024;
      }
      else
        res = 0; /* Unknown size modifier */
    }
  }

  return res;
}


unsigned long long sb_get_value_size(const char *name)
{
  option_t            *opt;
 
  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  return sb_opt_to_size(opt);
}


double sb_opt_to_double(option_t *opt)
{
  value_t        *val;
  sb_list_item_t *pos;
  double          res = 0;

  SB_LIST_FOR_EACH(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    res = strtod(val->data, NULL);
  }

  return res;
}


double sb_get_value_double(const char *name)
{
  option_t       *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  return sb_opt_to_double(opt);
}


char *sb_opt_to_string(option_t *opt)
{
  value_t        *val;
  sb_list_item_t *pos;

  SB_LIST_ONCE(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    return val->data;
  }

  return NULL;
}


char *sb_get_value_string(const char *name)
{
  option_t       *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return NULL;

  return sb_opt_to_string(opt);
}


bool sb_opt_copy(const char *to, const char *from)
{
  option_t *opt = find_option(&options, from);

  if (opt == NULL)
    return false;

  set_option(to, sb_opt_to_string(opt), opt->type);

  return true;
}


sb_list_t *sb_opt_to_list(option_t *opt)
{
  return &opt->values;
}


sb_list_t *sb_get_value_list(const char *name)
{
  option_t       *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return NULL;

  return sb_opt_to_list(opt);
}


char *sb_print_value_size(char *buf, unsigned int buflen, double value)
{
  unsigned int i;

  for (i = 0; i < sizeof(sizemods) && value >= 1024; i++, value /= 1024)
    /* empty */ ;

  if (i > 0)
    snprintf(buf, buflen, "%.5g%ci", value, sizemods[i-1]);
  else
    snprintf(buf, buflen, "%.5g", value);

  return buf;
}


value_t *new_value()
{
  value_t *newval;
  
  newval = (value_t *)malloc(sizeof(value_t));
  if (newval != NULL)
    memset(newval, 0, sizeof(value_t));

  return newval;
}


option_t *new_option()
{
  option_t *newopt;
  
  newopt = (option_t *)malloc(sizeof(option_t));
  if (newopt != NULL)
  {
    memset(newopt, 0, sizeof(option_t));
    SB_LIST_INIT(&newopt->values);
  }
  
  return newopt;
}


void free_values(sb_list_t *values)
{
  sb_list_item_t *next;
  sb_list_item_t *cur;
  value_t        *val;

  if (values == NULL)
    return;

  SB_LIST_FOR_EACH_SAFE(cur, next, values)
    {
    val = SB_LIST_ENTRY(cur, value_t, listitem);
    if (val->data != NULL)
      free(val->data);
    SB_LIST_DELETE(cur);
    free(val);
  }
}


void free_options(sb_list_t *options)
{
  sb_list_item_t *next;
  sb_list_item_t *cur;
  option_t       *opt;

  if (options == NULL)
    return;

  SB_LIST_FOR_EACH_SAFE(cur, next, options)
  {
    opt = SB_LIST_ENTRY(cur, option_t, listitem);
    if (opt->name != NULL)
      free(opt->name);
    free_values(&opt->values);
    free(opt);
  }
}  


int remove_value(sb_list_t *values, char *valname)
{
  value_t *  value;
  
  if (values == NULL || valname == NULL)
    return 1;

  if ((value = find_value(values, valname)) == NULL)
    return 1;
  if (value->data != NULL)
  {
    free(value->data);
  }
  SB_LIST_DELETE(&value->listitem);
  free(value);
  
  return 0;
}


int remove_option(sb_list_t * options, char * optname)
{
  option_t *  option;
  
  if (options == NULL || optname == NULL)
    return 1;

  if ((option = find_option(options, optname)) == NULL)
    return 1;
  free_values(&option->values);
  if (option->name != NULL)
    free(option->name);
  SB_LIST_DELETE(&option->listitem);
  free(option);
  
  return 0;
} 


value_t *add_value(sb_list_t *values, const char *data)
{
  value_t *newval;
  
  if (values == NULL || data == NULL)
    return NULL;
  
  if ((newval = new_value()) == NULL)
    return NULL;
  if ((newval->data = strdup(data)) == NULL)
  {
    free(newval);
    return NULL;
  }
  
  SB_LIST_ADD_TAIL(&newval->listitem, values);
  
  return newval;
}


value_t *find_value(sb_list_t *values, const char *data)
{
  sb_list_item_t *pos;
  value_t        *value; 
  
  if (values == NULL || data == NULL)
    return NULL;

  SB_LIST_FOR_EACH(pos, values)
  {
    value = SB_LIST_ENTRY(pos, value_t, listitem);
    if (!strcmp(value->data, data))
      return value;
  }
  
  return NULL;
}


option_t *add_option(sb_list_t *options, const char *name)
{
  option_t *option;
  
  if (options == NULL || name == NULL)
    return NULL;

  if ((option = find_option(options, name)) != NULL)
    return option; 

  if ((option = new_option()) == NULL)
    return NULL;

  option->name = strdup(name);
  convert_dashes(option->name);
  
  SB_LIST_ADD_TAIL(&option->listitem, options);

  return option;
}


void convert_dashes(char *s)
{
  while (*s != '\0')
  {
    if (*s == '-')
      *s = '_';
    s++;
  }
}


int opt_name_cmp(const char *s1, const char *s2)
{
  for (/* empty */; *s1 != '\0'; s1++, s2++)
  {
    if (*s1 == *s2)
      continue;

    if ((*s1 != '-' && *s1 != '_') || (*s2 != '-' && *s2 != '_'))
      break;
  }

  return *s1 - *s2;
}


option_t *find_option(sb_list_t *options, const char *name)
{
  sb_list_item_t *pos;
  option_t       *opt;    
  
  if (options == NULL || name == NULL)
    return NULL;

  SB_LIST_FOR_EACH(pos, options)
  {
    opt = SB_LIST_ENTRY(pos, option_t, listitem);
    if (!opt_name_cmp(opt->name, name))
      return opt;
  }
  
  return NULL;
}


sb_list_item_t *sb_options_enum_start(void)
{
  return SB_LIST_ENUM_START(&options);
}

sb_list_item_t *sb_options_enum_next(sb_list_item_t *pos, option_t **opt)
{
  pos = SB_LIST_ENUM_NEXT(pos, &options);
  if (pos == NULL)
    return NULL;

  *opt = SB_LIST_ENTRY(pos, option_t, listitem);

  return pos;
}


sb_list_t *read_config(FILE *fp, sb_list_t *options)
{
  char        buf[MAXSTRLEN];
  char        *tmp;
  char        qc;
  option_t    *newopt;
  int         optlen;
  int         nline;
  
  if (fp == NULL || options == NULL)
    return NULL;
  
  nline = 0;
  while (fgets(buf, MAXSTRLEN, fp) != NULL)
  {
    nline++;

    tmp = strchr(buf, VALUE_DELIMITER);
    if (tmp == NULL)
      continue;
    if (*tmp != '\0')
      *tmp++ = '\0';

    if ((newopt = add_option(options, buf)) == NULL)
      return NULL;

    free_values(&newopt->values);
    while (*tmp != '\0')
    {
      if (isspace((int)*tmp))
      {
        tmp++;
        continue;
      }
      if (*tmp == COMMENT_CHAR)
        break;
      else if (*tmp == '\'' || *tmp == '\"')
      {
        qc = *tmp;
        for (tmp++, optlen = 0; tmp[optlen] != '\0' && tmp[optlen] != qc;
             optlen++)
        {
          /* Empty */
        }
        if (tmp[optlen] == '\0') {
          fprintf(stderr, "unexpected EOL on line %d\n", nline);
          return NULL;
        }
        tmp[optlen++] = '\0';
        add_value(&newopt->values, tmp);
        for (tmp = tmp + optlen; *tmp != '\0' && *tmp != VALUE_SEPARATOR; tmp++)
        {
          /* Empty */
        }
        if (*tmp == VALUE_SEPARATOR)
          tmp++;
      } else {
        for (optlen = 0; tmp[optlen] != '\0' && tmp[optlen] != VALUE_SEPARATOR
                         && !isspace(tmp[optlen]);
             optlen++)
        {
          /* Empty */
        }

        if (tmp[optlen] != '\0')
          tmp[optlen++] = '\0';

        add_value(&newopt->values, tmp);
        tmp += optlen;
      }
    }
  }

  return options;
}


int write_config(FILE *fp, sb_list_t *options)
{
  option_t       *opt;
  value_t        *val;
  sb_list_item_t *pos_opt;
  sb_list_item_t *pos_val;

  if (fp == NULL || options == NULL)
    return 1;

  SB_LIST_FOR_EACH(pos_opt, options)
  {
    opt = SB_LIST_ENTRY(pos_opt, option_t, listitem);
    if (opt->ignore || opt->name == NULL)
      continue;
    opt->ignore = 1;
    fprintf(fp, "%s %c ", opt->name, VALUE_DELIMITER);
    SB_LIST_FOR_EACH(pos_val, &opt->values)
    {
      val = SB_LIST_ENTRY(pos_val, value_t, listitem);
      if (!val->ignore && val->data != NULL)
        fprintf(fp, "%s", val->data);
      if (!SB_LIST_ITEM_LAST(pos_val, &opt->values))
        fprintf(fp, "%c ", VALUE_SEPARATOR);
    }
    fputc('\n', fp);
  }
    
  return 0;
}
