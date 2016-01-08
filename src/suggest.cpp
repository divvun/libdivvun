/*
* Copyright (C) 2015-2016, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#include "suggest.hpp"

namespace gtd {

// or we could make an (h)fst out of these to match on lines :)
std::basic_regex<char> CG_SKIP_TAG (
	"^"
	"(#"
	"|&"
	"|@"
	"|<"
	"|ADD:"
	"|MAP:"
	"|REPLACE:"
	"|SELECT:"
	"|REMOVE:"
	"|IFF:"
	"|APPEND:"
	"|SUBSTITUTE:"
	"|REMVARIABLE:"
	"|SETVARIABLE:"
	"|DELIMIT:"
	"|MATCH:"
	"|SETPARENT:"
	"|SETCHILD:"
	"|ADDRELATION:"
	"|SETRELATION:"
	"|REMRELATION:"
	"|ADDRELATIONS:"
	"|SETRELATIONS:"
	"|REMRELATIONS:"
	"|MOVE:"
	"|MOVE-AFTER:"
	"|MOVE-BEFORE:"
	"|SWITCH:"
	"|REMCOHORT:"
	"|UNMAP:"
	"|COPY:"
	"|ADDCOHORT:"
	"|ADDCOHORT-AFTER:"
	"|ADDCOHORT-BEFORE:"
	"|EXTERNAL:"
	"|EXTERNAL-ONCE:"
	"|EXTERNAL-ALWAYS:"
	"|REOPEN-MAPPINGS:"
	").*"
	);


const hfst::HfstTransducer *readTransducer(const std::string& file) {
	hfst::HfstInputStream *in = NULL;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException e)
	{
		std::cerr << "ERROR: File does not exist." << std::endl;
		return NULL;
	}

	hfst::HfstTransducer* t = NULL;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "ERROR: Stream cannot be read." << std::endl;
			return NULL;
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

const std::pair<bool, StringVec> get_gentags(const std::string& tags) {
	StringVec gentags;
	bool suggest = false;
	for(auto& tag : split(tags)) {
		if(tag == "&SUGGEST") {
			suggest = true;
		}
		std::match_results<const char*> result;
		std::regex_match(tag.c_str(), result, CG_SKIP_TAG);
		if (result.empty()) {
			gentags.push_back(tag);
		}
	}
	return std::pair<bool, StringVec>(suggest, gentags);
}

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer *t, bool json)
{
	std::string wf;
	for (std::string line; std::getline(is, line);) {
		os << line << std::endl;
		if(line.size()>2 && line[0]=='"' && line[1]=='<') {
			wf = line;
		}
		else if(line.size()>2 && line[0]=='\t' && line[1]=='"') {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			int lemma_end = line.find("\" ");
			std::string lemma = line.substr(2, lemma_end-2);
			std::string tags = line.substr(lemma_end+2);
			auto suggen = get_gentags(tags);
			bool suggest = suggen.first;
			StringVec gentags = suggen.second;
			if(!suggest) {
				continue;
			}
			auto tagsplus = join(gentags, "+");
			auto ana = lemma+"+"+tagsplus;
			auto paths = t->lookup_fd({ ana }, -1, 10.0);
			os << ana<<"\t";
			if(paths->size() > 0) {
				for(auto& p : *paths) {
					for(auto& symbol : p.second) {
						// TODO: this is a hack to avoid flag diacritics; is there a way to make lookup skip them?
						if(symbol.size()>0 && symbol[0]!='@') {
							os << symbol;
						}
					}
					os << "\t";
				}
				os << std::endl;
			}
			else {
				os << "?" << std::endl;
			}
		}
		else {
			//pass
		}
	}
}

}
