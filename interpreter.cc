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


#include <strstream>
#include <XSUB.h>


#ifdef PERL_IMPLICIT_CONTEXT
#  error "PERL_IMPLICIT_CONTEXT is not fully supported."
#endif

#define PICKLE_INTERPRETER_PRIVATE				\
    Interpreter (Interpreter_imp* i) : interpreter_imp (i) {}	\
    static Interpreter& ref (Interpreter_imp*& ptr)		\
    { return *(Interpreter*) &ptr; }				\
								\
    void init (int, const char* const *, const char* const *);	\
    Scalar_imp* call_function (Scalar_imp* func, int argc,	\
			       Scalar_imp** argv,		\
			       Context ctx) const;		\
    static void propagate_to_perl (Exception* e);		\
    static XS (xs_entry_one_arg);				\
    static XS (xs_entry_hashref);				\
    static XS (xs_entry_list);


#include "pickle_int.hh"

#if REFCNT_DEBUG
SV* my_sv_refcnt_inc (SV* sv) { return my_inline_sv_refcnt_inc (sv); }
void my_sv_refcnt_dec (SV* sv) { my_inline_sv_refcnt_dec (sv); }
#endif


namespace Pickle
{

  static Scalar
  my_warner (Scalar& msg)
  {
    cerr << msg .as_string ();
    return Scalar ();
  }

  extern "C" { void xs_init (pTHXo); }
  static void
  my_xs_init (pTHXo)
  {
    xs_init (aTHXo);

    // Install a warning handler to prevent the default warning behavior,
    // which prints to stderr, conflicting with iostream use.
    // XXX Make this sub anonymous by adding Coderef constructors that
    // take a function pointer.
    define_sub ("Pickle", "my_warner", my_warner);
    Hashref ("SIG") .store ("__WARN__", "Pickle::my_warner");
    // XXX Perl doesn't assign early enough?!
    if (PL_warnhook)
      SvREFCNT_dec (PL_warnhook);
    PL_warnhook = Perl_newSVpv ("Pickle::my_warner", 0);
  }

  static void barf (PerlInterpreter* p, const char* whodied, int status)
    __attribute__((noreturn));
  static void barf (PerlInterpreter* p, const char* whodied, int status)
  {
    perl_destruct (p);
    perl_free (p);
    PL_curinterp = 0;
    ostrstream s;
    s << whodied << " failed with status " << status;
    throw new Init_Exception (s .str ());
  }

  void
  Interpreter::init (int argc, const char* const * argv,
		     const char* const * envp)
  {
    int status;

    my_perl = perl_alloc ();
    perl_construct (my_perl);
    PL_perl_destruct_level = 2;  // hope to catch memory leaks

    status = perl_parse (my_perl, my_xs_init, argc, const_cast<char**> (argv),
			 const_cast<char**> (envp));
    if (status)
      barf (my_perl, "perl_parse", status);

    // Avoid perl_call_method because it cannot trap the no-such-method
    // error.  XXX
    eval_string
      ("sub Pickle::call_method {		\
            my ($meth, $obj, $args) = @_;	\
            $obj->$meth (@$args);		\
        }");

    status = perl_run (my_perl);
    if (status)
      barf (my_perl, "perl_run", status);
  }

  Interpreter::Interpreter (int argc, const char* const * argv,
			    const char* const * envp)
  {
    init (argc, argv, envp);
  }

  Interpreter::Interpreter ()
  {
    static const char* argv[] = { "Pickle", "-e0" };
    init (sizeof argv / sizeof argv [0], argv, 0);
  }
  Interpreter::Interpreter (const vector<string>& args)
  {
    int argc = args.size ();
    const char* argv[argc];
    vector<string>::const_iterator i = args.begin ();
    
    for (int n = 0; n < argc; n++)
      argv [n] = (i++)->c_str();
    init (argc, argv, 0);
  }
  Interpreter::Interpreter (const vector<string>& args,
			    const vector<string>& env)
  {
    int argc = args.size ();
    int envc = env.size ();
    const char* argv[argc];
    vector<string>::const_iterator i = args.begin();
    
    for (int n = 0; n < argc; n++)
      argv [n] = (i++)->c_str();
    init (argc, argv, 0);
  }

