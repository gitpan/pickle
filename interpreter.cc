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


#include <XSUB.h>

#define PICKLE_INTERPRETER_PRIVATE				\
    Interpreter (Interpreter_imp* i) : interpreter_imp (i) {}	\
								\
    void init (int, const char* const *, const char* const *);	\
    Scalar_imp* call_function (Scalar_imp* func, int argc,	\
			       Scalar_imp** argv,		\
			       Context ctx) const;		\
    friend void xs_entry_one_arg (pTHX_ CV* cv);		\
    friend void xs_entry_hashref (pTHX_ CV* cv);		\
    friend void xs_entry_list (pTHX_ CV* cv);


#include "pickle_int.hh"
#include <sstream>
#include <iostream>

#if REFCNT_DEBUG
SV* my_sv_refcnt_inc (SV* sv) { return my_inline_sv_refcnt_inc (sv); }
void my_sv_refcnt_dec (SV* sv) { my_inline_sv_refcnt_dec (sv); }
#endif


namespace Pickle
{

  // XXX need callbacks to take an interp.
  static Scalar
  my_warner (Scalar& msg)
  {
    cerr << msg .as_string ();
    return Scalar ();
  }

  extern "C" { void xs_init (pTHX); }
  static void
  my_xs_init (pTHX)
  {
    xs_init (aTHX);

    // Install a warning handler to prevent the default warning behavior,
    // which prints to stderr, conflicting with iostream use.
    // XXX Make this sub anonymous by adding Coderef constructors that
    // take a function pointer.
    define_sub ("Pickle", "my_warner", my_warner);
    Hashref ("SIG") .store ("__WARN__", "Pickle::my_warner");
    // XXX Perl doesn't assign early enough?!
    if (PL_warnhook)
      SvREFCNT_dec (PL_warnhook);
    PL_warnhook = newSVpv ("Pickle::my_warner", 0);
  }

  static void barf (PerlInterpreter* p, const char* whodied, int status)
    __attribute__((noreturn));
  static void barf (PerlInterpreter* p, const char* whodied, int status)
  {
    perl_destruct (p);
    perl_free (p);
    PL_curinterp = 0;
    ostringstream s;
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
    const char* argv[argc + 1];  // XXX portable?
    const char* envp[envc + 1];  // XXX portable?
    vector<string>::const_iterator p;
    int i;

    for (p = args .begin (), i = 0; i < argc; i++)
      argv [i] = (p++) ->c_str ();
    argv [argc] = 0;
    for (p = env .begin (), i = 0; i < envc; i++)
      envp [i] = (p++) ->c_str ();
    envp [envc] = 0;
    init (argc, argv, envp);
  }

  Interpreter::~Interpreter ()
  {
    perl_destruct (my_perl);
    perl_free (my_perl);
    PERL_SET_CONTEXT (0);
  }

  Interpreter*
  Interpreter::get_current ()
  {
    // XXX use PERL_GET_CONTEXT and keep a table of context --> Interpreter.
#ifdef PERL_IMPLICIT_CONTEXT
    dTHX;
    return aTHX ?
      // XXX leak memory for now, just testing this stuff...
      new Interpreter (aTHX)
      : 0;
#else  // !PERL_IMPLICIT_CONTEXT
    return PL_curinterp ? (Interpreter*) &PL_curinterp : 0;
#endif
  }

  // Data allocation.  cf. "Conversion from native C++ types to scalar"
  // in scalar.cc.

  Pickle::Scalar Interpreter::Scalar () const
  { return &PL_sv_undef; }
  Pickle::Scalar Interpreter::Scalar (unsigned long i) const
  { return newSVnv (i); }
  Pickle::Scalar Interpreter::Scalar (long i) const
  { return newSViv (i); }
  Pickle::Scalar Interpreter::Scalar (unsigned int i) const
  { return newSVnv (i); }
  Pickle::Scalar Interpreter::Scalar (int i) const
  { return newSViv (i); }
  Pickle::Scalar Interpreter::Scalar (unsigned short i) const
  { return newSVnv (i); }
  Pickle::Scalar Interpreter::Scalar (short i) const
  { return newSViv (i); }
  Pickle::Scalar Interpreter::Scalar (unsigned char i) const
  { return newSVnv (i); }
  Pickle::Scalar Interpreter::Scalar (signed char i) const
  { return newSViv (i); }

  Pickle::Scalar
  Interpreter::Scalar (const string& s) const
  {
    return newSVpvn (const_cast<char*> (s .data ()), s .size ());
  }

  Pickle::Scalar
  Interpreter::Scalar (const char* s) const
  {
    return newSVpv (const_cast<char*> (s), 0);
  }

