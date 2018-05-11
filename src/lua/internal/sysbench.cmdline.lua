-- Copyright (C) 2017-2018 Alexey Kopytov <akopytov@gmail.com>

-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

-- ----------------------------------------------------------------------
-- Command line option handling
-- ----------------------------------------------------------------------

ffi = require("ffi")

ffi.cdef[[
/* The following has been copied from sb_option.h */

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

int sb_lua_set_test_args(sb_arg_t *args, size_t len);
]]

sysbench.cmdline.ARG_NULL = ffi.C.SB_ARG_TYPE_NULL
sysbench.cmdline.ARG_BOOL = ffi.C.SB_ARG_TYPE_BOOL
sysbench.cmdline.ARG_INT = ffi.C.SB_ARG_TYPE_INT
sysbench.cmdline.ARG_SIZE = ffi.C.SB_ARG_TYPE_SIZE
sysbench.cmdline.ARG_DOUBLE = ffi.C.SB_ARG_TYPE_DOUBLE
sysbench.cmdline.ARG_STRING = ffi.C.SB_ARG_TYPE_STRING
sysbench.cmdline.ARG_LIST = ffi.C.SB_ARG_TYPE_LIST
sysbench.cmdline.ARG_FILE = ffi.C.SB_ARG_TYPE_FILE
sysbench.cmdline.ARG_MAX = ffi.C.SB_ARG_TYPE_MAX

-- Attribute indicating that a custom command can be executed in parallel
sysbench.cmdline.PARALLEL_COMMAND = true

local arg_types = {
   boolean = sysbench.cmdline.ARG_BOOL,
   string = sysbench.cmdline.ARG_STRING,
   number = sysbench.cmdline.ARG_DOUBLE,
   table = sysbench.cmdline.ARG_LIST
}

local function __genOrderedIndex( t )
   local orderedIndex = {}
   for key in pairs(t) do
      table.insert( orderedIndex, key )
   end
   table.sort( orderedIndex )
   return orderedIndex
end

local function orderedNext(t, state)
   local key = nil
   if state == nil then
      t.__orderedIndex = __genOrderedIndex( t )
      key = t.__orderedIndex[1]
   else
      for i = 1,table.getn(t.__orderedIndex) do
         if t.__orderedIndex[i] == state then
            key = t.__orderedIndex[i+1]
         end
      end
   end

   if key then
      return key, t[key]
   end

   t.__orderedIndex = nil
   return
end

local function orderedPairs(t)
   return orderedNext, t, nil
end

-- Parse command line options definitions, if present in the script as a
-- 'sysbench.cmdline.options' table. If no such table exists, or if there a
-- parsing error, return false. Return true on success. After parsing the
-- command line arguments, option values are available as the sysbench.opt
-- table.
function sysbench.cmdline.read_cmdline_options()
   if sysbench.cmdline.options == nil then
      return true
   end

   local t = type(sysbench.cmdline.options)
   assert(t == "table", "wrong type for sysbench.cmdline.options: " .. t)

   local i = 0
   for name, def in pairs(sysbench.cmdline.options) do
      i = i+1
   end

   local args = ffi.new('sb_arg_t[?]', i)
   i = 0

   for name, def in orderedPairs(sysbench.cmdline.options) do
      -- name
      assert(type(name) == "string" and type(def) == "table",
             "wrong table structure in sysbench.cmdline.options")
      args[i].name = name

      -- description
      assert(def[1] ~= nil, "nil description for option " .. name)
      args[i].desc = def[1]

      if type(def[2]) == "table" then
         assert(type(def[3]) == "nil" or
                   type(def[3]) == sysbench.cmdline.ARG_LIST,
                "wrong type for list option " .. name)
         args[i].value = table.concat(def[2], ',')
      else
         if type(def[2]) == "boolean" then
            args[i].value = def[2] and 'on' or 'off'
         elseif type(def[2]) == "number" then
            args[i].value = tostring(def[2])
         else
            args[i].value = def[2]
         end
      end

      -- type
      local t = def[3]
      if t == nil then
         if def[2] ~= nil then
            -- Try to determine the type by the default value
            t = arg_types[type(def[2])]
         else
            t = sysbench.cmdline.ARG_STRING
         end
      end

      assert(t ~= nil, "cannot determine type for option " .. name)
      args[i].type = t

      -- validation function
      args[i].validate = def[4]

      i = i + 1
   end

   return ffi.C.sb_lua_set_test_args(args, i) == 0
end

function sysbench.cmdline.command_defined(name)
   return type(sysbench.cmdline.commands) == "table" and
      sysbench.cmdline.commands[name] ~= nil and
      sysbench.cmdline.commands[name][1] ~= nil
end

function sysbench.cmdline.command_parallel(name)
   return sysbench.cmdline.command_defined(name) and
      sysbench.cmdline.commands[name][2] == sysbench.cmdline.PARALLEL_COMMAND
end

function sysbench.cmdline.call_command(name)
   if not sysbench.cmdline.command_defined(name) then
      return false
   end

   local rc = sysbench.cmdline.commands[name][1]()

   if rc == nil then
      -- handle the case when the command does not return and value as success
      return true
   else
      -- otherwise return success for any returned value other than false
      return rc and true or false
   end
end

ffi.cdef[[
void sb_print_test_options(void);
]]

-- ----------------------------------------------------------------------
-- Print descriptions of command line options, if defined by
-- sysbench.cmdline.options
-- ----------------------------------------------------------------------
function sysbench.cmdline.print_test_options()
   ffi.C.sb_print_test_options()
end