  Interpreter::~Interpreter ()
  {
    perl_destruct (my_perl);
    perl_free (my_perl);
    PL_curinterp = 0;
  }

  const Interpreter&
  Interpreter::get_current ()
  {
    return Interpreter::ref (PL_curinterp);
  }

  bool
  Interpreter::ping ()
  {
    dTHX;
    return PL_curinterp != 0;
  }

  void
  Interpreter::require_module (const string& bare) const
  {
    eval_string (string ("require ") .append (bare));
  }

  Scalar
  Interpreter::eval_string (const string& proggie) const
  {
    djSP;
    SV* retsv;
    Scalar err;

    ENTER;
    SAVETMPS;
    // This is equivalent to `local($@)'.
    Perl_save_scalar (PL_errgv);

    retsv = Perl_eval_pv (proggie.c_str(), G_SCALAR);

    // I wasted half a day figuring out that this is needed.
    POPMARK;

    SvREFCNT_inc (retsv);
    if (SvTRUE (ERRSV))
      err = Perl_newSVsv (ERRSV);

    FREETMPS;
    LEAVE;

    if (err)
      die (err);

    return retsv;
  }

  Scalar
  Interpreter::call_function (const Scalar& func, const List& args,
			      Context cx = SCALAR) const
  {
    AV* av = (AV*) SvRV (((const Arrayref&) args) .imp);
    return call_function (const_cast<SV*> (func .imp), 1 + AvFILL (av),
			  AvARRAY (av), cx);
  }

  SV*
  Interpreter::call_function (SV* func, int argc, SV** argv, Context cx) const
  {
    djSP;
    SV* retsv;
    Scalar err;
    I32 numret;
    I32 ctx;

    switch (cx)
      {
      default:
      case SCALAR: ctx = G_SCALAR; break;
      case LIST  : ctx = G_ARRAY;  break;
      case VOID  : ctx = G_VOID;   break;
      }

    ENTER;
    SAVETMPS;
    // This is equivalent to `local($@)'.
    Perl_save_scalar (PL_errgv);

    PUSHMARK (sp);
    EXTEND (sp, argc);
    for (int i = 0; i < argc; i++)
      PUSHs (argv [i]);
    PUTBACK;

    numret = perl_call_sv (func, ctx | G_EVAL);
    SPAGAIN;

    switch (ctx)
      {
      case G_ARRAY:
	sp -= numret;
	retsv = Perl_newRV_noinc ((SV*) Perl_av_make (numret, sp + 1));
	break;

      case G_SCALAR:
	retsv = SvREFCNT_inc (POPs);
	break;

      case G_VOID:
	retsv = &PL_sv_undef;
	break;
      }

    PUTBACK;

    if (SvTRUE (ERRSV))
      err = Perl_newSVsv (ERRSV);

    FREETMPS;
    LEAVE;

    if (err)
      die (err);

    return retsv;
  }

  void Interpreter::propagate_to_perl (Exception* e)
  {
    // XXX This is not especially clever.
    Perl_sv_setsv (ERRSV, e->get_scalar () .imp);
    delete e;
    Perl_croak ("%_", ERRSV);
  }

  XS (Interpreter::xs_entry_one_arg)
  {
    dXSARGS;
    if (items != 1)
      Perl_croak ("Usage: %s(arg)", GvNAME (CvGV (cv)));

    try
      {
	Scalar arg (SvREFCNT_inc (ST (0)));
	Scalar ret (((sub_one_arg) CvXSUBANY (cv) .any_ptr) (arg));
	/* The callback return value is basically a time bomb, an SV with
	   0 refcount: actually, its refcnt is about to drop to 0 when the
	   container (Scalar) dies.  Therefore, we must inc its refcount.
	   But we are putting it on the Perl arg stack, so it must be
	   "mortalized".  In other words, this replaces one terminal illness
	   with another.  A clever compiler might optimize away SvREFCNT_inc.
	   (nah...)
	*/
	ST (0) = SvREFCNT_inc (Perl_sv_2mortal (ret .imp));
      }
    catch (Exception* e)
      {
	propagate_to_perl (e);
      }
    XSRETURN (1);
  }

