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

  static inline SV*
  new_arrayref ()
  {
    dTHX;
    return newRV_noinc ((SV*) newAV ());
  }

  Arrayref::Arrayref () : Scalar (new_arrayref ()) {}

  static inline SV*
  new_arrayref (size_t len, SV** ary)
  {
    dTHX;
    return newRV_noinc
      ((SV*) av_make (len, ary));
  }

  Arrayref::Arrayref (size_t len, const Scalar* const & ss)
    : Scalar (new_arrayref (len, const_cast<SV**> (&ss [0] .imp))) {}

  static inline SV*
  lookup_array (const string& name)
  {
    dTHX;
    return newRV ((SV*) get_av (const_cast<char*> (name .c_str ()), 1));
  }

  Arrayref::Arrayref (const string& name) : Scalar (lookup_array (name)) {}
  Arrayref::Arrayref (const char* name) : Scalar (lookup_array (name)) {}

  size_t
  Arrayref::push (const Scalar& t)
  {
    dTHX;
    av_push ((AV*) SvRV (imp), SvREFCNT_inc (t.imp));
    return size ();
  }

  Arrayref&
  Arrayref::clear ()
  {
    dTHX;
    av_clear ((AV*) SvRV (imp));
    return *this;
  }

  size_t
  Arrayref::size () const
  {
    dTHX;
    return 1 + av_len ((AV*) SvRV (imp));
  }

  Scalar
  Arrayref::fetch (size_t index) const
  {
    dTHX;

    SV** loc = av_fetch ((AV*) SvRV (imp), index, 0);
    if (loc)
      return newSVsv (*loc);
    else
      return &PL_sv_undef;
  }

  const Scalar&
  Arrayref::at (size_t index) const
  {
    dTHX;
    return ref (*av_fetch ((AV*) SvRV (imp), index, 1));
  }

  Scalar&
  Arrayref::at (size_t index)
  {
    dTHX;
    return ref (*av_fetch ((AV*) SvRV (imp), index, 1));
  }

  Scalar
  Arrayref::shift ()
  {
    dInterp;
    return av_shift ((AV*) SvRV (imp));
  }

}
