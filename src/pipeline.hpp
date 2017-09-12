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
#ifndef a361aad5b4636c78_CHECKER_H
#define a361aad5b4636c78_CHECKER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>

#include <pugixml.hpp>
// divvun-suggest:
#include "suggest.hpp"
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

typedef enum {
	CG3_ERROR   = 0,
	CG3_SUCCESS = 1
} cg3_status;


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
		PipeSpec() {}
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
		explicit ArPipeSpec(const std::string& ar_path)
			: ar_path(ar_path)
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
		PipeCmd() {};
		virtual void run(std::stringstream& input, std::stringstream& output) const = 0;
		virtual ~PipeCmd() {};
	private:
		// no copying
		PipeCmd(PipeCmd const &) = delete;
		PipeCmd &operator=(PipeCmd const &) = delete;
};


class TokenizeCmd: public PipeCmd {
	public:
		TokenizeCmd (std::istream& instream, bool verbose);
		TokenizeCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const;
		~TokenizeCmd() {};
	private:
		hfst_ol::PmatchContainer* mkContainer(std::istream& instream, bool verbose);
		bool tokenize_multichar = false; // Not useful for analysing text, only generation/bidix
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
			// cg3_mwesplitapplicator_free(ptr);
			std::cerr << "TODO: use cg3_mwesplitapplicator_free once that's available" << std::endl;
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
		void run(std::stringstream& input, std::stringstream& output) const;
		~MweSplitCmd() {};
	private:
		std::unique_ptr<cg3_mwesplitapplicator, CGMweSplitApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};


class CGCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		CGCmd (const char* buff, const size_t size, bool verbose);
		CGCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const;
		~CGCmd() {};
	private:
		std::unique_ptr<cg3_grammar, CGGrammarDeleter> grammar;
		// cg3_grammar* grammar;
		std::unique_ptr<cg3_applicator, CGApplicatorDeleter> applicator;
		// cg3_applicator* applicator;
};


class SuggestCmd: public PipeCmd {
	public:
		SuggestCmd (const hfst::HfstTransducer* generator, divvun::msgmap msgs, bool verbose);
		SuggestCmd (const std::string& gen_path, const std::string& msg_path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
		~SuggestCmd() {};
	private:
		std::unique_ptr<const hfst::HfstTransducer> generator;
		divvun::msgmap msgs;
};


class Pipeline {
	public:
		Pipeline(const std::u16string& pipename, bool v);
		Pipeline(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool verbose);
		Pipeline(const std::unique_ptr<ArPipeSpec>& spec, const std::u16string& pipename, bool verbose);
		// ~Pipeline() {
		// TODO: gives /usr/include/c++/6/bits/stl_construct.h:75:7: error: use of deleted function ‘std::unique_ptr<_Tp, _Dp>::unique_ptr(const std::unique_ptr<_Tp, _Dp>&) [with _Tp = divvun::PipeCmd; _Dp = std::default_delete<divvun::PipeCmd>]’
		// 	if (!cg3_cleanup()) {
		// 		std::cerr << "WARNING: Couldn't cleanup from CG3" << std::endl;
		// 	}
		// }
		void proc(std::stringstream& input, std::stringstream& output);
		const bool verbose;
	private:
		std::vector<std::unique_ptr<PipeCmd>> cmds;
};

} // namespace divvun

#endif
