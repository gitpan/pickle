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

#ifndef _PICKLE_HH
#define _PICKLE_HH

#include <string>
#include <vector>

namespace Pickle
{

  class Interpreter;
  class Scalar;
  class Exception;
  class Scalarref;
  class Arrayref;
  class List;
  class Hashref;
  class Coderef;

#ifndef Interpreter_imp
  class Interpreter_imp;
#endif
#ifndef Scalar_imp
  class Scalar_imp;
#endif

  enum Context { SCALAR, LIST, VOID };

  typedef Scalar (*sub_one_arg) (Scalar&);
  typedef Scalar (*sub_hashref) (Scalar&, Hashref&);
  typedef List (*sub) (List&, Context);

  class Interpreter
  {
  private:
    Interpreter_imp* interpreter_imp;
    Interpreter (const Interpreter&);
    Interpreter& operator= (const Interpreter&);

#ifdef PICKLE_INTERPRETER_PRIVATE
    PICKLE_INTERPRETER_PRIVATE
#endif

  public:
    // Construct an interpreter with args "Pickle", "-e0"
    Interpreter ();

    // Specify perl's command line and environment.
    Interpreter (const vector<string>& args);
    Interpreter (const vector<string>& args, const vector<string>& env);
    Interpreter (int argc, const char* const * argv,
		 const char* const * envp = 0);

    // Perform Perl global destruction and free its memory.
    ~Interpreter ();

    // When you want a Perl interpreter but don't want a new one, try this.
    static const Interpreter& get_current ();

    // Return true if there is a current interpreter.
    static bool ping ();

    // Create an Interpreter only if there isn't one already.
    static Interpreter* vivify ()
    {
      return ping () ? 0 : new Interpreter;
    }

    // Perform `eval $code'.
    Scalar eval_string (const string& code) const;

    // Perform `$func->(@$args)'.
    Scalar call_function (const Scalar& func, const List& args,
			  Context cx = SCALAR) const;
    inline Scalar call_function (const Scalar&, Context cx = SCALAR) const;

    // Throw a $@ value.
    // XXX Too much copying.  I need to grok C++ exception mechanics.
    inline void die (const Scalar s) const __attribute__((noreturn));

    // Perform `require module;' where module is a bare name like
    // "Data::Dumper".
    void require_module (const string& bare) const;

    void define_sub (const string& package, const string& name,
		     sub_one_arg fn) const;
    void define_sub (const string& package, const string& name,
		     sub_hashref fn) const;
    void define_sub (const string& package, const string& name, sub fn) const;

  };
  inline static void die (const Scalar& msg) __attribute__((noreturn));

  class Init_Exception : public exception
  {
  private:
    string msg;

  public:
    Init_Exception (const string& m) : msg (m) {}
    const char* what () { return msg .c_str (); }
  };


  class Scalar
  {
  private:
    Scalar_imp* imp;
    Scalar (Scalar_imp* i) : imp (i) {}
    static Scalar& ref (Scalar_imp*& ptr) { return *(Scalar*) &ptr; }
    static const Scalar& ref (const Scalar_imp*& ptr)
    { return *(const Scalar*) &ptr; }

    // Use `friend' rather than `protected' because we want
    // to be subclassed, but only as an interface.
    friend class Interpreter;
    friend class Scalarref;
    friend class Arrayref;
    friend class Hashref;
    friend class Coderef;
    friend ostream& operator << (ostream& os, const Scalar& o);
    friend istream& operator >> (istream& os, Scalar& o);

  public:
    // Manage reference counts.
    ~Scalar ();
    Scalar (const Scalar&);

    // Create a Perl-level copy.
    Scalar& operator= (const Scalar&);

    // Create a scalar whose value is undef.
    Scalar ();

    // Return a scalar from the symbol table.
    static Scalar lookup (const string& name);

    // XXX currently only calls Interpreter::get_current.
    const Interpreter& get_interpreter () const;

    // Common Perl operator equivalents.
    bool defined () const;
    Scalar ref () const;
    bool eq (const Scalar&) const;

    Scalar (unsigned long);
    unsigned long as_ulong () const;
    operator unsigned long () const { return as_ulong (); }

    Scalar (long);
    long as_long () const;
    operator long () const { return as_long (); }

    Scalar (unsigned int);
    unsigned int as_uint () const;
    operator unsigned int () const { return as_uint (); }

    Scalar (int);
    int as_int () const;
    operator int () const { return as_int (); }

    Scalar (const string&);
    string as_string () const;
    operator string () const { return as_string (); }

    Scalar (const char* s);
    const char* as_c_str () const;

    Scalar (double);
    double as_double () const;
    operator double () const { return as_double (); }

    Scalar (bool);
    bool as_bool () const;
    operator bool () const { return as_bool (); }

    // Load the XML::Dumper module if not loaded yet.  Return true on success.
    static bool check_xml_dumper (const Interpreter&);
    static bool check_xml_dumper ()
    { return check_xml_dumper (Interpreter::get_current ()); }

    // Convert data to/from XML using XML::Dumper, which must have been loaded.
    string as_xml () const;
    static Scalar from_xml (const Interpreter&, const string&);
    static Scalar from_xml (const string& s)
    { return from_xml (Interpreter::get_current (), s); }

    // Convert using Data::Dumper and `eval'.
    // `as_perl' requires Data::Dumper to have been loaded.
    string as_perl () const;
    static Scalar from_perl (const Interpreter&, const string&);
    static Scalar from_perl (const string& s)
    { return from_perl (Interpreter::get_current (), s); }

    // Do the equivalent of `$this->$meth (@$args)'.
    Scalar call_method (const string& meth, const List& args,
			Context cx = SCALAR) const;
    inline Scalar call_method (const string&, Context cx = SCALAR) const;

