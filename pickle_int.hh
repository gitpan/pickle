/* 
   Copyright (C) 2000 by John Tobey,
   jtobey@john-edwin-tobey.org.  All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
   MA 02111-1307  USA
*/

extern "C"
{
#include <EXTERN.h>
#include <perl.h>
}
#undef assert
#undef call_method
#undef ref
#undef die
#undef vform

#define Interpreter_imp PerlInterpreter
#define interpreter_imp my_perl
#define Scalar_imp SV
#include "pickle.hh"

#ifdef PERL_IMPLICIT_CONTEXT
#  if 0  // This is the ideal way... if get_interpreter really worked.
#    define dInterp PerlInterpreter* my_perl = get_interpreter () ->my_perl
#  else  // !0
#    define dInterp dTHX
#  endif  // !0
#else
#  define dInterp dNOOP
#endif


#ifndef REFCNT_DEBUG
#  define REFCNT_DEBUG 0
#endif
#if REFCNT_DEBUG
// extern "C" for easier breakpoint setting.
extern "C" SV* my_sv_refcnt_inc (SV* sv);
extern "C" void my_sv_refcnt_dec (SV* sv);
inline SV* my_inline_sv_refcnt_inc (SV* sv) { return SvREFCNT_inc (sv); }
inline void my_inline_sv_refcnt_dec (SV* sv) { SvREFCNT_dec (sv); }

#undef SvREFCNT_inc
#define SvREFCNT_inc(_sv) ({ SV* sv = const_cast<SV*> (_sv); if (!SvIMMORTAL(sv)) cerr << "inc " << (sv) << " line " << __LINE__ << " to " << 1 + SvREFCNT(sv) << endl; my_sv_refcnt_inc (sv); })
#undef SvREFCNT_dec
#define SvREFCNT_dec(_sv) ({ SV* sv = const_cast<SV*> (_sv); if (!SvIMMORTAL(sv)) cerr << "dec " << (sv) << " line " << __LINE__ << " to " << SvREFCNT(sv) - 1 << endl; my_sv_refcnt_dec (sv); })
#endif  // REFCNT_DEBUG