  Pickle::Scalar
  Interpreter::Scalar (const char* s, unsigned long len) const
  {
    return newSVpvn (const_cast<char*> (s), len);
  }

  Pickle::Scalar Interpreter::Scalar (double d) const
  { return newSVnv (d); }
  Pickle::Scalar Interpreter::Scalar (float d) const
  { return newSVnv (d); }

  Pickle::Scalar
  Interpreter::Scalar (bool b) const
  {
    return b ? &PL_sv_yes : &PL_sv_no;
  }

  Pickle::Scalarref Interpreter::Scalarref () const
  { return Pickle::Scalarref (newRV_noinc (newSVsv (&PL_sv_undef)), false);}
  Pickle::Arrayref Interpreter::Arrayref () const
  { return Pickle::Arrayref (newRV_noinc ((SV*) newAV ()), false); }
  Pickle::Hashref Interpreter::Hashref () const
  { return Pickle::Hashref (newRV_noinc ((SV*) newHV ()), false); }
  Pickle::List Interpreter::List () const
  { return Arrayref (); }

  // Symbol table lookups.

  Pickle::Scalarref Interpreter::Scalarref (const string& name)
  { return Scalarref (name .c_str ()); }  // XXX embedded \0
  Pickle::Scalarref Interpreter::Scalarref (const char* name)
  {
    return Pickle::Scalarref
      (newRV (get_sv (const_cast<char*> (name), 1)), false);
  }
  Pickle::Arrayref Interpreter::Arrayref (const string& name)
  { return Arrayref (name .c_str ()); }  // XXX embedded \0
  Pickle::Arrayref Interpreter::Arrayref (const char* name)
  {
    return Pickle::Arrayref
      (newRV ((SV*) get_av (const_cast<char*> (name), 1)), false);
  }
  Pickle::Hashref Interpreter::Hashref (const string& name)
  { return Hashref (name .c_str ()); }  // XXX embedded \0
  Pickle::Hashref Interpreter::Hashref (const char* name)
  {
    return Pickle::Hashref
      (newRV ((SV*) get_hv (const_cast<char*> (name), 1)), false);
  }
  Pickle::Coderef Interpreter::Coderef (const string& name)
  { return Coderef (name .c_str ()); }  // XXX embedded \0
  Pickle::Coderef Interpreter::Coderef (const char* name)
  {
    return Pickle::Coderef
      (newRV ((SV*) get_cv (const_cast<char*> (name), 1)), false);
  }

  // Compilation.

  void
  Interpreter::require_module (const string& bare) const
  {
    eval_string (string ("require ") .append (bare));
  }

  Pickle::Scalar
  Interpreter::eval_string (const string& proggie) const
  {
    djSP;
    SV* retsv;
    Pickle::Scalar err;

    ENTER;
    SAVETMPS;
    // This is equivalent to `local($@)'.  XXX missing from perlapi.pod
    save_scalar (PL_errgv);

    retsv = eval_pv (const_cast<char*> (proggie.c_str()), G_SCALAR);

    // I wasted half a day figuring out that this is needed.
    POPMARK;

    SvREFCNT_inc (retsv);
    if (SvTRUE (ERRSV))
      err = newSVsv (ERRSV);

    FREETMPS;
    LEAVE;

    if (err)
      throw new Exception (err);

    return retsv;
  }

  // Transfering control from C++ to Perl.

  Pickle::Scalar
  Interpreter::call_function (const Pickle::Scalar& func,
			      const Pickle::List& args,
			      Context cx) const
  {
    AV* av = (AV*) SvRV (((const Pickle::Arrayref&) args) .imp);
    return call_function (const_cast<SV*> (func .imp), 1 + AvFILL (av),
			  AvARRAY (av), cx);
  }

