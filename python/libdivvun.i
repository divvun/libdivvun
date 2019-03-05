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
#include "../src/utf8.h"
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
%include "../src/utf8.h"

%template(StringVector) std::vector<std::string>;
%template(StringSet) std::set<std::string>;
%template(StringStringVectorMap) std::map<std::string, std::vector<std::string> >;

// TODO: Would it be possible to have ErrBytes defined in
// checkertypes.hpp? Seems like SWIG no longer understands the
// StringVector rep (seems like it gives the pointer instead of value
// on trying to access err.rep[0])
%inline %{
#include <locale>
#include <sstream>
#include <map>
#include <set>
	typedef std::vector<std::string> StringVector;
	typedef std::set<std::string> StringSet;
	typedef std::map<std::string, std::vector<std::string> > StringStringVectorMap;
	struct ErrBytes {
			std::string form;
			size_t beg;
			size_t end;
			std::string err;
			std::string dsc;
			StringVector rep;
                        std::string msg;
	};

	typedef std::map<std::string, std::pair<std::string, std::string>> ToggleIdsBytes;      // toggleIds[errtype] = msg;
	typedef std::vector<std::pair<std::string, std::pair<std::string, std::string>> > ToggleResBytes; // toggleRes = [(errtype_regex, msg), …];

	struct OptionBytes {
			std::string type;
			std::string name;
			ToggleIdsBytes choices;      // choices[errtype] = msg;
	};
	struct OptionBytesCompare {
			bool operator() (const OptionBytes& a, const OptionBytes& b) const {
				return a.name < b.name;
			}
	};
	typedef std::set<OptionBytes, OptionBytesCompare> OptionSetBytes;

	struct PrefsBytes {
			ToggleIdsBytes toggleIds;
			ToggleResBytes toggleRes;
			OptionSetBytes options;
	};
	typedef std::map<divvun::Lang, PrefsBytes> LocalisedPrefsBytes;
%}

%template(ErrBytesVector) std::vector<ErrBytes>;
%template(ToggleIdsBytes) std::map<std::string, std::pair<std::string, std::string> >;
%template(ToggleResBytes) std::vector<std::pair<std::string, std::pair<std::string, std::string> > >;
%template(OptionSetBytes) std::set<OptionBytes, OptionBytesCompare>;
%template(LocalisedPrefsBytes) std::map<divvun::Lang, PrefsBytes>;

%inline %{
	typedef std::vector<ErrBytes> ErrBytesVector;

	const std::string toUtf8(const std::u16string& from) {
		std::string to;
        	utf8::utf16to8(from.begin(), from.end(), std::back_inserter(to));
		return to;
	}

	const ErrBytesVector proc_errs_bytes(std::unique_ptr<divvun::Checker>& checker, const std::string& input) {
		std::stringstream ss = std::stringstream(input);
		const auto& errs = checker->proc_errs(ss);
		ErrBytesVector errs_bytes;
		for(const divvun::Err& e : errs) {
			std::vector<std::string> rep;
			for(const std::u16string& r : e.rep) {
				std::string r8;
				utf8::utf16to8(r.begin(), r.end(), std::back_inserter(r8));
				rep.push_back(r8);
			}
			std::string form8, err8, msg8, dsc8;
			utf8::utf16to8(e.form.begin(), e.form.end(), std::back_inserter(form8));
			utf8::utf16to8(e.err.begin(), e.err.end(), std::back_inserter(err8));
			utf8::utf16to8(e.msg.first.begin(), e.msg.first.end(), std::back_inserter(msg8));
			utf8::utf16to8(e.msg.second.begin(), e.msg.second.end(), std::back_inserter(dsc8));
			errs_bytes.push_back({
				form8,
				e.beg,
				e.end,
				err8,
				dsc8,
				rep,
                                msg8
			});
		}
		return errs_bytes;
	};

	const LocalisedPrefsBytes prefs_bytes(std::unique_ptr<divvun::Checker>& checker) {
		divvun::LocalisedPrefs prefs = checker->prefs();
		LocalisedPrefsBytes prefs_bytes;
		for(const std::pair<divvun::Lang, divvun::Prefs>& lp : prefs) {
			const divvun::Prefs& p = lp.second;
			PrefsBytes pb;
			for(const std::pair<divvun::ErrId, divvun::Msg>& em : p.toggleIds) {
				pb.toggleIds[toUtf8(em.first)] = std::make_pair(toUtf8(em.second.first), toUtf8(em.second.second));
			}
			// toggleRes TODO: can we get the regex as string out?
			for(const divvun::Option& o : p.options) {
				ToggleIdsBytes choices;
				for(const std::pair<divvun::ErrId, divvun::Msg>& c : o.choices) {
					choices[toUtf8(c.first)] = std::make_pair(toUtf8(c.second.first), toUtf8(c.second.second));
				}
				OptionBytes ob = OptionBytes {
					o.type, o.name, choices
				};
				pb.options.insert(ob);
			}
			prefs_bytes[lp.first] = pb;
		}
		return prefs_bytes;
	};

%}

