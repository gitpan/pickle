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
# See README about alternative licensing.

use 5.005;
use ExtUtils::MakeMaker;

WriteMakefile (
	       'NAME' => 'pickle',
	       'VERSION' => '0.5.1',
	       'MAN3PODS' => { 'pickle.pod' =>
			       "\$(INST_MAN3DIR)/pickle.\$(MAN3EXT)" },

	       # Don't put \ at end of lines, MakeMaker adds a second \,
	       # which breaks (GNU) Make.
	       'OBJECT' => q/interpreter$(OBJ_EXT) scalar$(OBJ_EXT)
			     scalarref$(OBJ_EXT) arrayref$(OBJ_EXT)
			     hashref$(OBJ_EXT) coderef$(OBJ_EXT)
			     globref$(OBJ_EXT)/,
	      );

package MY;

sub constants {
    my $self = shift;
    my $ret = $self->SUPER::constants;

    if ($self->{'LD'} =~ /^gcc/) {
	warn <<END;
###
### Your LD is $$self{'LD'}.  This had better know about C++ libraries!
### If you get undefined references to a lot of Standard C++ functions,
### try rerunning Makefile.PL with `LD=g++'.
###
END
    }

    # Don't use MakeMaker's default handling of C/C++ files, which is
    # to build an XS module.  We're building ordinary libs instead.
    $ret =~ s/^INST_(STATIC|DYNAMIC|BOOT).*//gm;  # Yuck.
    return $ret;
}

sub test { <<'DONE' }

test_dynamic: test_pickle_d$(EXE_EXT)
	LD_LIBRARY_PATH=. ./test_pickle_d

test_static: test_pickle$(EXE_EXT)
	./test_pickle

test: test_static

test_pickle_d$(EXE_EXT): test_pickle$(OBJ_EXT) libperlint.$(SO) libpickle.$(SO)
	$(LD) -o $@ test_pickle$(OBJ_EXT) -L. -lpickle -lperlint

test_pickle$(EXE_EXT): test_pickle$(OBJ_EXT) $(OBJECT) perlxsi$(OBJ_EXT)
	$(LD) -o $@ $^ $(EMBED_LDOPTS)

test_pickle$(OBJ_EXT): test_pickle.cc pickle.hh
	$(CC) -o $@ -c test_pickle.cc -I .
DONE

sub postamble { <<'DONE' }

clean ::
	$(RM_F) test_pickle$(OBJ_EXT) perlxsi$(OBJ_EXT) perlxsi.c

pure_install :: install_headers install_pickle install_perlint

install_headers: $(LIBHEADERS)
	$(TEST_D) $(PREFIX)/include || mkdir -p $(PREFIX)/include
	$(CP_P) $(LIBHEADERS) $(PREFIX)/include

install_pickle: $(LIBPICKLE)
	$(TEST_D) $(PREFIX)/lib || mkdir -p $(PREFIX)/lib
	$(CP_P) $(LIBPICKLE) $(PREFIX)/lib
	$(RM_F) $(PREFIX)/lib/libpickle.so
	$(LN_S) $(LIBPICKLE) $(PREFIX)/lib/libpickle.so

install_perlint: $(LIBPERLINT)
	$(TEST_D) $(PREFIX)/lib || mkdir -p $(PREFIX)/lib
	$(CP_P) $(LIBPERLINT) $(PREFIX)/lib
	$(RM_F) $(PREFIX)/lib/libperlint.so
	$(LN_S) $(LIBPERLINT) $(PREFIX)/lib/libperlint.so

realclean purge ::
	$(RM_F) test_pickle$(EXE_EXT) test_pickle_d$(EXE_EXT) \
		libpickle.$(SO) $(LIBPICKLE) libperlint.$(SO) \
		$(LIBPERLINT) pickle.$(MAN3EXT) Makefile Makefile.old

pure_all :: libpickle.$(SO) libperlint.$(SO)

LIBPERLINT = libperlint.$(SO).1
LIBPICKLE = libpickle.$(SO).1
LIBHEADERS = pickle.hh
EMBED_LDOPTS = `$(PERL) -MExtUtils::Embed -e ldopts`

libpickle.$(SO): $(LIBPICKLE)
	$(RM_F) $@
	$(LN_S) $< $@

libperlint.$(SO): $(LIBPERLINT)
	$(RM_F) $@
	$(LN_S) $< $@

# "-u" is supposed to mean require the following symbol.
# XXX -Wl, is gcc-specific.  -h is specific to GNU and Solaris.
$(LIBPERLINT): perlxsi$(OBJ_EXT)
	$(LD) -o $@ $(LDDLFLAGS) -u perl_alloc -u boot_DynaLoader \
		$(EMBED_LDOPTS) perlxsi$(OBJ_EXT) -Wl,-h,$@
	$(CHMOD) $(PERM_RWX) $@

perlxsi.c:
	$(PERL) -MExtUtils::Embed -e xsinit

# XXX -Wl, is gcc-specific.  -h is specific to GNU and Solaris.
$(LIBPICKLE): $(OBJECT)
	$(LD) -o $@ $(OBJECT) $(LDDLFLAGS) -Wl,-h,$@
	$(CHMOD) $(PERM_RWX) $@

LN_S = ln -s
CP_P = $(CP) -p
TEST_D = test -d

$(OBJECT): pickle.hh

interpreter$(OBJ_EXT) scalar$(OBJ_EXT) scalarref$(OBJ_EXT) \
	arrayref$(OBJ_EXT) hashref$(OBJ_EXT) coderef$(OBJ_EXT) \
	globref$(OBJ_EXT) : \
	pickle_int.hh

test_pickle$(OBJ_EXT): pickle.hh
DONE
