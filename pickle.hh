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
  class Globref;

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

    friend class Scalar;
    friend class Scalarref;
    friend class Arrayref;
    friend class Hashref;
    friend class Coderef;
    friend class Globref;

  public:
    // Construct an interpreter with args "Pickle", "-e0"
    Interpreter ();

    // Specify perl's command line and environment.
    Interpreter (const std::vector<std::string>& args);
    Interpreter (const std::vector<std::string>& args,
		 const std::vector<std::string>& env);
    Interpreter (int argc, const char* const * argv,
		 const char* const * envp = 0);

    // Perform Perl global destruction and free its memory.
    ~Interpreter ();

    // Gloss over the interpreter/process distinction as much as possible.

    // Returns the currently running interpreter, or 0 if there is none.
    static Interpreter* get_current ();

    // Return true if there is a current interpreter.
    static bool ping () { return get_current () != 0; }

    // Create an Interpreter only if there isn't one already.
    static Interpreter* vivify ()
    {
      return ping () ? 0 : new Interpreter;
    }

    // Data allocation.
    Pickle::Scalar Scalar () const;
    Pickle::Scalar Scalar (unsigned long i) const;
    Pickle::Scalar Scalar (long i) const;
    Pickle::Scalar Scalar (unsigned int i) const;
    Pickle::Scalar Scalar (int i) const;
    Pickle::Scalar Scalar (unsigned short i) const;
    Pickle::Scalar Scalar (short i) const;
    Pickle::Scalar Scalar (unsigned char i) const;
    Pickle::Scalar Scalar (signed char i) const;
    Pickle::Scalar Scalar (const std::string& s) const;
    Pickle::Scalar Scalar (const char* s) const;
    Pickle::Scalar Scalar (const char* s, unsigned long len) const;
    Pickle::Scalar Scalar (double d) const;
    Pickle::Scalar Scalar (float d) const;
    Pickle::Scalar Scalar (bool b) const;
    Pickle::Scalarref Scalarref () const;
    Pickle::Arrayref Arrayref () const;
    Pickle::Hashref Hashref () const;
    Pickle::Globref Globref () const;
    Pickle::List List () const;

    // Symbol table lookups.
    Pickle::Scalarref Scalarref (const std::string& name);
    Pickle::Scalarref Scalarref (const char* name);
    Pickle::Arrayref Arrayref (const std::string& name);
    Pickle::Arrayref Arrayref (const char* name);
    Pickle::Hashref Hashref (const std::string& name);
    Pickle::Hashref Hashref (const char* name);
    Pickle::Coderef Coderef (const std::string& name);
    Pickle::Coderef Coderef (const char* name);
    Pickle::Globref Globref (const std::string& name);
    Pickle::Globref Globref (const char* name);

    // Perform `eval $code'.
    Pickle::Scalar eval_string (const std::string& code) const;

    // Perform `$func->(@$args)'.
    Pickle::Scalar call_function (const Pickle::Scalar& func,
				  const Pickle::List& args,
				  Context cx = SCALAR) const;
    inline Pickle::Scalar
    call_function (const Pickle::Scalar&, Context cx = SCALAR) const;

    // Perform `require module;' where module is a bare name like
    // "Data::Dumper".
    void require_module (const std::string& bare) const;

    void define_sub (const std::string& package, const std::string& name,
		     sub_one_arg fn) const;
    void define_sub (const std::string& package, const std::string& name,
		     sub_hashref fn) const;
    void define_sub (const std::string& package, const std::string& name, sub fn) const;

    // Perl operator equivalents.
    inline Pickle::Scalar undef () const;

  };

  class Init_Exception : public std::exception
  {
  private:
    std::string msg;

  public:
    Init_Exception (const std::string& m) : msg (m) {}
    ~Init_Exception () throw () {}
    const char* what () { return msg .c_str (); }
  };


  class Scalar
  {
  private:
    Scalar_imp* imp;  // imp is an SV pointer, no more, no less.

    // Kludge to let Arrayref::at et al. return Scalar&.
    static Scalar& ref (Scalar_imp*& ptr)
    { return *(Scalar*) &ptr; }
    static const Scalar& ref (const Scalar_imp*& ptr)
    { return *(const Scalar*) &ptr; }

    // Use `friend' rather than `protected' because we want
    // to be subclassed, but only as an interface.
    friend class Interpreter;
    friend class Scalarref;
    friend class Arrayref;
    friend class Hashref;
    friend class Coderef;
    friend class Globref;
    friend std::ostream& operator << (std::ostream& os, const Scalar& o);
    friend std::istream& operator >> (std::istream& os, Scalar& o);

  public:
    Scalar (Scalar_imp* i) : imp (i) {}
    Scalar_imp* get_imp () const { return imp; }

    // Manage reference counts.
    ~Scalar ();
    Scalar (const Scalar&);
    Scalar& operator= (const Scalar&);

    // 'undef'
    Scalar ();

    // XXX currently only calls Interpreter::get_current.
    const Interpreter* get_interpreter () const;
    Interpreter* get_interpreter ();

    // Runtime type information.
    // is_<type> - return true if this is of type <type>
    // check_<type> - die if not is_<type>()
    // <type> - convert this to type <type>, optionally checking.

    bool is_scalarref () const;
    void check_scalarref () const;
    Scalarref& scalarref (bool must_check)
    { if (must_check) check_scalarref (); return (Scalarref&) *this; }
    const Scalarref& scalarref (bool must_check) const
    { if (must_check) check_scalarref (); return (const Scalarref&) *this; }

    bool is_arrayref () const;
    void check_arrayref () const;
    Arrayref& arrayref (bool must_check)
    { if (must_check) check_arrayref (); return (Arrayref&) *this; }
    const Arrayref& arrayref (bool must_check) const
    { if (must_check) check_arrayref (); return (const Arrayref&) *this; }

    bool is_hashref () const;
    void check_hashref () const;
    Hashref& hashref (bool must_check)
    { if (must_check) check_hashref (); return (Hashref&) *this; }
    const Hashref& hashref (bool must_check) const
    { if (must_check) check_hashref (); return (const Hashref&) *this; }

    bool is_coderef () const;
    void check_coderef () const;
    Coderef& coderef (bool must_check)
    { if (must_check) check_coderef (); return (Coderef&) *this; }
    const Coderef& coderef (bool must_check) const
    { if (must_check) check_coderef (); return (const Coderef&) *this; }

    bool is_globref () const;
    void check_globref () const;
    Globref& globref (bool must_check)
    { if (must_check) check_globref (); return (Globref&) *this; }
    const Globref& globref (bool must_check) const
    { if (must_check) check_globref (); return (const Globref&) *this; }

    // Common Perl operator equivalents.
    // XXX Move implementations into class Interpreter.
    bool defined () const;
    Scalar ref () const;
    unsigned long length () const;
    bool eq (const Scalar&) const;
    Scalar can (const std::string& meth) const;
    bool isa (const std::string& package) const;
    void die () __attribute__((noreturn));

    // Conversions to and from native C++ types.

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

    Scalar (unsigned short);
    unsigned short as_ushort () const;
    operator unsigned short () const { return as_ushort (); }

    Scalar (short);
    short as_short () const;
    operator short () const { return as_short (); }

    Scalar (unsigned char);
    unsigned char as_uchar () const;
    operator unsigned char () const { return as_uchar (); }

    Scalar (signed char);
    signed char as_schar () const;
    operator signed char () const { return as_schar (); }

    Scalar (const std::string&);
    std::string as_string () const;
    operator std::string () const { return as_string (); }

    Scalar (const char* s);
    const char* as_c_str () const;
    Scalar (const char* s, unsigned long len);

    Scalar (double);
    double as_double () const;
    operator double () const { return as_double (); }

    Scalar (float);
    float as_float () const;
    operator float () const { return as_float (); }

    Scalar (bool);
    bool as_bool () const;
    operator bool () const { return as_bool (); }

    // Convert data to/from XML using XML::Dumper, which must have been loaded.
    std::string as_xml () const;
    static Scalar from_xml (const std::string& s);

    // Convert using Data::Dumper and `eval'.
    // `as_perl' requires Data::Dumper to have been loaded.
    std::string as_perl () const;
    static Scalar from_perl (const std::string& s);

    // Do the equivalent of `$this->$meth (@$args)'.
    // `const' is a lie, because Perl can't indicate which methods
    // are const.  Wrapper functions should declare themselves non-const
    // if the Perl method is "really" not const.
    Scalar call_method (const std::string& meth, const List& args,
			Context cx = SCALAR) const;
    // Do the equivalent of `$this->$meth'.  `const' is a lie.
    inline Scalar call_method (const std::string& meth,
			       Context cx = SCALAR) const;

    // THIS must point to a reference.  Bless it into package PKGNAME.
    void bless (const Scalar& pkgname);

  };
  // Use XML::Dumper if available, else Data::Dumper.
  std::ostream& operator << (std::ostream&, const Scalar&);
  // Assume XML::Dumper.
  std::istream& operator >> (std::istream&, Scalar&);


  class Exception : public std::exception
  {
  private:
    Scalar msg;

  public:
    Exception () {}
    ~Exception () throw () {}
    Exception (const Scalar& s) : msg (s) {}

    // These two initialize the interpreter if it hasn't been.
    Exception (const std::string& s);
    Exception (const char* s);

    Scalar get_scalar () { return msg; }
    virtual const char* what () { return msg .as_c_str (); }
  };


  class Scalarref : public Scalar
  {
  public:
    Scalarref (const Scalarref& r) : Scalar (r) {}
    static void check (const Scalar& s);  // Die if s is not a scalar ref.
    Scalarref (const Scalar& s, bool must_check = true) : Scalar (s)
    { if (must_check) check_scalarref (); }

    // Create a new, uninitialized value.
    Scalarref ();
    // Look up a scalar in the symbol table.
    Scalarref (const std::string& name);
    Scalarref (const char* name);

    Scalar fetch () const;
    void store (const Scalar&);
  };


  class Arrayref : public Scalar
  {
  public:
    Arrayref ();
    Arrayref (const Arrayref& r) : Scalar (r) {}
    static void check (const Scalar& s);  // Die if s is not an array ref.
    Arrayref (const Scalar& s, bool must_check = true) : Scalar (s)
    { if (must_check) check_arrayref (); }

    // Look up an array in the symbol table.
    Arrayref (const std::string& name);
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

  // Note that a list will never be based on a tied array.
  class List
  {
  private:
    Arrayref arrayref;

  public:
    List () {}
    List (const List& l) : arrayref (l.arrayref) {}
    List& operator= (const List& l) { arrayref = l.arrayref; return *this; }
    List (const Arrayref& a) : arrayref (a) {}
    List (const Scalar& s, bool must_check = true)
      : arrayref (s, must_check) {}

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
    Hashref (const Scalar& s, bool must_check = true) : Scalar (s)
    { if (must_check) check_hashref (); }

    // Lookup a hash in the symbol table.
    Hashref (const std::string& name);
    Hashref (const char* name);

    Scalar fetch (const Scalar& key) const;
    Scalar& store (const Scalar& key, const Scalar& val);

  };


  class Coderef : public Scalar
  {
  public:
    Coderef (const Coderef& r) : Scalar (r) {}
    Coderef (const Scalar& s, bool must_check = true) : Scalar (s)
    { if (must_check) check_coderef (); }

  };


  class Globref : public Scalar
  {
  public:
    Globref (const Globref& r) : Scalar (r) {}
    Globref (const Scalar& s, bool must_check = true) : Scalar (s)
    { if (must_check) check_globref (); }

  };


  inline Pickle::Scalar
  Interpreter::undef () const
  {
    return Scalar ();
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

  inline Pickle::Scalar
  Interpreter::call_function (const Pickle::Scalar& func,
			      Context cx) const
  {
    return call_function (func, List (), cx);
  }

  inline Scalar
  Scalar::call_method (const std::string& meth, Context cx) const
  {
    return call_method (meth, List (), cx);
  }

  // Perform `eval $code'.
  inline Scalar
  eval_string (const std::string& code)
  {
    return Interpreter::get_current () ->eval_string (code);
  }

  // Call function with args.
  inline Scalar
  call_function (const Scalar& func, const List& args,
		 Context cx = SCALAR)
  {
    return Interpreter::get_current () ->call_function (func, args, cx);
  }

  // Call function with no args.
  inline Scalar
  call_function (const Scalar& func, Context cx = SCALAR)
  {
    return Interpreter::get_current () ->call_function (func, cx);
  }

  inline void
  define_sub (const std::string& package, const std::string& name, sub_one_arg fn)
  {
    Interpreter::get_current () ->define_sub (package, name, fn);
  }

  inline void
  define_sub (const std::string& package, const std::string& name, sub_hashref fn)
  {
    Interpreter::get_current () ->define_sub (package, name, fn);
  }

  inline void
  define_sub (const std::string& package, const std::string& name, sub fn)
  {
    Interpreter::get_current () ->define_sub (package, name, fn);
  }

  inline void
  define_sub (const char* package, const char* name, sub fn)
  {
    define_sub (std::string (package), std::string (name), fn);
  }

}


#endif  // _PICKLE_HH
