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

#include "pickle_int.hh"


namespace Pickle
{

  inline static SV*
  string_to_sv (pTHX_ const string& s)
  {
    return newSVpvn (s.data (), s.size ());
  }

  inline static string
  sv_to_string (pTHX_ SV* sv)
  {
    STRLEN len;
    const char* p;

    p = SvPV (sv, len);
    return string (p, len);
  }

  Scalar::~Scalar ()
  {
    dInterp;
    SvREFCNT_dec (imp);
  }

  static inline SV*
  new_scalar ()
  {
    dTHX;
    return newSVsv (&PL_sv_undef);
  }

  Scalar::Scalar () : imp (new_scalar ()) {}

  Scalar::Scalar (const Scalar& o) : imp (o.imp)
  {
    dTHX;
    SvREFCNT_inc (imp);
  }

  Scalar&
  Scalar::operator= (const Scalar& o)
  {
    dInterp;
    SV* t;

    t = imp;
    imp = SvREFCNT_inc (o.imp);
    SvREFCNT_dec (t);
    return *this;
  }

  // Not-yet-implemented value-to-interpreter mapping.

  const Interpreter*
  Scalar::get_interpreter () const
  {
    return Interpreter::get_current ();
  }

  Interpreter*
  Scalar::get_interpreter ()
  {
    return Interpreter::get_current ();
  }

  bool
  Scalar::is_scalarref () const
  {
    dInterp;
    djSP;  // tryAMAGICunDEREF needs `sp'.
    // tryAMAGICunDEREF needs a var named `sv'.
    SV* sv = const_cast<SV*> (imp);
    if (! SvROK (sv))
      return false;

    // See pp_rv2sv in pp.c .
    tryAMAGICunDEREF(to_sv);
    // XXX
    switch (SvTYPE (SvRV (sv)))
      {
      case SVt_PVAV:
      case SVt_PVHV:
      case SVt_PVCV:
      case SVt_PVGV:  // In my book, globs are not scalars.
	  return false;
      }
    return true;
  }

  void
  Scalar::check_scalarref () const
  {
    if (! is_scalarref ())
      throw new Exception ("Not a scalar reference");
  }

  bool
  Scalar::is_arrayref () const
  {
    dInterp;
    djSP;  // tryAMAGICunDEREF needs `sp'.
    SV* sv = imp;  // tryAMAGICunDEREF needs a var named `sv'.
    if (! SvROK (sv))
      return false;

    tryAMAGICunDEREF(to_av);  // See pp_rv2av in pp_hot.c .
    return SvTYPE (SvRV (sv)) == SVt_PVAV;
  }

  void
  Scalar::check_arrayref () const
  {
    if (! is_arrayref ())
      throw new Exception ("Not an array reference");
  }

  bool
  Scalar::is_hashref () const
  {
    dInterp;
    djSP;  // tryAMAGICunDEREF needs `sp'.
    SV* sv = imp;  // tryAMAGICunDEREF needs a var named `sv'.
    if (! SvROK (sv))
      return false;

    tryAMAGICunDEREF(to_hv);  // See pp_rv2hv in pp_hot.c .
    // XXX Ignoring pseudohashes.
    return SvTYPE (SvRV (sv)) == SVt_PVHV;
  }

  void
  Scalar::check_hashref () const
  {
    if (! is_hashref ())
      throw new Exception ("Not a hash reference");
  }

  bool
  Scalar::is_globref () const
  {
    dInterp;
    djSP;  // tryAMAGICunDEREF needs `sp'.
    // tryAMAGICunDEREF needs a var named `sv'.
    SV* sv = const_cast<SV*> (imp);
    if (! SvROK (sv))
      return false;

    tryAMAGICunDEREF(to_gv);  // See pp_rv2gv in pp.c .
    // XXX Leaving out stuff about SVt_PVIO.
    return SvTYPE (SvRV (sv)) == SVt_PVGV;
  }

  void
  Scalar::check_globref () const
  {
    if (! is_globref ())
      throw new Exception ("Not a glob reference");
  }

  bool
  Scalar::is_coderef () const
  {
    //dInterp;
    return SvROK (imp) && SvTYPE (SvRV (imp)) == SVt_PVCV;
  }

  void
  Scalar::check_coderef () const
  {
    if (! is_coderef ())
      throw new Exception ("Not a code reference");
  }

  // Conversion from native C++ types to scalar.

  static inline SV* make (unsigned long i) { dTHX; return newSVnv (i); }
  Scalar::Scalar (unsigned long i) : imp (make (i)) {}

  static inline SV* make (long i) { dTHX; return newSViv (i); }
  Scalar::Scalar (long i) : imp (make (i)) {}

  static inline SV* make (unsigned int i) { dTHX; return newSVnv (i); }
  Scalar::Scalar (unsigned int i) : imp (make (i)) {}

  static inline SV* make (int i) { dTHX; return newSViv (i); }
  Scalar::Scalar (int i) : imp (make (i)) {}

  static inline SV* make (unsigned short i) { dTHX; return newSVnv (i); }
  Scalar::Scalar (unsigned short i) : imp (make (i)) {}

  static inline SV* make (short i) { dTHX; return newSViv (i); }
  Scalar::Scalar (short i) : imp (make (i)) {}

  static inline SV* make (unsigned char i) { dTHX; return newSVnv (i); }
  Scalar::Scalar (unsigned char i) : imp (make (i)) {}

