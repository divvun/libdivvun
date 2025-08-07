/*
* Copyright (C) 2021 Divvun
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef a0d6827329788a87_NORMALISER_HPP
#	define a0d6827329788a87_NORMALISER_HPP

#	include <locale>
#	include <vector>
#	include <string>
#	include <regex>
#	include <unordered_map>
#	include <exception>

// divvun-gramcheck:
#	include "util.hpp"
#	include "hfst_util.hpp"
// hfst:
#	include <hfst/implementations/optimized-lookup/pmatch.h>
#	include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>

namespace divvun {

using std::string;
using std::unique_ptr;
using std::vector;

class Normaliser {
public:
	Normaliser(const hfst::HfstTransducer* generator,
	  const hfst::HfstTransducer* sanalyser,
	  const hfst::HfstTransducer* danalyser, bool verbose, bool trace,
	  bool debug);
	Normaliser(const string& generator, const string& sanalyser,
	  const string& danalyser, bool verbose, bool trace, bool debug);
	void addNormaliser(
	  const std::string& tag, const hfst::HfstTransducer* normaliser);
	void addNormaliser(const std::string& tag, const std::string& normaliser);
	/*const*/ void run(std::istream& is, std::ostream& os);

private:
	void process_cohort(CGCohort& cohort, std::ostream& os);
	void process_reading(CGReading& reading, std::ostream& os);
	std::string process_subreading(CGReading& subreading, std::ostream& os);
	void mangle_reading(CGReading& reading, std::ostream& os);
	std::map<std::string, std::unique_ptr<const hfst::HfstTransducer>>
	  normalisers;
	unique_ptr<const hfst::HfstTransducer> generator;
	unique_ptr<const hfst::HfstTransducer> sanalyser;
	unique_ptr<const hfst::HfstTransducer> danalyser;
	bool verbose;
	bool trace;
	bool debug;
};

}

#endif
