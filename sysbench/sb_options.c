/* Copyright (C) 2004 MySQL AB

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "sb_options.h"

#define VALUE_DELIMITER '='
#define VALUE_SEPARATOR ','
#define COMMENT_CHAR '#'

#define MAXSTRLEN 256

/* Global options list */
static sb_list_t options;

/* List of size modifiers (kilo, mega, giga, tera) */
static const char sizemods[] = "KMGT";

/* Initialize options library */


int sb_options_init(void)
{
  SB_LIST_INIT(&options);

  return 0;
}


/* Register set of command line arguments */


int sb_register_arg_set(sb_arg_t *set)
{
  unsigned int i;

  for (i=0; set[i].name != NULL; i++)
    if (set_option(set[i].name, set[i].value, set[i].type))
      return 1;
  
  return 0;
}

option_t *sb_find_option(char *name)
{
  return find_option(&options, name);
}

int set_option(char *name, char *value, sb_arg_type_t type)
{
  option_t *opt;
  char     *tmp;

  opt = add_option(&options, name);
  if (opt == NULL)
    return 1;
  free_values(&opt->values);
  opt->type = type;
  if (type != SB_ARG_TYPE_FLAG && (value == NULL || value[0] == '\0'))
    return 0;
  
  switch (type) {
    case SB_ARG_TYPE_FLAG:
      if (value == NULL || !strcmp(value, "on")) 
        add_value(&opt->values, "on");
      break;
    case SB_ARG_TYPE_INT:
    case SB_ARG_TYPE_SIZE:
    case SB_ARG_TYPE_FLOAT:
    case SB_ARG_TYPE_STRING:
      add_value(&opt->values, value);
      break;
    case SB_ARG_TYPE_LIST:
      tmp = strdup(value);
      for (tmp = strtok(tmp, ","); tmp != NULL; tmp = strtok(NULL, ","))
        add_value(&opt->values, tmp);
      free(tmp);
      break;
    default:
      printf("Unknown argument type: %d", type);
      return 1;
  }

  return 0;
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
    if (len > maxlen)
      maxlen = len;
  }
  
  for (i = 0; opts[i].name != NULL; i++)
  {
    switch (opts[i].type) {
      case SB_ARG_TYPE_FLAG:
        fmt = "=[on|off]";
        break;
      case SB_ARG_TYPE_LIST:
        fmt = "=[LIST,...]";
        break;
      case SB_ARG_TYPE_SIZE:
        fmt = "=SIZE";
        break;
      case SB_ARG_TYPE_INT:
      case SB_ARG_TYPE_FLOAT:
        fmt = "=N";
        break;
      case SB_ARG_TYPE_STRING:
        fmt = "=STRING";
        break;
      default:
        fmt = "<UNKNOWN>";
    }
    printf("  --%s%-*s%s", opts[i].name,
           (int)(maxlen - strlen(opts[i].name) + 8), fmt,
           opts[i].desc);
    if (opts[i].value != NULL)
      printf(" [%s]", opts[i].value);
    printf("\n");
  }
  printf("\n");
}


int sb_get_value_flag(char *name)
{
  option_t *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  return !SB_LIST_IS_EMPTY(&opt->values);
}


int sb_get_value_int(char *name)
{
  option_t       *opt;
  value_t        *val;
  sb_list_item_t *pos;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  SB_LIST_FOR_EACH(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    return atoi(val->data);
  }

  return 0;
}


unsigned long long sb_get_value_size(char *name)
{
  option_t            *opt;
  value_t             *val;
  sb_list_item_t      *pos;
  unsigned long long  res = 0;
  char                mult = 0;
  int                 rc;
  unsigned int        i, n;
  char                *c;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  SB_LIST_FOR_EACH(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    /*
     * Reimplentation of sscanf(val->data, "%llu%c", &res, &mult), since
     * there is little standartization about how to specify long long values
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

    if (rc == 2) {
      for (n = 0; sizemods[n] != '\0'; n++)
        if (mult == sizemods[n])
          break;
      if (sizemods[n] != '\0')
      {
        for (i = 0; i <= n; i++)
          res *= 1024;
      }
      else
        res = 0; /* Unknown size modifier */
    }
    break;
  }

  return res;
}


float sb_get_value_float(char *name)
{
  option_t       *opt;
  value_t        *val;
  sb_list_item_t *pos;
  float          res;

  opt = find_option(&options, name);
  if (opt == NULL)
    return 0;

  SB_LIST_FOR_EACH(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    if (sscanf(val->data, "%f", &res) < 1)
      res = 0;
  }

  return res;
}


char *sb_get_value_string(char *name)
{
  option_t       *opt;
  value_t        *val;
  sb_list_item_t *pos;

  opt = find_option(&options, name);
  if (opt == NULL)
    return NULL;

  SB_LIST_FOR_EACH(pos, &opt->values)
  {
    val = SB_LIST_ENTRY(pos, value_t, listitem);
    return val->data;
  }

  return NULL;
}


sb_list_t *sb_get_value_list(char *name)
{
  option_t       *opt;

  opt = find_option(&options, name);
  if (opt == NULL)
    return NULL;

  return &opt->values;
}


char *sb_print_value_size(char *buf, unsigned int buflen, double value)
{
  unsigned int i;

  for (i = 0; i < sizeof(sizemods) && value >= 1024; i++, value /= 1024)
    /* empty */ ;

  if (i > 0)
    snprintf(buf, buflen, "%.5g%c", value, sizemods[i-1]);
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


value_t *add_value(sb_list_t *values, char *data)
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


value_t *find_value(sb_list_t *values, char *data)
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


option_t *add_option(sb_list_t *options, char *name)
{
  option_t *option;
  
  if (options == NULL || name == NULL)
    return NULL;

  if ((option = find_option(options, name)) != NULL)
    return option; 

  if ((option = new_option()) == NULL)
    return NULL;

  option->name = strdup(name);
  
  SB_LIST_ADD_TAIL(&option->listitem, options);

  return option;
}


option_t *find_option(sb_list_t *options, char *name)
{
  sb_list_item_t *pos;
  option_t       *opt;    
  
  if (options == NULL || name == NULL)
    return NULL;

  SB_LIST_FOR_EACH(pos, options)
  {
    opt = SB_LIST_ENTRY(pos, option_t, listitem);
    if (!strcmp(opt->name, name))
      return opt;
  }
  
  return NULL;
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
        for (optlen = 0; tmp[optlen] != '\0' && tmp[optlen] != VALUE_SEPARATOR;
             optlen++)
        {
          /* Empty */
        }
        if (tmp[optlen] != '\0')
          tmp[optlen++] = '\0';
        add_value(&newopt->values, tmp);
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

