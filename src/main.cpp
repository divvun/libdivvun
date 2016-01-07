//! @file hfst-proc2.cc
//!
//! @brief Feedback module for Divvun grammar checker
//!
//! @author Kevin Brubeck Unhammer

//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

bool VERBOSE=false;

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>
// #include <ospell.h>

#include <iostream>
#include <fstream>
#include <iterator>

#include <vector>
#include <map>
#include <string>
#include <set>

using std::string;
using std::vector;
using std::pair;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <queue>

typedef std::vector<std::string> StringVec;

const hfst::HfstTransducer* readTransducer(const std::string & file) {
	hfst::HfstInputStream *in = NULL;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException e)
	{
		std::cerr << "ERROR: File does not exist." << std::endl;
		exit(1);
	}

	hfst::HfstTransducer* t = NULL;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "ERROR: Stream cannot be read." << std::endl;
			exit(1);
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == NULL) {
		std::cerr << "WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}

const std::string join(const StringVec vec, const std::string delim=" ") {
	std::ostringstream os;
	std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(os, delim.c_str()));
	std::string str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}
const StringVec split(const std::string str, const char delim=' ')
{
	std::string buf;
	std::stringstream ss(str);
	StringVec tokens;
	while (std::getline(ss, buf, delim)) {
		if(!buf.empty()) {
			tokens.push_back(buf);
		}
	}
	return tokens;
}

StringVec cg_keywords = {
	std::string("ADD:"),
	std::string("MAP:"),
	std::string("REPLACE:"),
	std::string("SELECT:"),
	std::string("REMOVE:"),
	std::string("IFF:"),
	std::string("APPEND:"),
	std::string("SUBSTITUTE:"),
	std::string("REMVARIABLE:"),
	std::string("SETVARIABLE:"),
	std::string("DELIMIT:"),
	std::string("MATCH:"),
	std::string("SETPARENT:"),
	std::string("SETCHILD:"),
	std::string("ADDRELATION:"),
	std::string("SETRELATION:"),
	std::string("REMRELATION:"),
	std::string("ADDRELATIONS:"),
	std::string("SETRELATIONS:"),
	std::string("REMRELATIONS:"),
	std::string("MOVE:"),
	std::string("MOVE-AFTER:"),
	std::string("MOVE-BEFORE:"),
	std::string("SWITCH:"),
	std::string("REMCOHORT:"),
	std::string("UNMAP:"),
	std::string("COPY:"),
	std::string("ADDCOHORT:"),
	std::string("ADDCOHORT-AFTER:"),
	std::string("ADDCOHORT-BEFORE:"),
	std::string("EXTERNAL:"),
	std::string("EXTERNAL-ONCE:"),
	std::string("EXTERNAL-ALWAYS:"),
	std::string("REOPEN-MAPPINGS:"),
};

inline int startswith(std::string big, std::string start)
{
	return big.compare(0, start.size(), start) == 0;
}

int main(int argc, char ** argv)
{
	if (argc != 2) {
		std::cerr <<"Expected hfstol as single arg, got:";
		for(int i=1; i<argc; ++i) {
			std::cerr<<" " <<argv[i];
		}
		std::cerr<<std::endl;
		return(EXIT_FAILURE);
	}
	if(VERBOSE) {
		std::cerr <<"Reading transducer "<<argv[1]<<std::endl;
	}
	const hfst::HfstTransducer *t = readTransducer(argv[1]);

	std::string wf;
	for (std::string line; std::getline(std::cin, line);) {
		std::cout << line << std::endl;
		if(line.size()>2 && line[0]=='"' && line[1]=='<') {
			wf = line;
		}
		else if(line.size()>2 && line[0]=='\t' && line[1]=='"') {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			int lemma_end = line.find("\" ");
			std::string lemma = line.substr(2, lemma_end-2);
			std::string tags = line.substr(lemma_end+2);
			StringVec gentags;
			bool suggest = false;
			for(auto& tag : split(tags)) {
				if(tag == "&SUGGEST") {
					suggest=true;
				}
				if(tag.size()>0 && !(tag[0]=='#' ||
						     tag[0]=='&' ||
						     tag[0]=='@' ||
						     tag[0]=='<' ||
						     startswith(tag, "SUBSTITUTE:") ||
						     startswith(tag, "COPY:") ||
						     startswith(tag, "ADD:") ||
						     startswith(tag, "MAP:")
					   )) {
					gentags.push_back(tag);
				}
			}
			if(!suggest) {
				continue;
			}
			auto tagsplus = join(gentags, "+");
			auto ana = lemma+"+"+tagsplus;
			auto paths = t->lookup_fd({ ana }, -1, 10.0);
			std::cout << ana<<"\t";
			if(paths->size() > 0) {
				for(auto& p : *paths) {
					for(auto& symbol : p.second) {
						// TODO: this is a hack to avoid flag diacritics; is there a way to make lookup skip them?
						if(symbol.size()>0 && symbol[0]!='@') {
							std::cout << symbol;
						}
					}
					std::cout << "\t";
				}
				std::cout << std::endl;
			}
			else {
				std::cout << "?" << std::endl;
			}
		}
		else {
			//pass
		}
	}
}
