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
  newSVstring (pTHX_ const string& s)
  {
    return Perl_newSVpvn (s.data (), s.size ());
  }

  inline static string
  sv_to_string (pTHX_ SV* sv)
  {
    STRLEN len;
    const char* p;

    p = SvPV (sv, len);
    return string (p, len);
  }

  const Interpreter&
  Scalar::get_interpreter () const
  {
    return Interpreter::get_current ();
  }

  Scalar::~Scalar ()
  {
    dTHX;
    SvREFCNT_dec (imp);
  }

  static inline SV*
  new_scalar ()
  {
    dTHX;
    return Perl_newSVsv (&PL_sv_undef);
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
    dTHX;
    SV* t;

    t = imp;
    imp = SvREFCNT_inc (o.imp);
    SvREFCNT_dec (t);
    return *this;
  }

  static inline SV* make (unsigned long i) { dTHX; return Perl_newSVnv (i); }
  Scalar::Scalar (unsigned long i) : imp (make (i)) {}

  static inline SV* make (long i) { dTHX; return Perl_newSViv (i); }
  Scalar::Scalar (long i) : imp (make (i)) {}

  static inline SV* make (unsigned int i) { dTHX; return Perl_newSVnv (i); }
  Scalar::Scalar (unsigned int i) : imp (make (i)) {}

  static inline SV* make (int i) { dTHX; return Perl_newSViv (i); }
  Scalar::Scalar (int i) : imp (make (i)) {}

  static inline SV* make (const string& s) { dTHX; return newSVstring (s); }
  Scalar::Scalar (const string& s) : imp (make (s)) {}

  static inline SV* make (const char* s) { dTHX; return Perl_newSVpv (s, 0); }
  Scalar::Scalar (const char* s) : imp (make (s)) {}

  static inline SV* make (double d) { dTHX; return Perl_newSVnv (d); }
  Scalar::Scalar (double d) : imp (make (d)) {}

  static inline SV* make (bool b) { dTHX; return b ? &PL_sv_yes : &PL_sv_no; }
  Scalar::Scalar (bool b) : imp (make (b)) {}

  Scalar
  Scalar::lookup (const string& name)
  {
    dTHX;
    return SvREFCNT_inc (Perl_get_sv (name .c_str (), 1));
  }

  bool
  Scalar::defined () const
  {
    dTHX;
    return SvOK (imp);
  }

  Scalar
  Scalar::ref () const
  {
    dTHX;
    if (! SvROK (imp))
      return &PL_sv_no;
    return Perl_sv_reftype (SvRV (imp), 1);
  }

  bool
  Scalar::eq (const Scalar& that) const
  {
    dTHX;
    return Perl_sv_eq (imp, that.imp);
  }

  unsigned long
  Scalar::as_ulong () const
  {
    dTHX;
    return SvUV (imp);
  }
  long
  Scalar::as_long () const
  {
    dTHX;
    return SvIV (imp);
  }
  unsigned int
  Scalar::as_uint () const
  {
    dTHX;
    return SvUV (imp);
  }
  int
  Scalar::as_int () const
  {
    dTHX;
    return SvIV (imp);
  }
  string
  Scalar::as_string () const
  {
    dTHX;
    return sv_to_string (imp);
  }
  const char*
  Scalar::as_c_str () const
  {
    dTHX;
    return SvPV_nolen (imp);
  }
  double
  Scalar::as_double () const
  {
    dTHX;
    return SvNV (imp);
  }
  bool
  Scalar::as_bool () const
  {
    dTHX;
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
    return Perl_sv_derived_from (imp, package.c_str());
  }

  ostream& operator << (ostream& os, const Scalar& t)
  {
    dTHX;
    SV* arg = const_cast<SV*> (t.imp);
    bool has_xml;

    has_xml = Scalar ("XML::Dumper") .can ("new");

    if (!has_xml && !Scalar ("Data::Dumper") .can ("Dumper"))
      {
	const Interpreter& i = t.get_interpreter ();

	try { i.require_module ("XML::Dumper"); }
	catch (Exception* e) { delete e; }

	has_xml = Scalar ("XML::Dumper")
	  .call_method ("can", List () << "new");

	if (!has_xml)
	  {
	    i .require_module ("Data::Dumper");
	    if (!Scalar ("Data::Dumper") .call_method ("can", List ()
						       << "Dumper"))
	      i.die ("Loaded Data::Dumper but Data::Dumper->Dumper"
			     " was not defined");
	  }
      }

    return os << (has_xml ? t.as_xml() : t.as_perl());
  }

  string
  Scalar::as_xml () const
  {
    get_interpreter() .require_module ("XML::Dumper");
    return Scalar ("XML::Dumper") .call_method ("new")
      .call_method ("pl2xml", List () << *this);
  }

  string
  Scalar::as_perl () const
  {
    const Interpreter& i (get_interpreter ());
    i .require_module ("Data::Dumper");
    return i .call_function ("Data::Dumper::Dumper", List () << *this);
  }

  /* Call a method with arguments in scalar context.  */
  Scalar
  Scalar::call_method (const string& meth, const List& args,
		       Context cx = SCALAR) const
  {
    // Avoid perl_call_method because it cannot trap the no-such-method
    // error.  XXX
    {
      dTHX;
      if (! perl_get_cv ("Pickle::call_method", 0))
	get_interpreter () .eval_string
	  ("sub Pickle::call_method {	\
		my ($meth, $obj, $args) = @_;	\
		$obj->$meth (@$args);		\
	    }");
    }
    return get_interpreter () .call_function
      ("Pickle::call_method",
       List () << meth << *this << (const Arrayref&) args, cx);
  }

}
