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

std::vector<std::string> cg_keywords = {
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


void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer *t)
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
