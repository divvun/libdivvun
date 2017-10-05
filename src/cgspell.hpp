/*
* Copyright (C) 2015-2017, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#ifndef a1e13de0fc0e1f37_CGSPELL_H
#define a1e13de0fc0e1f37_CGSPELL_H

#include <locale>
#include <codecvt>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <exception>

// divvun-gramcheck:
#include "util.hpp"
// hfst:
#include <ZHfstOspeller.h>
// zhfstospeller.h conflicts with these:
// #include <hfst/HfstInputStream.h>
// #include <hfst/HfstTransducer.h>

namespace divvun {

void run_cgspell(std::istream& is, std::ostream& os, hfst_ol::ZHfstOspeller& s, int limit, int max_weight, int max_analysis_weight);

}

#endif
