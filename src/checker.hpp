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
#include <TextualParser.hpp>
#include <BinaryGrammar.hpp>
#include <GrammarApplicator.hpp>
#include <MweSplitApplicator.hpp>
// hfst:
#include <hfst/implementations/optimized-lookup/pmatch.h>
#include <hfst/implementations/optimized-lookup/pmatch_tokenize.h>
// zips:
#include <archive.h>
#include <archive_entry.h>

namespace gtd {

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


struct PipeSpec {
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

struct ArPipeSpec {
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

std::unique_ptr<ArPipeSpec> readArchive(const std::string& ar_path);
std::unique_ptr<ArPipeSpec> readArchiveSpec(const std::string& ar_path);
std::unique_ptr<ArPipeSpec> readArchiveTwice(const std::string& ar_path);


class PipeCmd {
	public:
		PipeCmd() {};
		virtual void run(std::stringstream& input, std::stringstream& output) const = 0;
	private:
		// no copying
		PipeCmd(PipeCmd const &) = delete;
		PipeCmd &operator=(PipeCmd const &) = delete;
};

class TokenizeCmd: public PipeCmd {
	public:

		TokenizeCmd (std::ifstream istream, bool verbose);
		TokenizeCmd (std::istringstream& instream, bool verbose);
		TokenizeCmd (std::istream& instream, bool verbose);
		TokenizeCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const;
	private:
		void setup(bool verbose);
		std::unique_ptr<hfst_ol::PmatchContainer> container;
		bool tokenize_multichar = false; // Not useful for analysing text, only generation/bidix
		hfst_ol_tokenize::TokenizeSettings settings;
};


class MweSplitCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		explicit MweSplitCmd (bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const;
	private:
		std::unique_ptr<CG3::Grammar> grammar;
		std::unique_ptr<CG3::MweSplitApplicator> applicator;
		static CG3::Grammar *load();
};


class CGCmd: public PipeCmd {
	public:
		/* Assumes cg3_init has been called already */
		CGCmd (const char* buff, const size_t size, bool verbose);
		CGCmd (const std::string& path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const;
	private:
		std::unique_ptr<CG3::Grammar> grammar;
		std::unique_ptr<CG3::GrammarApplicator> applicator;
		static CG3::Grammar *load_buffer(const char *string, const size_t size);
		static CG3::Grammar *load_file(const char *filename);
};


class SuggestCmd: public PipeCmd {
	public:
		SuggestCmd (const hfst::HfstTransducer* generator, gtd::msgmap msgs, bool verbose);
		SuggestCmd (const std::string& gen_path, const std::string& msg_path, bool verbose);
		void run(std::stringstream& input, std::stringstream& output) const override;
	private:
		std::unique_ptr<const hfst::HfstTransducer> generator;
		gtd::msgmap msgs;
};

class Pipeline {
	public:
		Pipeline(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool verbose);
		Pipeline(const std::unique_ptr<ArPipeSpec>& spec, const std::u16string& pipename, bool verbose);
		// ~Pipeline() {if (!cg3_cleanup()) { return EXIT_FAILURE; }}
		void proc(std::stringstream& input, std::stringstream& output);
		const bool verbose;
	private:
		std::vector<std::unique_ptr<PipeCmd>> cmds;

		cg3_status cg3_init(FILE *in, FILE *out, FILE *err);
};



}

#endif
