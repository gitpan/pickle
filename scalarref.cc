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
  new_undef_scalarref ()
  {
    dTHX;
    return newRV_noinc (newSVsv (&PL_sv_undef));
  }

  Scalarref::Scalarref () : Scalar (new_undef_scalarref ()) {}

  static inline SV*
  lookup_scalar (const string& name)
  {
    dTHX;
    return newRV ((SV*) get_sv (const_cast<char*> (name .c_str ()), 1));
  }

  Scalarref::Scalarref (const string& name) : Scalar (lookup_scalar (name)) {}
  Scalarref::Scalarref (const char* name) : Scalar (lookup_scalar (name)) {}

  Scalar
  Scalarref::fetch () const
  {
    dInterp;
    return newSVsv (SvRV (imp));
  }

  void
  Scalarref::store (const Scalar& v)
  {
    dInterp;
    sv_setsv (SvRV (imp), v.imp);
  }

}
