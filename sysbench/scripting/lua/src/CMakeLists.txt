# Copyright (C) 2008 MySQL AB
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

if(MSVC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_WIN")
endif(MSVC)
add_library(lua lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c 
        lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  
        lundump.c lvm.c lzio.c 
        lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c 
        lstrlib.c loadlib.c linit.c 
        lapi.h lcode.h ldo.h lgc.h llimits.h lobject.h lparser.h lstring.h ltm.h 
        luaconf.h lundump.h lzio.h lauxlib.h ldebug.h lfunc.h llex.h lmem.h lopcodes.h 
        lstate.h ltable.h lua.h lualib.h lvm.h)