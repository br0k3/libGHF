__copyright__ = "Copyright (C) 2007 - *nixCoders team"
__license__ = """
        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software Foundation,
        Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
"""

import os

######################################################
## Global var for targets in ./src
targets = "asm.c elf.c utils.c vfunction.cpp"
######################################################

opts = Options()
env = Environment(option = opts, CPPPATH = ['src'])
#conf = Configure(env)

env['LIBS'] = ['z', 'dl', 'ghf', 'stdc++']

if os.environ.has_key('CFLAGS'):
 env.Append (CFLAGS = os.environ['CFLAGS'])

if os.environ.has_key('CXXFLAGS'):
 env.Append (CXXFLAGS = os.environ['CXXFLAGS'])

env.Append (CPPFLAGS = "-fPIC -Wall -pipe -march=i686 -m32")

static = env.StaticLibrary ('ghf', env.Split(targets), srcdir='src')
shared = env.SharedLibrary ('ghf', env.Split(targets), srcdir='src')

Alias('static', static)
Alias('shared', shared)
Alias('lib', ['static','shared'])
Default('lib')
