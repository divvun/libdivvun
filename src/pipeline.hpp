/*
* Copyright (C) 2017-2021, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#ifndef a361aad5b4636c78_PIPELINE_H
#	define a361aad5b4636c78_PIPELINE_H

#	ifdef HAVE_CONFIG_H
#		include <config.h>
#	endif

#	include <cstring>
#	include <cerrno>

// divvun-gramcheck:
#	include "pipespec.hpp"
#	include "suggest.hpp"
#	ifdef HAVE_CGSPELL
#		include "cgspell.hpp"
#	endif
#	include "blanktag.hpp"
#	include "normaliser.hpp"
#	include "phon.hpp"
// xml:
#	include <pugixml.hpp>
// cg3:
#	include <cg3.h>
// hfst:
#	include <hfst/implementations/optimized-lookup/pmatch.h>
#	include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>

namespace divvun {

using std::size_t;
using std::string;
using std::stringstream;
using std::u16string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

#	ifndef DEBUG
const bool DEBUG = false;
#	endif

using cg3_status = enum { CG3_ERROR = 0, CG3_SUCCESS = 1 };


#	ifdef HAVE_LIBARCHIVE
// From archive.h, to make it usable with libarchive<3.2.2:
/* Get appropriate definitions of 64-bit integer */
#		if !defined(__LA_INT64_T_DEFINED)
/* Older code relied on the __LA_INT64_T macro; after 4.0 we'll switch to the typedef exclusively. */
#			if ARCHIVE_VERSION_NUMBER < 4000000
#				define __LA_INT64_T la_int64_t
#			endif
#			define __LA_INT64_T_DEFINED
#			if defined(_WIN32) && !defined(__CYGWIN__) && \
			  !defined(__WATCOMC__)
typedef __int64 la_int64_t;
#			else
#				include <unistd.h> /* ssize_t */
#				if defined(_SCO_DS) || defined(__osf__)
typedef long long la_int64_t;
#				else
typedef int64_t la_int64_t;
#				endif
#			endif
#		endif
#	endif // HAVE_LIBARCHIVE


// https://stackoverflow.com/a/1449527/69663
struct OneShotReadBuf : public std::streambuf {
	OneShotReadBuf(char* s, size_t n) { setg(s, s, s + n); }
};

class PipeCmd {
public:
	PipeCmd() = default;
	virtual void run(stringstream& input, stringstream& output) const = 0;
	virtual ~PipeCmd() = default;
	// no copying
	PipeCmd(PipeCmd const&) = delete;
	PipeCmd& operator=(PipeCmd const&) = delete;
};


