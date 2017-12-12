// Copyright (c) 2017 Kevin Brubeck Unhammer
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

// This is a swig interface file that is used to create python bindings for divvun-gramcheck.
// Everything will be visible under module 'libdivvun', but will be wrapped under
// package 'divvun'.

%module libdivvun
// Needed for type conversions between C++ and python.
%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"
%include "std_set.i"
%include "std_map.i"
%include "exception.i"

/* %include "std_wstring.i" */
/* %apply std::wstring { std::u16string }; */

/* %include "std_wstring.i" */
/* %apply std::wstring { std::u16string }; */

// %feature("autodoc", "3");

%init %{
%}

// Make swig aware of what Divvun offers.
%{
#include "../src/checkertypes.hpp"
#include "../src/checker.hpp"
%}

#ifdef _MSC_VER
%include <windows.h>
#endif

// Templates needed for conversion between C++ and Python datatypes.
//
// Note that templating order matters; simple templates used as part of
// more complex templates must be defined first, e.g. StringPair must be
// defined before StringPairSet. Also templates that are not used as such
// but are used as part of other templates must be defined.

%include "typemaps.i"

%include "std_unique_ptr.i"

wrap_unique_ptr(CheckerUniquePtr, divvun::Checker);

namespace std {

%template(StringVector) vector<string>;
%template(StringPair) pair<string, string>;
%template(StringPairVector) vector<pair<string, string > >;
%template(StringSet) set<string>;
%template(StringPairSet) set<pair<string, string> >;
}


/* %include "docstrings.i" */

%include "../src/checkertypes.hpp"
%include "../src/checker.hpp"

// ****************************************************** //
// ********** WHAT IS MADE AVAILABLE ON PYTHON ********** //
// ****************************************************** //

namespace divvun {
}


