/*
 * Copyright (C) 2017-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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


const std::u16string from_bytes(const string& s) {
	return fromUtf8(s);
}

// CheckerSpec
CheckerSpec::CheckerSpec(const string& file)
	: pImpl( new PipeSpec(file) )
{
}
CheckerSpec::~CheckerSpec()
{
}
const std::set<string> CheckerSpec::pipeNames() const
{
	std::set<string> keys;
	for(const auto& it : pImpl->pnodes) {
		keys.insert(toUtf8(it.first));
	}
	return keys;
}
bool CheckerSpec::hasPipe(const string& pipename) const
{
	return pImpl->pnodes.find(from_bytes(pipename)) != pImpl->pnodes.end();
}
std::unique_ptr<Checker> CheckerSpec::getChecker(const string& pipename, bool verbose) {
	return std::unique_ptr<Checker>(new Checker(pImpl, pipename, verbose));
}
const string CheckerSpec::defaultPipe() const
{
	return toUtf8(pImpl->default_pipe);
}



// ArCheckerSpec
ArCheckerSpec::ArCheckerSpec(const string& file)
	: pImpl( readArPipeSpec(file) )
{
}
ArCheckerSpec::~ArCheckerSpec()
{
}
const std::set<string> ArCheckerSpec::pipeNames() const
{
	std::set<string> keys;
	for(const auto& it : pImpl->spec->pnodes) {
		keys.insert(toUtf8(it.first));
	}
	return keys;
}
bool ArCheckerSpec::hasPipe(const string& pipename) const
{
	return pImpl->spec->pnodes.find(from_bytes(pipename)) != pImpl->spec->pnodes.end();
}
std::unique_ptr<Checker> ArCheckerSpec::getChecker(const string& pipename, bool verbose) {
	return std::unique_ptr<Checker>(new Checker(pImpl, pipename, verbose));
}
const string ArCheckerSpec::defaultPipe() const
{
	return toUtf8(pImpl->spec->default_pipe);
}


// Checker
Checker::Checker(const std::unique_ptr<PipeSpec>& spec, const string& pipename, bool verbose)
	: pImpl(new Pipeline(spec, from_bytes(pipename), verbose))
{
};

Checker::Checker(const std::unique_ptr<ArPipeSpec>& spec, const string& pipename, bool verbose)
	: pImpl(new Pipeline(spec, from_bytes(pipename), verbose))
{
};

Checker::~Checker()
{
};

void Checker::proc(stringstream& input, stringstream& output) {
	pImpl->proc(input, output);
};

vector<Err> Checker::proc_errs(stringstream& input) {
	return pImpl->proc_errs(input);
};

const LocalisedPrefs& Checker::prefs() const {
	return pImpl->prefs;
};

void Checker::setIgnores(const std::set<ErrId>& ignores) {
	return pImpl->setIgnores(ignores);
};


/**
 * Note: This will silently return an empty vector if the directory doesn't exist.
 * (We don't really care if some directory isn't there.)
 */
vector<string> zcheckFilesInDir(const string& path) {
	const string suffix = ".zcheck";
	const size_t suflen = suffix.length();
	vector<string> files;
	DIR *dir;
	if ((dir = opendir(path.c_str())) == NULL) {
		return files;
	}
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		string name = string(ent->d_name);
		if(name.length() > suflen && name.substr(name.length()-suflen, suflen) == suffix) {
			// TODO: \\ on Windows
			files.push_back(path + "/" + name);
		}
	}
	closedir(dir);
	return files;
}

variant<Nothing, string> expanduser() {
	const struct passwd* pwd = getpwuid(getuid());
	if (pwd)
	{
		return pwd->pw_dir;
	}
	const string HOME = std::getenv("HOME");
	if(!HOME.empty())
	{
		return HOME;
	}
	return Nothing();
}

std::set<string> searchPaths() {
	// TODO: prioritise user dir – make it a vector and check if
	// it's seen already?
	std::set<string> dirs = {
		// -DPREFIX is set in Makefile.am; doesn't include DESTDIR
		string(PREFIX) + "/share/voikko/4",
		"/usr/share/voikko/4",
		"/usr/local/share/voikko/4"
	};
	std::visit([&](auto&& arg){
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, Nothing>) {}
		if constexpr (std::is_same_v<T, string>) {
			dirs.insert(arg + "/.voikko/4");
			// TODO: getenv freedesktop stuff
			dirs.insert(arg + "/.config/voikko/4");
		}
	}, expanduser());
	return dirs;
}

std::map<Lang, vector<string>> listLangs(const std::string& extraPath) {
	std::map<Lang, vector<string>> pipes;
	std::set<string> paths = searchPaths();
	if(!extraPath.empty()) {
		paths.insert(extraPath);
	}
	for(const auto& d : paths) {
		const auto& zpaths = zcheckFilesInDir(d);
		for(const auto& zpath : zpaths) {
			const auto& ar_spec = readArPipeSpec(zpath);
			pipes[ar_spec->spec->language].push_back(zpath);
		}
	}
	return pipes;
}


}