  SV*
  Interpreter::call_function (SV* func, int argc, SV** argv, Context cx) const
  {
    djSP;
    SV* retsv;
    Pickle::Scalar err;
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
    // This is equivalent to `local($@)'.  XXX missing from perlapi.pod
    save_scalar (PL_errgv);

    PUSHMARK (sp);
    EXTEND (sp, argc);
    for (int i = 0; i < argc; i++)
      PUSHs (argv [i]);
    PUTBACK;

    numret = call_sv (func, ctx | G_EVAL);
    SPAGAIN;

    switch (ctx)
      {
      case G_ARRAY:
	sp -= numret;
	retsv = newRV_noinc ((SV*) av_make (numret, sp + 1));
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
      err = newSVsv (ERRSV);

    FREETMPS;
    LEAVE;

    if (err)
      throw new Exception (err);

    return retsv;
  }

  // Tranfering control from Perl to C++.

  static void
  propagate_to_perl (pTHX_ Exception* e)
  {
    // XXX This is not especially clever.
    sv_setsv (ERRSV, e->get_scalar () .get_imp ());
    delete e;
    croak ("%_", ERRSV);
  }

  void
  xs_entry_one_arg (pTHX_ CV* cv)
  {
    dXSARGS;
    if (items != 1)
      croak ("Usage: %s(arg)", GvNAME (CvGV (cv)));

    try
      {
	Pickle::Scalar arg (SvREFCNT_inc (ST (0)));
	Pickle::Scalar ret (((sub_one_arg) CvXSUBANY (cv) .any_ptr) (arg));
	/* The callback return value is basically a time bomb, an SV with
	   0 refcount: actually, its refcnt is about to drop to 0 when the
	   container (Scalar) dies.  Therefore, we must inc its refcount.
	   But we are putting it on the Perl arg stack, so it must be
	   "mortalized".  In other words, this replaces one terminal illness
	   with another.  A clever compiler might optimize away SvREFCNT_inc.
	   (nah...)
	*/
	ST (0) = SvREFCNT_inc (sv_2mortal (ret .get_imp ()));
      }
    catch (Exception* e)
      {
	propagate_to_perl (aTHX_ e);
      }
    XSRETURN (1);
  }

  void
  xs_entry_hashref (pTHX_ CV* cv)
  {
    dXSARGS;
    if ((items % 2) == 0)
      croak ("Usage: OBJECT->%s (NAME, VALUE, ...)", GvNAME (CvGV (cv)));

    HV* args = newHV ();
    for (int i = 1; i < items - 1; i += 2)
      hv_store_ent (args, ST (i), SvREFCNT_inc (ST (i + 1)), 0);

    try
      {
	Pickle::Scalar obj (SvREFCNT_inc (ST (0)));
	Pickle::Hashref arg (newRV_noinc ((SV*) args), false);
	Pickle::Scalar ret (((sub_hashref) CvXSUBANY (cv) .any_ptr)
			    (obj, arg));
	ST (0) = SvREFCNT_inc (sv_2mortal (ret .get_imp ()));
      }
    catch (Exception* e)
      {
	propagate_to_perl (aTHX_ e);
      }
    XSRETURN (1);
  }

  void
  xs_entry_list (pTHX_ CV* cv)
  {
    dXSARGS;
    Context cx;
    SV* retsv;
    AV* retav;

    SV* args = newRV_noinc ((SV*) av_make (items, & ST (0)));
    switch (GIMME_V)
      {
      case G_ARRAY : cx = LIST;   break;
      case G_SCALAR: cx = SCALAR; break;
      default      : cx = VOID;   break;
      }

    try
      {
	Pickle::List arglist (args);
	Pickle::List retlist (((sub) CvXSUBANY (cv) .any_ptr) (arglist, cx));
	retsv = SvREFCNT_inc (sv_2mortal (((Pickle::Arrayref&) retlist)
					  .get_imp ()));
      }
    catch (Exception* e)
      {
	propagate_to_perl (aTHX_ e);
      }

    retav = (AV*) SvRV (retsv);

    switch (cx)
      {
      case LIST:
	{
	  size_t sz = av_len (retav) + 1;
	  if (sz != 0)
	    {
	      if (sz > (size_t) items)
		EXTEND (sp, (I32) sz - items);

	      // Note: List AVs are never magical.
	      SV** svp = AvARRAY (retav);
	      for (size_t i = 0; i < sz; i++)
		ST (i) = SvREFCNT_inc (sv_2mortal (svp [i]));
	    }
	  XSRETURN (sz);
	}
	break;

      case SCALAR:
	{
	  size_t sz = av_len (retav) + 1;
	  if (sz != 0)
	    {
	      if (items < 1)
		EXTEND (sp, 1);

	      ST (0) = SvREFCNT_inc (sv_2mortal
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
  reg (pTHX_ const string& package, const string& name, void* fn,
       void (*xs) (pTHX_ CV*))
  {
    string fullname (package);
    fullname .append ("::") .append (name);
    CV* cv = newXS (const_cast<char*> (fullname .c_str ()),
		    xs, const_cast<char*> (__FILE__));
    CvXSUBANY (cv) .any_ptr = fn;
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub_one_arg fn) const
  {
    reg (aTHX_ package, name, (void*) fn, xs_entry_one_arg);
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub fn) const
  {
    reg (aTHX_ package, name, (void*) fn, xs_entry_list);
  }

  void
  Interpreter::define_sub (const string& package, const string& name,
			   sub_hashref fn) const
  {
    reg (aTHX_ package, name, (void*) fn, xs_entry_hashref);
  }

  // Exception class.

  Exception::Exception (const string& s)
  {
    Interpreter::vivify ();
    msg = s;
  }

  Exception::Exception (const char* s)
  {
    Interpreter::vivify ();
    msg = s;
  }

}