class TokenizeCmd : public PipeCmd {
public:
	TokenizeCmd(std::istream& instream, int weight_classes, bool verbose);
	TokenizeCmd(const string& path, int weight_classes, bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~TokenizeCmd() override = default;

private:
	hfst_ol::PmatchContainer* mkContainer(
	  std::istream& instream, int weight_classes, bool verbose);
	hfst_ol_tokenize::TokenizeSettings settings;
	unique_ptr<std::istream>
	  istream; // Only used if we're not given a stream in the first place
	unique_ptr<hfst_ol::PmatchContainer> container;
};


struct CGApplicatorDeleter {
	void operator()(cg3_applicator* ptr) { cg3_applicator_free(ptr); }
};

struct CGMweSplitApplicatorDeleter {
	void operator()(cg3_mwesplitapplicator* ptr) {
		cg3_mwesplitapplicator_free(ptr);
	}
};

struct CGGrammarDeleter {
	void operator()(cg3_grammar* ptr) {
		cg3_grammar_free(ptr);
		if (!cg3_cleanup()) {
			std::cerr << "libdivvun: WARNING: Couldn't cleanup from CG3"
			          << std::endl;
		}
	}
};

class MweSplitCmd : public PipeCmd {
public:
	/* Assumes cg3_init has been called already */
	explicit MweSplitCmd(bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~MweSplitCmd() override = default;

private:
	unique_ptr<cg3_mwesplitapplicator, CGMweSplitApplicatorDeleter> applicator;
	// cg3_applicator* applicator;
};

class NormaliseCmd : public PipeCmd {
public:
	//        Normaliser(const string& normaliser, const string& generator,
	//                   const string& sanalyser, const string& danalyser,
	//                   const vector<string>& tags, bool verbose);
	explicit NormaliseCmd(const string& generator, const string& analyser,
	  const map<string, string>& normalisers, bool verbose);
	NormaliseCmd(const hfst::HfstTransducer* generator,
	  const hfst::HfstTransducer* analyser,
	  const map<string, const hfst::HfstTransducer*>& normalisers,
	  bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~NormaliseCmd() override = default;

private:
	unique_ptr<Normaliser> normaliser;
};

class CGCmd : public PipeCmd {
public:
	/* Assumes cg3_init has been called already */
	CGCmd(const char* buff, const size_t size, bool verbose, bool trace);
	CGCmd(const string& path, bool verbose, bool trace);
	void run(stringstream& input, stringstream& output) const override;
	~CGCmd() override = default;

private:
	unique_ptr<cg3_grammar, CGGrammarDeleter> grammar;
	// cg3_grammar* grammar;
	unique_ptr<cg3_applicator, CGApplicatorDeleter> applicator;
	// cg3_applicator* applicator;
};

#	ifdef HAVE_CGSPELL
class CGSpellCmd : public PipeCmd {
public:
	CGSpellCmd(hfst_ospell::Transducer* errmodel,
	  hfst_ospell::Transducer* acceptor, int limit, float beam,
	  float max_weight, float max_sent_unknown_rate, bool verbose);
	CGSpellCmd(const string& err_path, const string& lex_path, int limit,
	  float beam, float max_weight, float max_sent_unknown_rate, bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~CGSpellCmd() override = default;
	// Some sane defaults for the speller
	// TODO: Do we want any of this configurable from pipespec.xml, or from the Checker API?
	static constexpr Weight max_analysis_weight = -1.0;
	static constexpr bool real_word = false;
	static constexpr float time_cutoff = 0.0;

private:
	unique_ptr<Speller> speller;
};
#	endif

class BlanktagCmd : public PipeCmd {
public:
	BlanktagCmd(const hfst::HfstTransducer* analyser, bool verbose);
	BlanktagCmd(const string& ana_path, bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~BlanktagCmd() override = default;

private:
	unique_ptr<Blanktag> blanktag;
};

class PhonCmd : public PipeCmd {
public:
	PhonCmd(const hfst::HfstTransducer* analyser,
	  const map<string, const hfst::HfstTransducer*>& alttagfsas, bool verbose,
	  bool trace);
	PhonCmd(const string& ana_path, const map<string, string>& alttagpaths,
	  bool verbose, bool trace);
	void run(stringstream& input, stringstream& output) const override;
	~PhonCmd() override = default;

private:
	unique_ptr<Phon> phon;
};


class SuggestCmd : public PipeCmd {
public:
	SuggestCmd(const hfst::HfstTransducer* generator, divvun::MsgMap msgs,
	  const string& locale, bool verbose, bool generate_all_readings);
	SuggestCmd(const string& gen_path, const string& msg_path,
	  const string& locale, bool verbose, bool generate_all_readings);
	void run(stringstream& input, stringstream& output) const override;
	vector<Err> run_errs(stringstream& input) const;
	~SuggestCmd() override = default;
	void setIgnores(const std::set<ErrId>& ignores);
	void setIncludes(const std::set<ErrId>& includes);
	const MsgMap& getMsgs();

private:
	unique_ptr<Suggest> suggest;
};

class ShCmd : public PipeCmd {
public:
	ShCmd(const string& prog, const std::vector<string>& args, bool verbose);
	void run(stringstream& input, stringstream& output) const override;
	~ShCmd() override;

private:
	char** argv;
};

inline void parsePrefs(LocalisedPrefs& prefs, const pugi::xml_node& cmd) {
	for (const pugi::xml_node& pref : cmd.children()) {
		const auto type = pref.attribute("type").value();
		const auto name = pref.attribute("name").value();
		unordered_map<Lang, unordered_map<ErrId, Msg>> lems;
		for (const pugi::xml_node& option : pref.children()) {
			const auto errId = fromUtf8(option.attribute("err-id").value());
			for (const pugi::xml_node& label : option.children("label")) {
				const auto lang = label.attribute("xml:lang").value();
				const auto msg =
				  fromUtf8(label.text().get()); // or xml_raw_cdata(label);
				// Let <description> default to <label> first:
				lems[lang][errId] = std::make_pair(msg, msg);
			}
			for (const pugi::xml_node& description :
			  option.children("description")) {
				const auto lang = description.attribute("xml:lang").value();
				const auto msg = fromUtf8(description.text().get());
				if (lems[lang].find(errId) != lems[lang].end()) {
					lems[lang][errId].second = msg;
				}
				else {
					// No <label> for this language, fallback to <description>:
					lems[lang][errId] = std::make_pair(msg, msg);
				}
			}
		}
		for (const auto& lem : lems) {
			const Lang& lang = lem.first;
			Option o;
			o.type = type;
			o.name = name;
			for (const auto& em : lem.second) {
				o.choices[em.first] = em.second;
			}
			prefs[lang].options.insert(o);
		}
	}
};

inline void mergePrefsFromMsgs(LocalisedPrefs& prefs, const MsgMap& msgs) {
	for (const auto& lm : msgs) {
		const Lang& lang = lm.first;
		const ToggleIds& tids = lm.second.first;
		const ToggleRes& tres = lm.second.second;
		prefs[lang].toggleIds.insert(tids.begin(), tids.end());
		prefs[lang].toggleRes.insert(
		  prefs[lang].toggleRes.end(), tres.begin(), tres.end());
	}
}

class Pipeline {
public:
	Pipeline(const unique_ptr<PipeSpec>& spec, const u16string& pipename,
	  bool verbose, bool trace = false);
	Pipeline(const unique_ptr<ArPipeSpec>& spec, const u16string& pipename,
	  bool verbose, bool trace = false);
	// ~Pipeline() {
	// TODO: gives /usr/include/c++/6/bits/stl_construct.h:75:7: error: use of deleted function ‘unique_ptr<_Tp, _Dp>::unique_ptr(const unique_ptr<_Tp, _Dp>&) [with _Tp = divvun::PipeCmd; _Dp = std::default_delete<divvun::PipeCmd>]’
	// 	if (!cg3_cleanup()) {
	// 		std::cerr << "libdivvun: WARNING: Couldn't cleanup from CG3" << std::endl;
	// 	}
	// }


