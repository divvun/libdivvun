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

const hfst::HfstTransducer* readTransducer(const std::string &file) {
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


int main(int argc, char ** argv)
{

	const hfst::HfstTransducer *t = readTransducer(argv[1]);
	// FILE *fd = fopen(argv[1], "rb");
	// hfst_ol::Transducer *t;
	// t = new hfst_ol::Transducer(fd);
	for (std::string line; std::getline(std::cin, line);) {
		std::cout << line << "\t";
		auto paths = t->lookup_fd(line, 1);
		if(paths->size() > 0) {
			auto it = paths->begin();
			auto hit = it->first;
			std::cout << hit;
		}
		else {
			std::cout << "?";
		}
		// char* cline = &line[0];
		// TODO: lookup wants non-const, why?
		// hfst_ol::AnalysisQueue aq = t->lookup(cline);
		// if(aq.size()>0) {
		// 	hfst_ol::StringWeightPair ana = aq.top();
		// 	std::cout << ana.first;
		// }
		// else {
		// 	std::cout << "?";
		// }
		std::cout << std::endl;
	}
	std::cerr << "unimplemented" << std::endl;

	auto testauto = 1+2;
	std::cout << testauto;
}