    // Call the UNIVERSAL methods.
    Scalar can (const string& meth) const;
    bool isa (const string& package) const;

  };
  // Use XML::Dumper if available, else Data::Dumper.
  ostream& operator << (ostream&, const Scalar&);
  // Assume XML::Dumper.
  istream& operator >> (istream&, Scalar&);


  class Exception : public exception
  {
  private:
    Scalar scalar;

  public:
    Exception () {}
    Exception (const Scalar& s) : scalar (s) {}

    // These two initialize the interpreter if it hasn't been.
    Exception (const string& s);
    Exception (const char* s);

    Scalar get_scalar () { return scalar; }
    virtual const char* what () { return scalar .as_c_str (); }
  };


  class Scalarref : public Scalar
  {
  public:
    Scalarref (const Scalarref& r) : Scalar (r) {}
    Scalarref (const Scalar& s) : Scalar (s) {}

    // Create a new, uninitialized value.
    Scalarref ();
    // Look up a scalar in the symbol table.
    Scalarref (const string& name);
    Scalarref (const char* name);

    Scalar fetch () const;
    void store (const Scalar&);
  };


  class Arrayref : public Scalar
  {
  public:
    Arrayref ();
    Arrayref (const Arrayref& r) : Scalar (r) {}
    Arrayref (const Scalar& s) : Scalar (s) {}
    // Look up an array in the symbol table.
    Arrayref (const string& name);
    Arrayref (const char* name);
    Arrayref (size_t count, const Scalar* const & elts);
    inline Arrayref (const List&);

    size_t size () const;

    // Return an alias to an element.
    const Scalar& at (size_t index) const;
    Scalar& at (size_t index);
    const Scalar& operator[] (size_t index) const { return at (index); }
    Scalar& operator[] (size_t index) { return at (index); }

    Scalar fetch (size_t index) const;
    Scalar& store (size_t index, const Scalar& val)
    { return at (index) = val; }

    size_t push (const Scalar& elt);
    size_t push (const List& elts);
    inline List deref () const;

    Arrayref& clear ();
    Scalar shift ();
  };


  /* The purpose of the List class is to allow overloading functions
     based on whether a single arrayref or a list of scalars is meant,
     and to avoid confusing the compiler with the << operator since
     Scalar, and thus also Arrayref, has an operator int.
  */

  // Note that a list can never be tied.
  class List
  {
  private:
    Arrayref arrayref;

  public:
    List () {}
    List (const List& l) : arrayref (l.arrayref) {}
    List& operator= (const List& l) { arrayref = l.arrayref; return *this; }
    List (const Scalar& s) : arrayref (s) {}

    size_t size () const { return arrayref .size (); }
    List& add (const Scalar& s) { arrayref .push (s); return *this; }
    List& operator<< (const Scalar& s) { return add (s); }
    operator const Arrayref& () const { return arrayref; }
    operator Arrayref& () { return arrayref; }

    const Scalar& at (size_t index) const { return arrayref [index]; }
    Scalar& at (size_t index) { return arrayref [index]; }
    const Scalar& operator[] (size_t index) const { return at (index); }
    Scalar& operator[] (size_t index) { return at (index); }

    Scalar shift () { return arrayref .shift (); }
    List& operator>> (Scalar& s) { s = shift (); return *this; }
  };


  class Hashref : public Scalar
  {
  public:
    Hashref ();
    Hashref (const Hashref& r) : Scalar (r) {}
    Hashref (const Scalar& s) : Scalar (s) {}
    // Lookup a hash in the symbol table.
    Hashref (const string& name);
    Hashref (const char* name);

    Scalar fetch (const Scalar& key) const;
    Scalar& store (const Scalar& key, const Scalar& val);

  };


  class Coderef : public Scalar
  {
  public:
    Coderef (const Coderef& r) : Scalar (r) {}
    Coderef (const Scalar& s) : Scalar (s) {}

  };


  // XXX Make the globals the "real" version and the methods inline.

  inline void
  Interpreter::die (Scalar s) const
  {
    throw new Exception (s);
  }

  inline List
  Arrayref::deref () const
  {
    List l;
    size_t len = size ();
    for (size_t i = 0; i < len; i++)
      l .add (at (i));
    return l;
  }

  inline Scalar
  Interpreter::call_function (const Scalar& func, Context cx = SCALAR) const
  {
    return call_function (func, List (), cx);
  }

  inline Scalar
  Scalar::call_method (const string& meth, Context cx = SCALAR) const
  {
    return call_method (meth, List (), cx);
  }

  inline void
  die (const Scalar& err)
  {
    Interpreter::get_current () .die (err);
  }

  // Perform `eval $code'.
  inline Scalar
  eval_string (const string& code)
  {
    return Interpreter::get_current () .eval_string (code);
  }

  // Call function with args.
  inline Scalar
  call_function (const Scalar& func, const List& args,
		 Context cx = SCALAR)
  {
    return Interpreter::get_current () .call_function (func, args, cx);
  }

  // Call function with no args.
  inline Scalar
  call_function (const Scalar& func, Context cx = SCALAR)
  {
    return Interpreter::get_current () .call_function (func, cx);
  }

  inline void
  define_sub (const string& package, const string& name, sub_one_arg fn)
  {
    Interpreter::get_current () .define_sub (package, name, fn);
  }

  inline void
  define_sub (const string& package, const string& name, sub_hashref fn)
  {
    Interpreter::get_current () .define_sub (package, name, fn);
  }

  inline void
  define_sub (const string& package, const string& name, sub fn)
  {
    Interpreter::get_current () .define_sub (package, name, fn);
  }

  inline void
  define_sub (const char* package, const char* name, sub fn)
  {
    define_sub (string (package), string (name), fn);
  }

}


#endif  // _PICKLE_HH