	// Run pipeline on input, printing to output
	void proc(stringstream& input, stringstream& output);

	// Run pipeline that ends in a SuggestCmd on input,
	// and instead of printing output with SuggestCmd.run,
	// we use SuggestCmd.run_errs as the last step
	vector<Err> proc_errs(stringstream& input);

	const bool verbose;
	const bool trace;
	// Preferences:
	void setIgnores(const std::set<ErrId>& ignores);
	void setIncludes(const std::set<ErrId>& includes);
	const LocalisedPrefs prefs;

private:
	vector<unique_ptr<PipeCmd>> cmds;
	// the final command, if it is SuggestCmd, can also do non-stringly-typed output, see proc_errs
	SuggestCmd* suggestcmd;
	// "Real" constructors here since we can't init const members in constructor bodies:
	static Pipeline mkPipeline(const unique_ptr<PipeSpec>& spec,
	  const u16string& pipename, bool verbose, bool trace);
	static Pipeline mkPipeline(const unique_ptr<ArPipeSpec>& spec,
	  const u16string& pipename, bool verbose, bool trace);
	Pipeline(LocalisedPrefs prefs, vector<unique_ptr<PipeCmd>> cmds,
	  SuggestCmd* suggestcmd, bool verbose, bool trace);
};

} // namespace divvun

#endif
