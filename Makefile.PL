# Copyright (C) 2000 by John Tobey,
# jtobey@john-edwin-tobey.org.  All rights reserved.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; see the file COPYING.  If not, write to the
#   Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#   MA 02111-1307  USA
#
# See also README.

use 5.006;
use ExtUtils::MakeMaker;

WriteMakefile (
	       'NAME' => 'pickle',
	       'VERSION' => '0.4',
	       'CC' => 'g++',
	       'LD' => 'g++',
	      );

package MY;

sub top_targets { <<'DONE' }

#all: pure_all htmlifypods manifypods
all: pure_all htmlifypods

pure_all: libpickle.$(SO) libperlint.$(SO)

LIBPERLINT = libperlint.$(SO).1
LIBPICKLE = libpickle.$(SO).1
LIBHEADERS = pickle.hh
EMBED_LDOPTS = `$(PERL) -MExtUtils::Embed -e ldopts`
OBJECTS = interpreter$(OBJ_EXT) scalar$(OBJ_EXT) scalarref$(OBJ_EXT) \
	arrayref$(OBJ_EXT) hashref$(OBJ_EXT) coderef$(OBJ_EXT)

libpickle.$(SO): $(LIBPICKLE)
	$(RM_F) $@
	$(LN_S) $< $@

libperlint.$(SO): $(LIBPERLINT)
	$(RM_F) $@
	$(LN_S) $< $@

# "-u" is supposed to mean require the following symbol.
$(LIBPERLINT):
	$(LD) -o $@ $(LDDLFLAGS) -u perl_alloc -u boot_DynaLoader \
		$(EMBED_LDOPTS)
	$(CHMOD) $(PERM_RWX) $@

perlxsi.c:
	$(PERL) -MExtUtils::Embed -e xsinit

$(LIBPICKLE): $(OBJECTS) perlxsi$(OBJ_EXT) libperlint.$(SO)
	$(LD) -o $@ $^ $(LDDLFLAGS) -L. -lperlint
	$(CHMOD) $(PERM_RWX) $@
DONE

sub test { <<'DONE' }

test_dynamic: test_pickle_d$(EXE_EXT)
	LD_LIBRARY_PATH=. ./test_pickle_d

test_static: test_pickle$(EXE_EXT)
	./test_pickle

test: test_static

test_pickle_d$(EXE_EXT): test_pickle$(OBJ_EXT) libperlint.$(SO) libpickle.$(SO)
	$(LD) -o $@ test_pickle$(OBJ_EXT) -L. -lpickle -lperlint

test_pickle$(EXE_EXT): test_pickle$(OBJ_EXT) $(OBJECTS) perlxsi$(OBJ_EXT)
	$(LD) -o $@ $^ $(EMBED_LDOPTS)
DONE

sub install { <<'DONE' }

install: $(LIBHEADERS) $(LIBPICKLE) $(LIBPERLINT)
	$(TEST_D) $(PREFIX)/include || mkdir -p $(PREFIX)/include
	$(CP_P) $(LIBHEADERS) $(PREFIX)/include
	$(TEST_D) $(PREFIX)/lib || mkdir -p $(PREFIX)/lib
	$(CP_P) $(LIBPICKLE) $(PREFIX)/lib
	$(RM_F) $(PREFIX)/lib/libpickle.so
	$(LN_S) $(LIBPICKLE) $(PREFIX)/lib/libpickle.so
	$(CP_P) $(LIBPERLINT) $(PREFIX)/lib
	$(RM_F) $(PREFIX)/lib/libperlint.so
	$(LN_S) $(LIBPERLINT) $(PREFIX)/lib/libperlint.so
DONE

sub clean { <<'DONE' }
clean ::
	$(RM_F) $(OBJECTS) test_pickle$(OBJ_EXT) perlxsi$(OBJ_EXT) perlxsi.c

DONE

sub realclean { <<'DONE' }
realclean purge :: clean
	$(RM_F) test_pickle$(EXE_EXT) test_pickle_d$(EXE_EXT) \
		libpickle.$(SO) $(LIBPICKLE) libperlint.$(SO) \
		$(LIBPERLINT) pickle.$(MAN3EXT) Makefile Makefile.old

DONE

sub postamble { <<'DONE' }

LN_S = ln -s
CP_P = $(CP) -p
TEST_D = test -d

$(OBJECTS): pickle.hh

interpreter$(OBJ_EXT) scalar$(OBJ_EXT) scalarref$(OBJ_EXT) \
	arrayref$(OBJ_EXT) hashref$(OBJ_EXT) coderef$(OBJ_EXT): \
	pickle_int.hh

test_pickle$(OBJ_EXT): pickle.hh
DONE
