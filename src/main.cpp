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
	std::cerr <<"Reading transducer "<<argv[1]<<std::endl;
	const hfst::HfstTransducer *t = readTransducer(argv[1]);

	std::string wf;
	for (std::string line; std::getline(std::cin, line);) {
		if(line.size()>2 && line[0]=='"' && line[1]=='<') {
			wf = line;
			std::cout <<wf << std::endl;
		}
		else if(line.size()>2 && line[0]=='\t' && line[1]=='"') {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			int lemma_end = line.find("\" ");
			std::string lemma = line.substr(2, lemma_end-2);
			std::string tags = line.substr(lemma_end+2);
			// split removes empties; TODO: only keep certain tags:
			auto tagsplus = join(split(tags), "+");
			std::cout << line << std::endl;
			auto ana = lemma+"+"+tagsplus;
			auto paths = t->lookup_fd({ ana }, -1, 10.0);
			if(paths->size() > 0) {
				for(auto& p : *paths) {
					for(auto& symbol : p.second) {
						// TODO: this is a hack to avoid flag diacritics, is there a way to make lookup skip them?
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
			std::cout << line << std::endl;
		}
	}
	std::cout << "EOF" << std::endl;
}
