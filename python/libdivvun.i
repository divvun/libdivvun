// Copyright (c) 2017 Kevin Brubeck Unhammer
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

// This is a SWIG interface file that is used to create Python
// bindings for divvun-gramcheck. The/ Python module will be named
// 'libdivvun'.

%module libdivvun

// Needed for type conversions between C++ and python.
%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"
%include "std_set.i"
%include "std_map.i"
%include "exception.i"
// TODO: Investigate if we can use u16string's via the wstring typemap
/* %include "std_wstring.i" */
/* %apply std::wstring { std::u16string }; */

%feature("autodoc", "3");

%naturalvar;

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

// We make our own ErrBytes instead of using the one from
// checkertypes.hpp, to work around lack of u16string support in SWIG:
%ignore divvun::Err;
%ignore divvun::Checker::proc_errs;
%ignore divvun::CheckerUniquePtr::proc_errs;

%include "../src/checkertypes.hpp"
%include "../src/checker.hpp"

%template(StringVector) std::vector<std::string>;

// TODO: Would it be possible to have ErrBytes defined in
// checkertypes.hpp? Seems like SWIG no longer understands the
// StringVector rep (seems like it gives the pointer instead of value
// on trying to access err.rep[0])
%inline %{
#include <locale>
#include <codecvt>
	typedef std::vector<std::string> StringVector;
	struct ErrBytes {
			std::string form;
			size_t beg;
			size_t end;
			std::string err;
			std::string msg;
			StringVector rep;
	};
%}

%template(ErrBytesVector) std::vector<ErrBytes>;

%inline %{
	typedef std::vector<ErrBytes> ErrBytesVector;

	const ErrBytesVector proc_errs_bytes(std::unique_ptr<divvun::Checker>& checker, const std::string& input) {
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		auto ss = std::stringstream(input);
		const auto& errs = checker->proc_errs(ss);
		ErrBytesVector errs_bytes;
		for(const divvun::Err& e : errs) {
			std::vector<std::string> rep;
			for(const std::u16string& r : e.rep) {
				rep.push_back(utf16conv.to_bytes(r));
			}
			errs_bytes.push_back({
					utf16conv.to_bytes(e.form),
						e.beg,
						e.end,
						utf16conv.to_bytes(e.err),
						utf16conv.to_bytes(e.msg),
						rep
						});
		}
		return errs_bytes;
	};

%}

