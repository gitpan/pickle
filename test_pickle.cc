#include <iostream>
#include "pickle.hh"
#ifdef USE_DATE
#include "date.hh"
#endif

using namespace Pickle;

static Interpreter* p;

int
main (int argc, char** real_argv)
{

  try
    {
      const char* argv[] = { "perl", "-we0" };
      p = new Interpreter (sizeof argv / sizeof argv [0], argv);
    }
  catch (Init_Exception* e)
    {
      cerr << e->what () << endl;
      exit (1);
    }

  try
    {
      void test_except ();
      test_except ();

      void test_call ();
      test_call ();

      void test_cb ();
      test_cb ();

#ifdef USE_DATE
      void test_dates ();
      test_dates ();
#endif

      Scalar a1[] = { 1, "2.1" };
      Arrayref a (sizeof a1 / sizeof a1 [0], a1);
      cerr << "a has " << a .size () << " elements." << endl;
      a = a;
      cerr << "a is a " << a .ref () .as_string () << endl;

      cerr << (string) Scalar::lookup ("\030") << endl;

      Hashref h;
      h .store ("foo", 5L);
      h .store ("bar", "baz");
      cerr << int (h .fetch ("foo")) << string (h .fetch ("bar")) << endl;

      h .store ("ary", a);
      Arrayref (h .fetch ("ary")) .store (2, 4.9);

      cerr << 2.4 * cos (a[2]) << endl;
      cerr << ((int) a [0] + (int) a [1] + (int) a [2]) << endl;
      Scalar s = "+5.0";
      cerr << s;
      cerr << int (s) << endl;
      cerr << s;

      // Trigger "use of uninitialized" warning under `-w'.
      cerr << Scalar () .as_int () << endl;

      //Coderef sum = p->compile_sub ("my $x = 0; $x += $_ for @_; $x");

      //cerr << sum .call (List () << 7 << 14.0 << "21") .as_int () <<endl;

    }
  catch (Exception* e)
    {
      cerr << "test: " << e->what () << endl;
      delete e;
      exit (1);
    }
  try
    {
      if (p != 0)
	delete p;
    }
  catch (Exception* e)
    {
      cerr << "test: " << e->what () << endl;
      delete e;
      exit (1);
    }
  cerr << "ping: " << Interpreter::ping () << endl;
  return 0;
}

void
test_except ()
{
  try
    {
      eval_string ("$x = 0; 4 / $x");
    }
  catch (Exception* e)
    {
      cerr << "Exception caught: " << e->what () << endl;
      delete e;
    }
}

void
test_call ()
{
  eval_string ("use Cwd;");
  Scalar cwd = call_function ("cwd");
  cerr << "cwd is " << cwd .as_string () << endl;

  eval_string ("use File::Glob ':glob';");
  List args = List () << "~/*.doc"
		      << call_function ("GLOB_TILDE");
  List files = call_function ("glob", args, LIST);
  for (size_t i = 0; i < files.size(); i++)
    cout << (string) files[i] << endl;
}

static List my_cb (List& args, Context cx);
static Scalar my_hashref_cb (Scalar& self, Hashref& args);
static Scalar doit (Scalar& s)
{
  if (double (s) <= 0)
    throw new Exception (string ("'") .append (s)
			 .append ("' is not a positive number"));
  return sqrt (double (s));
}

void
test_cb ()
{
  define_sub ("Foo", "fa", (sub) my_cb);
  cerr << call_function ("Foo::fa", List () << 2 << 3 << 5 << 7, SCALAR);
  call_function ("Foo::fa", List () << 2 << 3 << 5 << 7, VOID);

  Scalar size, ans1, ans2;
  List l (call_function ("Foo::fa", List () << 2 << 3 << 5 << 7, LIST));
  l >> size >> ans1 >> ans2;
  cerr << int (size) << " " << int (ans1) << " " << int (ans2) << endl;

  p->define_sub ("Foo", "fah", my_hashref_cb);
  cerr << p->eval_string ("Foo->fah (baz => 21, foo => 'bla')");

  define_sub ("main", "do_it", doit);
  cout << eval_string ("do_it(42)") .as_double () << endl;

  eval_string ("eval { do_it(-100); }; warn \"got error: $@\" if $@");
}

static Scalar
my_hashref_cb (Scalar& self, Hashref& args)
{
  cerr << "self: " << string(self) << endl;
  cerr << "foo arg: " << args .fetch ("foo") .as_string () << endl;
  // trigger an uninit warning
  cerr << "bar arg: " << args .fetch ("bar") .as_string () << endl;
  return 2 * (int) args .fetch ("baz");
}

static List my_cb (List& args, Context cx)
{
  if (cx == VOID)
    {
      cerr << "my_cb called in void context." << endl;
      return List ();
    }
  return List () << args .size ()
		 << int (args[0]) + int (args[1])
		 << int (args[2]) + int (args[3]);
}

#ifdef USE_DATE
void
test_dates ()
{
  Interpreter::get_current () .require_module ("Date::NotTime");

  Date d1 (1969, 2, 19);
  Date d2 (1970, 8, 20);
  Date d3;

  cerr << "yesterday: " << (d3 - 1) << endl;
  cerr << "tomorrow: " << (d3 + 1) << endl;
  cerr << "age: " << (d3 - d1) << endl;
  cerr << d1 << " < " << d2 << "?  " << (d1 < d2) << endl;
  cerr << d2 << " < " << d1 << "?  " << (d2 < d1) << endl;
  cerr << d1 << " == " << d2 << "?  " << (d1 == d2) << endl;
  cerr << d2 << " != " << d3 << "?  " << (d2 != d3) << endl;
  d3 = Date (1970, 8, 20);
  cerr << d2 << " != " << d3 << "?  " << (d2 != d3) << endl;
  cerr << d1 .get_year () << "/" << d1 .get_month () << "/"
       << d1 .get_day () << endl;
  cerr << "I was " << (d1 .get_day_of_week () == Date::Wednesday ? "" : "not ")
       << "born on a Wednesday" << endl;
  enum Date::Week_Day wd = d2 .get_day_of_week ();
  cerr << "Dave was " << (wd == 4 ? "" : "not ")
       << "born on a Thursday" << endl;
}
#endif

struct Foo : public Scalar
{
public:
  Foo (int x) : Scalar (2 * x) {}
  double dbl () const { return 2 * as_int (); }
};
