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
#pragma once
#ifndef a361aad5b4636c78_PIPELINE_H
#define a361aad5b4636c78_PIPELINE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>
#include <cerrno>

// divvun-gramcheck:
#include "pipespec.hpp"
#include "suggest.hpp"
#ifdef HAVE_CGSPELL
#include "cgspell.hpp"
#endif
#include "blanktag.hpp"
// xml:
#include <pugixml.hpp>
// cg3:
#include <cg3.h>
// hfst:
#include <hfst/implementations/optimized-lookup/pmatch.h>
#include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>

namespace divvun {

using std::string;
using std::stringstream;
using std::u16string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::size_t;

#ifndef DEBUG
const bool DEBUG=false;
#endif

using cg3_status = enum {
	CG3_ERROR   = 0,
	CG3_SUCCESS = 1
};


#ifdef HAVE_LIBARCHIVE
// From archive.h, to make it usable with libarchive<3.2.2:
/* Get appropriate definitions of 64-bit integer */
#if !defined(__LA_INT64_T_DEFINED)
/* Older code relied on the __LA_INT64_T macro; after 4.0 we'll switch to the typedef exclusively. */
# if ARCHIVE_VERSION_NUMBER < 4000000
#define __LA_INT64_T la_int64_t
# endif
#define __LA_INT64_T_DEFINED
# if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
typedef __int64 la_int64_t;
# else
# include <unistd.h>  /* ssize_t */
#  if defined(_SCO_DS) || defined(__osf__)
typedef long long la_int64_t;
#  else
typedef int64_t la_int64_t;
#  endif
# endif
#endif
#endif	// HAVE_LIBARCHIVE


// https://stackoverflow.com/a/1449527/69663
struct OneShotReadBuf : public std::streambuf
{
    OneShotReadBuf(char* s, size_t n)
    {
        setg(s, s, s + n);
    }
};

class PipeCmd {
	public:
		PipeCmd() = default;
		virtual void run(stringstream& input, stringstream& output) const = 0;
		virtual ~PipeCmd() = default;
		// no copying
		PipeCmd(PipeCmd const &) = delete;
		PipeCmd &operator=(PipeCmd const &) = delete;
};


class TokenizeCmd: public PipeCmd {
	public:
		TokenizeCmd (std::istream& instream, int weight_classes, bool verbose);
		TokenizeCmd (const string& path, int weight_classes, bool verbose);
		void run(stringstream& input, stringstream& output) const override;
		~TokenizeCmd() override = default;
	private:
		hfst_ol::PmatchContainer* mkContainer(std::istream& instream, int weight_classes, bool verbose);
		hfst_ol_tokenize::TokenizeSettings settings;
		unique_ptr<std::istream> istream; // Only used if we're not given a stream in the first place
		unique_ptr<hfst_ol::PmatchContainer> container;
};


struct CGApplicatorDeleter {
		void operator()(cg3_applicator* ptr)
		{
			cg3_applicator_free(ptr);
		}
};

struct CGMweSplitApplicatorDeleter {
		void operator()(cg3_mwesplitapplicator* ptr)
		{
			cg3_mwesplitapplicator_free(ptr);
		}
};

struct CGGrammarDeleter {
		void operator()(cg3_grammar* ptr)
		{
			cg3_grammar_free(ptr);
		}
};

class MweSplitCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		explicit MweSplitCmd (bool verbose);
		void run(stringstream& input, stringstream& output) const override;
		~MweSplitCmd() override = default;
	private:
		unique_ptr<cg3_mwesplitapplicator, CGMweSplitApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};


class CGCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		CGCmd (const char* buff, const size_t size, bool verbose);
		CGCmd (const string& path, bool verbose);
		void run(stringstream& input, stringstream& output) const override;
		~CGCmd() override = default;
	private:
		unique_ptr<cg3_grammar, CGGrammarDeleter> grammar;
		// cg3_grammar* grammar;
		unique_ptr<cg3_applicator, CGApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};

#ifdef HAVE_CGSPELL
class CGSpellCmd: public PipeCmd {
	public:
		CGSpellCmd (hfst_ospell::Transducer* errmodel, hfst_ospell::Transducer* acceptor, float max_sent_unknown_rate, bool verbose);
		CGSpellCmd (const string& err_path, const string& lex_path, float max_sent_unknown_rate, bool verbose);
		void run(stringstream& input, stringstream& output) const override;
		~CGSpellCmd() override = default;
		// Some sane defaults for the speller
		// TODO: Do we want any of this configurable from pipespec.xml, or from the Checker API?
		static constexpr Weight max_analysis_weight = -1.0;
		static constexpr Weight max_weight = 5000.0;
		static constexpr bool real_word = false;
		static constexpr unsigned long limit = 10;
		static constexpr hfst_ospell::Weight beam = 15.0;
		static constexpr float time_cutoff = 0.0;
	private:
		unique_ptr<Speller> speller;
};
#endif

