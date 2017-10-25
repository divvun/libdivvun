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
#ifndef a361aad5b4636c78_PIPELINE_H
#define a361aad5b4636c78_PIPELINE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

// Would like to
// #include <experimental/filesystem>
// but need to support macos

// divvun-gramcheck:
#include "suggest.hpp"
#include "cgspell.hpp"
// xml:
#include <pugixml.hpp>
// cg3:
#include <cg3.h>
// hfst:
#include <hfst/implementations/optimized-lookup/pmatch.h>
#include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>
// zips:
#include <archive.h>
#include <archive_entry.h>


namespace divvun {

#ifndef DEBUG
const bool DEBUG=false;
#endif

using cg3_status = enum {
	CG3_ERROR   = 0,
	CG3_SUCCESS = 1
};


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


class PipeSpec {
	public:
		PipeSpec() = default;
		PipeSpec(PipeSpec const &) = delete;
		PipeSpec &operator=(PipeSpec const &) = delete;
		// PipeSpec(const pugi::xml_document& doc, const std::unordered_map<std::u16string, pugi::xml_node>& pipespec)
		// {
		// 	throw std::runtime_error("PipeSpec can't be copied ");
		// }
		pugi::xml_document doc; // needs to be alive for as long as we're referring to nodes in it
		std::unordered_map<std::u16string, pugi::xml_node> pnodes;
};

std::unique_ptr<PipeSpec> readPipeSpec(const std::string& file);

void writePipeSpecSh(const std::string& specfile, const std::u16string& pipename, std::ostream& os);

// https://stackoverflow.com/a/1449527/69663
struct OneShotReadBuf : public std::streambuf
{
    OneShotReadBuf(char* s, size_t n)
    {
        setg(s, s, s + n);
    }
};

class ArPipeSpec {
	public:
		explicit ArPipeSpec(const std::string& ar_path_)
			: ar_path(ar_path_)
			, spec(new PipeSpec) {}
		ArPipeSpec(ArPipeSpec const &) = delete;
		ArPipeSpec &operator=(ArPipeSpec const &) = delete;
		// ArPipeSpec(const std::unique_ptr<PipeSpec> spec, const std::string& ar_path)
		// {
		// 	throw std::runtime_error("ArPipeSpec can't be copied ");
		// }
		const std::string ar_path;
		std::unique_ptr<PipeSpec> spec;
};

std::unique_ptr<ArPipeSpec> readArPipeSpec(const std::string& ar_path);


class PipeCmd {
	public:
		PipeCmd() = default;
		virtual void run(std::stringstream& input, std::stringstream& output) const = 0;
		virtual ~PipeCmd() = default;
		// no copying
		PipeCmd(PipeCmd const &) = delete;
		PipeCmd &operator=(PipeCmd const &) = delete;
};


class TokenizeCmd: public PipeCmd {
	public:
		TokenizeCmd (std::istream& instream, bool verbose);
		TokenizeCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
		~TokenizeCmd() override = default;
	private:
		hfst_ol::PmatchContainer* mkContainer(std::istream& instream, bool verbose);
		hfst_ol_tokenize::TokenizeSettings settings;
		std::unique_ptr<std::istream> istream; // Only used if we're not given a stream in the first place
		std::unique_ptr<hfst_ol::PmatchContainer> container;
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
		void run(std::stringstream& input, std::stringstream& output) const override;
		~MweSplitCmd() override = default;
	private:
		std::unique_ptr<cg3_mwesplitapplicator, CGMweSplitApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};


class CGCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		CGCmd (const char* buff, const size_t size, bool verbose);
		CGCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
		~CGCmd() override = default;
	private:
		std::unique_ptr<cg3_grammar, CGGrammarDeleter> grammar;
		// cg3_grammar* grammar;
		std::unique_ptr<cg3_applicator, CGApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};


class CGSpellCmd: public PipeCmd {
	public:
		CGSpellCmd (hfst_ospell::Transducer* errmodel, hfst_ospell::Transducer* acceptor, bool verbose);
		CGSpellCmd (const std::string& err_path, const std::string& lex_path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
		~CGSpellCmd() override = default;
	private:
		std::unique_ptr<Speller> speller;
};


class SuggestCmd: public PipeCmd {
	public:
		SuggestCmd (const hfst::HfstTransducer* generator, divvun::msgmap msgs, bool verbose);
		SuggestCmd (const std::string& gen_path, const std::string& msg_path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
		std::vector<Err> run_errs(std::stringstream& input) const;
		~SuggestCmd() override = default;
		void setIgnores(const std::set<err_id>& ignores);
		const msgmap msgs;
	private:
		std::unique_ptr<const hfst::HfstTransducer> generator;
		std::set<err_id> ignores;
};



inline void parsePrefs(LocalisedPrefs& prefs, const pugi::xml_node& cmd) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const pugi::xml_node& pref: cmd.children()) {
		const auto type = pref.attribute("type").value();
		const auto name = pref.attribute("name").value();
		std::unordered_map<lang, std::unordered_map<err_id, msg>> lems;
		for (const pugi::xml_node& option: pref.children()) {
			const auto errId = utf16conv.from_bytes(option.attribute("err-id").value());
			for (const pugi::xml_node& label: option.children()) {
				const auto lang = label.attribute("xml:lang").value();
				const auto msg = utf16conv.from_bytes(label.text().get()); // or xml_raw_cdata(label);
				lems[lang][errId] = msg;
			}
		}
		for(const auto& lem : lems) {
			const lang& lang = lem.first;
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

inline void mergePrefsFromMsgs(LocalisedPrefs& prefs, const msgmap& msgs) {
	for(const auto& lm : msgs) {
		const lang& lang = lm.first;
		const ToggleIds& tids = lm.second.first;
		const ToggleRes& tres = lm.second.second;
		prefs[lang].toggleIds.insert(tids.begin(), tids.end());
		prefs[lang].toggleRes.insert(prefs[lang].toggleRes.end(), tres.begin(), tres.end());
	}
}

class Pipeline {
	public:
		Pipeline(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool verbose);
		Pipeline(const std::unique_ptr<ArPipeSpec>& spec, const std::u16string& pipename, bool verbose);
		// ~Pipeline() {
		// TODO: gives /usr/include/c++/6/bits/stl_construct.h:75:7: error: use of deleted function ‘std::unique_ptr<_Tp, _Dp>::unique_ptr(const std::unique_ptr<_Tp, _Dp>&) [with _Tp = divvun::PipeCmd; _Dp = std::default_delete<divvun::PipeCmd>]’
		// 	if (!cg3_cleanup()) {
		// 		std::cerr << "WARNING: Couldn't cleanup from CG3" << std::endl;
		// 	}
		// }
		void proc(std::stringstream& input, std::stringstream& output);
		std::vector<Err> proc_errs(std::stringstream& input);
		const bool verbose;
		// Preferences:
		void setIgnores(const std::set<err_id>& ignores);
		const LocalisedPrefs prefs;
	private:
		std::vector<std::unique_ptr<PipeCmd>> cmds;
		// the final command, if it is SuggestCmd, can also do non-stringly-typed output, see proc_errs
		SuggestCmd* suggestcmd;
		// "Real" constructors here since we can't init const members in constructor bodies:
		static Pipeline mkPipeline(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool verbose);
		static Pipeline mkPipeline(const std::unique_ptr<ArPipeSpec>& spec, const std::u16string& pipename, bool verbose);
		Pipeline (LocalisedPrefs prefs,
			  std::vector<std::unique_ptr<PipeCmd>> cmds,
			  SuggestCmd* suggestcmd,
			  bool verbose);
};

} // namespace divvun

#endif
