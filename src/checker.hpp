/*
* Copyright (C) 2017, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#ifndef b28736af078229c6_CHECKER_H
#define b28736af078229c6_CHECKER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "checkertypes.hpp"

namespace divvun {

/**
 * Public types and functions for divvun-gramcheck library
 */


class PipeSpec;
class ArPipeSpec;
class Checker;

class CheckerSpec {
	public:
 		explicit CheckerSpec(const std::string& file);
		~CheckerSpec();
		bool hasPipe(const std::string& pipename);
		const std::set<std::string> pipeNames() const;
		std::unique_ptr<Checker> getChecker(const std::string& pipename, bool verbose);
	private:
		const std::unique_ptr<PipeSpec> pImpl;
};

class ArCheckerSpec {
	public:
 		explicit ArCheckerSpec(const std::string& file);
		~ArCheckerSpec();
		bool hasPipe(const std::string& pipename);
		const std::set<std::string> pipeNames() const;
		std::unique_ptr<Checker> getChecker(const std::string& pipename, bool verbose);
	private:
		const std::unique_ptr<ArPipeSpec> pImpl;
};

class Pipeline;

class Checker {
	public:
		Checker(const std::unique_ptr<PipeSpec>& spec, const std::string& pipename, bool verbose);
		Checker(const std::unique_ptr<ArPipeSpec>& spec, const std::string& pipename, bool verbose);
		~Checker();
                void proc(std::stringstream& input, std::stringstream& output);
		std::vector<Err> proc_errs(std::stringstream& input);
		const LocalisedPrefs& prefs() const;
		void setIgnores(const std::set<err_id>& ignores);
	private:
		const std::unique_ptr<Pipeline> pImpl;
};


} // namespace divvun

#endif