class BlanktagCmd: public PipeCmd {
	public:
		BlanktagCmd (const hfst::HfstTransducer* analyser, bool verbose);
		BlanktagCmd (const string& ana_path, bool verbose);
		void run(stringstream& input, stringstream& output) const override;
		~BlanktagCmd() override = default;
	private:
		unique_ptr<Blanktag> blanktag;
};


class SuggestCmd: public PipeCmd {
	public:
		SuggestCmd (const hfst::HfstTransducer* generator, divvun::MsgMap msgs, const string& locale, bool verbose, bool generate_all_readings);
		SuggestCmd (const string& gen_path, const string& msg_path, const string& locale, bool verbose, bool generate_all_readings);
		void run(stringstream& input, stringstream& output) const override;
		vector<Err> run_errs(stringstream& input) const;
		~SuggestCmd() override = default;
		void setIgnores(const std::set<ErrId>& ignores);
		const MsgMap& getMsgs();
	private:
		unique_ptr<Suggest> suggest;
};



inline void parsePrefs(LocalisedPrefs& prefs, const pugi::xml_node& cmd) {
	for (const pugi::xml_node& pref: cmd.children()) {
		const auto type = pref.attribute("type").value();
		const auto name = pref.attribute("name").value();
		unordered_map<Lang, unordered_map<ErrId, Msg>> lems;
		for (const pugi::xml_node& option: pref.children()) {
			const auto errId = fromUtf8(option.attribute("err-id").value());
			for (const pugi::xml_node& label: option.children("label")) {
				const auto lang = label.attribute("xml:lang").value();
				const auto msg = fromUtf8(label.text().get()); // or xml_raw_cdata(label);
				// Let <description> default to <label> first:
				lems[lang][errId] = std::make_pair(msg, msg);
			}
			for (const pugi::xml_node& description: option.children("description")) {
				const auto lang = description.attribute("xml:lang").value();
				const auto msg = fromUtf8(description.text().get());
				if(lems[lang].find(errId) != lems[lang].end()) {
					lems[lang][errId].second = msg;
                                }
				else {
					// No <label> for this language, fallback to <description>:
					lems[lang][errId] = std::make_pair(msg, msg);
				}
                        }
		}
		for(const auto& lem : lems) {
			const Lang& lang = lem.first;
			Option o;
			o.type = type;
			o.name = name;
			for(const auto& em : lem.second) {
				o.choices[em.first] = em.second;
			}
			prefs[lang].options.insert(o);
		}
	}
};

inline void mergePrefsFromMsgs(LocalisedPrefs& prefs, const MsgMap& msgs) {
	for(const auto& lm : msgs) {
		const Lang& lang = lm.first;
		const ToggleIds& tids = lm.second.first;
		const ToggleRes& tres = lm.second.second;
		prefs[lang].toggleIds.insert(tids.begin(), tids.end());
		prefs[lang].toggleRes.insert(prefs[lang].toggleRes.end(), tres.begin(), tres.end());
	}
}

class Pipeline {
	public:
		Pipeline(const unique_ptr<PipeSpec>& spec, const u16string& pipename, bool verbose);
		Pipeline(const unique_ptr<ArPipeSpec>& spec, const u16string& pipename, bool verbose);
		// ~Pipeline() {
		// TODO: gives /usr/include/c++/6/bits/stl_construct.h:75:7: error: use of deleted function ‘unique_ptr<_Tp, _Dp>::unique_ptr(const unique_ptr<_Tp, _Dp>&) [with _Tp = divvun::PipeCmd; _Dp = std::default_delete<divvun::PipeCmd>]’
		// 	if (!cg3_cleanup()) {
		// 		std::cerr << "libdivvun: WARNING: Couldn't cleanup from CG3" << std::endl;
		// 	}
		// }
		void proc(stringstream& input, stringstream& output);
		vector<Err> proc_errs(stringstream& input);
		const bool verbose;
		// Preferences:
		void setIgnores(const std::set<ErrId>& ignores);
		const LocalisedPrefs prefs;
	private:
		vector<unique_ptr<PipeCmd>> cmds;
		// the final command, if it is SuggestCmd, can also do non-stringly-typed output, see proc_errs
		SuggestCmd* suggestcmd;
		// "Real" constructors here since we can't init const members in constructor bodies:
		static Pipeline mkPipeline(const unique_ptr<PipeSpec>& spec, const u16string& pipename, bool verbose);
		static Pipeline mkPipeline(const unique_ptr<ArPipeSpec>& spec, const u16string& pipename, bool verbose);
		Pipeline (LocalisedPrefs prefs,
			  vector<unique_ptr<PipeCmd>> cmds,
			  SuggestCmd* suggestcmd,
			  bool verbose);
};

} // namespace divvun

#endif