  XS (Interpreter::xs_entry_hashref)
  {
    dXSARGS;
    if ((items % 2) == 0)
      Perl_croak ("Usage: OBJECT->%s (NAME, VALUE, ...)", GvNAME (CvGV (cv)));

    HV* args = Perl_newHV ();
    for (int i = 1; i < items - 1; i += 2)
      Perl_hv_store_ent (args, ST (i), SvREFCNT_inc (ST (i + 1)), 0);

    try
      {
	Scalar obj (SvREFCNT_inc (ST (0)));
	Hashref arg (Perl_newRV_noinc ((SV*) args));
	Scalar ret (((sub_hashref) CvXSUBANY (cv) .any_ptr) (obj, arg));
	ST (0) = SvREFCNT_inc (Perl_sv_2mortal (ret .imp));
      }
    catch (Exception* e)
      {
	propagate_to_perl (e);
      }
    XSRETURN (1);
  }

  XS (Interpreter::xs_entry_list)
  {
    dXSARGS;
    Context cx;
    SV* retsv;
    AV* retav;

    SV* args = Perl_newRV_noinc ((SV*) Perl_av_make (items, & ST (0)));
    switch (GIMME_V)
      {
      case G_ARRAY : cx = LIST;   break;
      case G_SCALAR: cx = SCALAR; break;
      default      : cx = VOID;   break;
      }

    try
      {
	List arglist (args);
	List retlist (((sub) CvXSUBANY (cv) .any_ptr) (arglist, cx));
	retsv = SvREFCNT_inc (Perl_sv_2mortal (((Arrayref&) retlist) .imp));
      }
    catch (Exception* e)
      {
	propagate_to_perl (e);
      }

    retav = (AV*) SvRV (retsv);

    switch (cx)
      {
      case LIST:
	{
	  size_t sz = Perl_av_len (retav) + 1;
	  if (sz != 0)
	    {
	      if (sz > (size_t) items)
		EXTEND (sp, (I32) sz - items);

	      // Note: List AVs are never magical.
	      SV** svp = AvARRAY (retav);
	      for (size_t i = 0; i < sz; i++)
		ST (i) = SvREFCNT_inc (Perl_sv_2mortal (svp [i]));
	    }
	  XSRETURN (sz);
	}
	break;

      case SCALAR:
	{
	  size_t sz = Perl_av_len (retav) + 1;
	  if (sz != 0)
	    {
	      if (items < 1)
		EXTEND (sp, 1);

	      ST (0) = SvREFCNT_inc (Perl_sv_2mortal
				     (AvARRAY (retav) [sz - 1]));
	      XSRETURN (1);
	    }
	  else
	    XSRETURN_UNDEF;
	}
	break;

      case VOID:
	XSRETURN (0);
      }
  }

  static void
  reg (const string& package, const string& name, void* fn, XS (xs))
  {
    string fullname (package);
    fullname .append ("::") .append (name);
    CV* cv = Perl_newXS (const_cast<char*> (fullname .c_str ()),
			 xs, const_cast<char*> (__FILE__));
    CvXSUBANY (cv) .any_ptr = fn;
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub_one_arg fn) const
  {
    reg (package, name, fn, xs_entry_one_arg);
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub fn) const
  {
    reg (package, name, fn, xs_entry_list);
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub_hashref fn) const
  {
    reg (package, name, fn, xs_entry_hashref);
  }

  Exception::Exception (const string& s)
  {
    Interpreter::vivify ();
    scalar = s;
  }

  Exception::Exception (const char* s)
  {
    Interpreter::vivify ();
    scalar = s;
  }

}
