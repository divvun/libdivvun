/*
* Copyright (C) 2015-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
#ifndef c9249b1422edf6fe_HFST_UTIL_H
#define c9249b1422edf6fe_HFST_UTIL_H

// hfst:
#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>

namespace divvun {

inline const hfst::HfstTransducer* readTransducer(std::istream& is) {
	hfst::HfstInputStream *in = nullptr;
	try
	{
		in = new hfst::HfstInputStream(is);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "libdivvun: ERROR: Stream not readable." << std::endl;
		return nullptr;
	}
	catch (HfstException& e) {
		std::cerr << "libdivvun: ERROR: HfstException." << std::endl;
		return nullptr;
	}

	hfst::HfstTransducer* t = nullptr;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "libdivvun: ERROR: Stream cannot be read." << std::endl;
			return nullptr;
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "libdivvun: WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == nullptr) {
		std::cerr << "libdivvun: WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}


inline const hfst::HfstTransducer* readTransducer(const string& file) {
	hfst::HfstInputStream *in = nullptr;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "libdivvun: ERROR: File does not exist." << std::endl;
		return nullptr;
	}
	catch (HfstException& e) {
		std::cerr << "libdivvun: ERROR: HfstException." << std::endl;
		return nullptr;
	}

	hfst::HfstTransducer* t = nullptr;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "libdivvun: ERROR: Stream cannot be read." << std::endl;
			return nullptr;
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "libdivvun: WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == nullptr) {
		std::cerr << "libdivvun: WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}

}

#endif
