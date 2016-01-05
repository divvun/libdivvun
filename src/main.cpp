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
#include <ospell.h>

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



int main(int argc, char ** argv)
{
	FILE *fd = fopen(argv[1], "rb");
	hfst_ol::Transducer *t;
	t = new hfst_ol::Transducer(fd);
	for (std::string line; std::getline(std::cin, line);) {
		char* cline = &line[0];
		// TODO: lookup wants non-const, why?
		hfst_ol::AnalysisQueue aq = t->lookup(cline);
		std::cout << line << "\t";
		if(aq.size()>0) {
			hfst_ol::StringWeightPair ana = aq.top();
			std::cout << ana.first;
		}
		else {
			std::cout << "?";
		}
		std::cout << std::endl;
	}
	std::cerr << "unimplemented" << std::endl;
}
