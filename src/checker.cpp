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

#include "checker.hpp"
#include "pipeline.hpp"


namespace divvun {


// CheckerSpec
CheckerSpec::CheckerSpec(const std::string& file) : pImpl( readPipeSpec(file) )
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for(const auto& k : pImpl->pnodes) {
		std::cerr << "init " << utf16conv.to_bytes(k.first.c_str()) <<std::endl;
	}
}
CheckerSpec::~CheckerSpec()
{
}
const std::set<std::u16string> CheckerSpec::pipeNames() const
{
	std::set<std::u16string> keys;
	for(const auto& it : pImpl->pnodes) {
		keys.insert(it.first);
	}
	return keys;
}
bool CheckerSpec::hasPipe(const std::u16string& pipename)
{
	return pImpl->pnodes.find(pipename) != pImpl->pnodes.end();
}
std::unique_ptr<Checker> CheckerSpec::getChecker(const std::u16string& pipename, bool verbose) {
	return std::unique_ptr<Checker>(new Checker(pImpl, pipename, verbose));
}



// ArCheckerSpec
ArCheckerSpec::ArCheckerSpec(const std::string& file) : pImpl( readArPipeSpec(file) )
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for(const auto& k : pImpl->spec->pnodes) {
		std::cerr << "init " << utf16conv.to_bytes(k.first.c_str()) <<std::endl;
	}
}
ArCheckerSpec::~ArCheckerSpec()
{
}
const std::set<std::u16string> ArCheckerSpec::pipeNames() const
{
	std::set<std::u16string> keys;
	for(const auto& it : pImpl->spec->pnodes) {
		keys.insert(it.first);
	}
	return keys;
}
bool ArCheckerSpec::hasPipe(const std::u16string& pipename)
{
	return pImpl->spec->pnodes.find(pipename) != pImpl->spec->pnodes.end();
}
std::unique_ptr<Checker> ArCheckerSpec::getChecker(const std::u16string& pipename, bool verbose) {
	return std::unique_ptr<Checker>(new Checker(pImpl, pipename, verbose));
}



// Checker
Checker::Checker(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool verbose)
	: pImpl(new Pipeline(spec, pipename, verbose))
{
};

Checker::Checker(const std::unique_ptr<ArPipeSpec>& spec, const std::u16string& pipename, bool verbose)
	: pImpl(new Pipeline(spec, pipename, verbose))
{
};

Checker::~Checker()
{
};

void Checker::proc(std::stringstream& input, std::stringstream& output) {
	pImpl->proc(input, output);
};

std::vector<Err> Checker::proc_errs(std::stringstream& input) {
	return pImpl->proc_errs(input);
};

const LocalisedPrefs& Checker::prefs() const {
	return pImpl->prefs;
};



}