  static inline SV* make (signed char i) { dTHX; return newSViv (i); }
  Scalar::Scalar (signed char i) : imp (make (i)) {}

  static inline SV* make (const string& s)
  { dTHX; return string_to_sv (aTHX_ s); }
  Scalar::Scalar (const string& s) : imp (make (s)) {}

  static inline SV* make (const char* s) { dTHX; return newSVpv (s, 0); }
  Scalar::Scalar (const char* s) : imp (make (s)) {}

  static inline SV* make (const char* s, unsigned long len)
  { dTHX; return newSVpvn (s, (STRLEN) len); }
  Scalar::Scalar (const char* s, unsigned long len) : imp (make (s, len)) {}

  static inline SV* make (double d) { dTHX; return newSVnv (d); }
  Scalar::Scalar (double d) : imp (make (d)) {}

  static inline SV* make (float d) { dTHX; return newSVnv (d); }
  Scalar::Scalar (float d) : imp (make (d)) {}

  static inline SV* make (bool b) { dTHX; return b ? &PL_sv_yes : &PL_sv_no; }
  Scalar::Scalar (bool b) : imp (make (b)) {}

  bool
  Scalar::defined () const
  {
    dInterp;
    return SvOK (imp);
  }

  Scalar
  Scalar::ref () const
  {
    dInterp;
    if (! SvROK (imp))
      return &PL_sv_no;
    return sv_reftype (SvRV (imp), 1);
  }

  unsigned long
  Scalar::length () const
  {
    dInterp;
    STRLEN len;
    SvPV (imp, len);
    return len;
  }

  bool
  Scalar::eq (const Scalar& that) const
  {
    dInterp;
    return sv_eq (imp, that.imp);
  }

  // Conversions from scalar to basic C++ types.

  unsigned long
  Scalar::as_ulong () const
  {
    dInterp;
    return SvUV (imp);
  }
  long
  Scalar::as_long () const
  {
    dInterp;
    return SvIV (imp);
  }
  unsigned int
  Scalar::as_uint () const
  {
    dInterp;
    return SvUV (imp);
  }
  int
  Scalar::as_int () const
  {
    dInterp;
    return SvIV (imp);
  }
  unsigned short
  Scalar::as_ushort () const
  {
    dInterp;
    return SvUV (imp);
  }
  short
  Scalar::as_short () const
  {
    dInterp;
    return SvIV (imp);
  }
  unsigned char
  Scalar::as_uchar () const
  {
    dInterp;
    return SvUV (imp);
  }
  signed char
  Scalar::as_schar () const
  {
    dInterp;
    return SvIV (imp);
  }
  string
  Scalar::as_string () const
  {
    dInterp;
    return sv_to_string (aTHX_ imp);
  }
  const char*
  Scalar::as_c_str () const
  {
    dInterp;
    return SvPV_nolen (imp);
  }
  double
  Scalar::as_double () const
  {
    dInterp;
    return SvNV (imp);
  }
  float
  Scalar::as_float () const
  {
    dInterp;
    return SvNV (imp);
  }
  bool
  Scalar::as_bool () const
  {
    dInterp;
    return SvTRUE (imp);
  }

  Scalar
  Scalar::can (const string& meth) const
  {
    return call_method ("can", List () << meth);
  }
  bool
  Scalar::isa (const string& package) const
  {
    dInterp;
    return sv_derived_from (imp, package.c_str());
  }

  ostream& operator << (ostream& os, const Scalar& t)
  {
    dTHX;  // XXX
    SV* arg = const_cast<SV*> (t.imp);
    bool has_xml;

    has_xml = Scalar ("XML::Dumper") .can ("new");

    if (!has_xml && !Scalar ("Data::Dumper") .can ("Dumper"))
      {
	const Interpreter* i = t.get_interpreter ();

	try { i ->require_module ("XML::Dumper"); }
	catch (Exception* e) { delete e; }

	has_xml = Scalar ("XML::Dumper")
	  .call_method ("can", List () << "new");

	if (!has_xml)
	  {
	    i ->require_module ("Data::Dumper");
	    if (!Scalar ("Data::Dumper") .call_method ("can", List ()
						       << "Dumper"))
	      throw new Exception ("Loaded Data::Dumper but"
				   " Data::Dumper->Dumper was not defined");
	  }
      }

    return os << (has_xml ? t .as_xml () : t .as_perl ());
  }

  string
  Scalar::as_xml () const
  {
    get_interpreter() ->require_module ("XML::Dumper");
    return Scalar ("XML::Dumper") .call_method ("new")
      .call_method ("pl2xml", List () << *this);
  }

  string
  Scalar::as_perl () const
  {
    const Interpreter* i (get_interpreter ());
    i ->require_module ("Data::Dumper");
    return i ->call_function ("Data::Dumper::Dumper", List () << *this);
  }

  /* Call a method with arguments in scalar context.  */
  Scalar
  Scalar::call_method (const string& meth, const List& args,
		       Context cx = SCALAR) const
  {
    // Avoid perl_call_method because it cannot trap the no-such-method
    // error.  XXX
    {
      dTHX;  // XXX
      if (! perl_get_cv ("Pickle::call_method", 0))
	get_interpreter () ->eval_string
	  ("sub Pickle::call_method {	\
		my ($meth, $obj, $args) = @_;	\
		$obj->$meth (@$args);		\
	    }");
    }
    return get_interpreter () ->call_function
      ("Pickle::call_method",
       List () << meth << *this << (const Arrayref&) args, cx);
  }

}
