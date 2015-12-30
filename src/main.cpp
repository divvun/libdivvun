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



int main(int argc, char ** argv)
{
	FILE *fd = fopen("foo.zhfst", "rb");
	hfst_ol::Transducer *t;
	t = new hfst_ol::Transducer(fd);
	std::cerr << "unimplemented" << std::endl;
}
