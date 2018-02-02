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
#ifndef f9875edb262d205d_PIPESPEC_H
#define f9875edb262d205d_PIPESPEC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

// Would like to
// #include <experimental/filesystem>
// but need to support macos

#include <cstring>
#include <cerrno>
#include <functional>
#include <iostream>
#include <climits>
#include <libgen.h>
#include <locale>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <regex>
#include <vector>
#include <sys/stat.h>

// divvun-gramcheck:
#include "util.hpp"
// xml:
#include <pugixml.hpp>
// zips:
#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif	// HAVE_LIBARCHIVE

namespace divvun {

using std::string;
using std::stringstream;
using std::u16string;
using std::unique_ptr;
using std::vector;

class PipeSpec {
	public:
		explicit PipeSpec(const string& file);
		explicit PipeSpec(pugi::char_t* buff, size_t size);
		PipeSpec(PipeSpec const &) = delete;
		PipeSpec &operator=(PipeSpec const &) = delete;
		// PipeSpec(const pugi::xml_document& doc, const std::unordered_map<u16string, pugi::xml_node>& pipespec)
		// {
		// 	throw std::runtime_error("PipeSpec can't be copied ");
		// }
		std::unordered_map<u16string, pugi::xml_node> pnodes;
		string language;
	private:
		pugi::xml_document doc; // needs to be alive for as long as we're referring to nodes in it
};

class ArPipeSpec {
	public:
		explicit ArPipeSpec(const string& ar_path_, pugi::char_t* buff, size_t size)
			: ar_path(ar_path_)
			, spec(new PipeSpec(buff, size)) {}
		ArPipeSpec(ArPipeSpec const &) = delete;
		ArPipeSpec &operator=(ArPipeSpec const &) = delete;
		// ArPipeSpec(const unique_ptr<PipeSpec> spec, const string& ar_path)
		// {
		// 	throw std::runtime_error("ArPipeSpec can't be copied ");
		// }
		const string ar_path;
		unique_ptr<PipeSpec> spec;
};

template<typename Ret>
using ArEntryHandler = std::function<Ret(const string& ar_path, const void* buff, const size_t size)>;

const size_t AR_BLOCK_SIZE = 10240;

#ifdef HAVE_LIBARCHIVE
template<typename Ret>
Ret archiveExtract(const string& ar_path,
		   archive *ar,
		   const string& entry_pathname,
		   ArEntryHandler<Ret>procFile)
{
	struct archive_entry* entry = nullptr;
	for (int rr = archive_read_next_header(ar, &entry);
	     rr != ARCHIVE_EOF;
	     rr = archive_read_next_header(ar, &entry))
	{
		if (rr != ARCHIVE_OK)
		{
			throw std::runtime_error("Archive not OK");
		}
		string filename(archive_entry_pathname(entry));
		if (filename == entry_pathname) {
			size_t fullsize = 0;
			const struct stat* st = archive_entry_stat(entry);
			size_t buffsize = st->st_size;
			if (buffsize == 0) {
				std::cerr << archive_error_string(ar) << std::endl;
				throw std::runtime_error("libdivvun: ERROR: Got a zero length archive entry for " + filename);
			}
			string buff(buffsize, 0);
			for (;;) {
				ssize_t curr = archive_read_data(ar, &buff[0] + fullsize, buffsize - fullsize);
				if (0 == curr) {
					break;
				}
				else if (ARCHIVE_RETRY == curr) {
					continue;
				}
				else if (ARCHIVE_FAILED == curr) {
					throw std::runtime_error("libdivvun: ERROR: Archive broken (ARCHIVE_FAILED)");
				}
				else if (curr < 0) {
					throw std::runtime_error("libdivvun: ERROR: Archive broken " + std::to_string(curr));
				}
				else {
					fullsize += curr;
				}
			}
			return procFile(ar_path, buff.c_str(), fullsize);
		}
	} // while r != ARCHIVE_EOF
	throw std::runtime_error("libdivvun: ERROR: Couldn't find " + entry_pathname + " in archive");
}

template<typename Ret>
Ret readArchiveExtract(const string& ar_path,
		       const string& entry_pathname,
		       ArEntryHandler<Ret>procFile)
{
	struct archive* ar = archive_read_new();
#if USE_LIBARCHIVE_2
	archive_read_support_compression_all(ar);
#else
	archive_read_support_filter_all(ar);
#endif // USE_LIBARCHIVE_2

	archive_read_support_format_all(ar);
	int rr = archive_read_open_filename(ar, ar_path.c_str(), AR_BLOCK_SIZE);
	if (rr != ARCHIVE_OK)
	{
		stringstream msg;
		msg << "Archive path not OK: '" << ar_path << "'" << std::endl;
		std::cerr << msg.str(); // TODO: why does the below cut off the string?
		throw std::runtime_error(msg.str());
	}
	Ret ret = archiveExtract(ar_path, ar, entry_pathname, procFile);

#if USE_LIBARCHIVE_2
	archive_read_close(ar);
	archive_read_finish(ar);
#else
	archive_read_free(ar);
#endif // USE_LIBARCHIVE_2
	return ret;
}
#else
template<typename Ret>
Ret readArchiveExtract(const string& ar_path,
		       const string& entry_pathname,
		       ArEntryHandler<Ret>procFile)
{
	throw std::runtime_error("libdivvun: ERROR: Can't extract zipped archives -- this library has been compiled without libarchive support. Please ensure libarchive is installed, and recompile divvun-gramcheck with --enable-checker.");
}
#endif	// HAVE_LIBARCHIVE


inline
unique_ptr<ArPipeSpec> readArPipeSpec(const string& ar_path) {
	ArEntryHandler<unique_ptr<ArPipeSpec>> f = [] (const string& ar_path, const void* buff, const size_t size) {
		unique_ptr<ArPipeSpec> ar_spec = unique_ptr<ArPipeSpec>(new ArPipeSpec(ar_path, (pugi::char_t*)buff, size));
		return ar_spec;
	};
	return readArchiveExtract(ar_path, "pipespec.xml", f);
}


void writePipeSpecSh(const string& specfile, const u16string& pipename, bool json, std::ostream& os);
void writePipeSpecShDir(const string& specfile, bool json, const string& modesdir, bool nodebug);

/* Run this first to feel safe in indexing into args. */
void validatePipespecCmd(const pugi::xml_node& cmd, const std::unordered_map<string, string>& args);

}

#endif
