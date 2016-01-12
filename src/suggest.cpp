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

const std::string CG_SUGGEST_TAG = "&SUGGEST";

// or we could make an (h)fst out of these to match on lines :)
const std::basic_regex<char> CG_SKIP_TAG (
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

const std::basic_regex<char> CG_LINE ("^"
				      "(\"<(.*)>\".*" // wordform, group 2
				      "|(\t)+(\"[^\"]*\"\\S*)(\\s+\\S+)*" // reading, group 3, 4, 5
				      ")");


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
		if(tag == CG_SUGGEST_TAG) {
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

const std::tuple<bool, std::string, StringVec> get_sugg(const hfst::HfstTransducer *t, const std::string& line) {
	const auto& lemma_end = line.find("\" ");
	const auto& lemma = line.substr(2, lemma_end-2);
	const auto& tags = line.substr(lemma_end+2);
	const auto& suggen = get_gentags(tags);
	const auto& suggest = suggen.first;
	const auto& gentags = suggen.second;
	StringVec forms;
	if(!suggest) {
		return std::make_tuple(suggest, lemma+"+"+tags, forms);
	}
	const auto& tagsplus = join(gentags, "+");
	const auto& ana = lemma+"+"+tagsplus;
	const auto& paths = t->lookup_fd({ ana }, -1, 10.0);
	std::stringstream form;
	if(paths->size() > 0) {
		for(auto& p : *paths) {
			for(auto& symbol : p.second) {
				// TODO: this is a hack to avoid flag diacritics; is there a way to make lookup skip them?
				if(symbol.size()>0 && symbol[0]!='@') {
					form << symbol;
				}
			}
			forms.push_back(form.str());
		}
	}
	return std::make_tuple(suggest, ana, forms);
}


void run_json(std::istream& is, std::ostream& os, const hfst::HfstTransducer *t)
{
	json::sanity_test();
	int pos = 0;
	std::u16string etype = u"boasttu kásushápmi"; // TODO from &-tag
	bool first_err = true;
	bool first_word = true;
	std::u16string wf;
	std::ostringstream text;
	bool blank = false;

	// TODO: could use http://utfcpp.sourceforge.net, but it's not in macports;
	// and ICU seems overkill just for iterating codepoints
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	os << "{"
	   << json::key(u"errs")
	   << "[";
	for (std::string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		if (!result.empty() && result[2].length() != 0) {
			if(!blank && !first_word) {
				// Assume we had a single space if there wasn't a superblank
				text << " ";
				pos += 1;
			}
			first_word = false;
			wf = utf16conv.from_bytes(result[2]);
			pos += wf.size();
			text << utf16conv.to_bytes(wf);
			// TODO: wrapper for pos-increasing and text-adding, since they should always happen together
		}
		else if(!result.empty() && result[3].length() != 0) {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			const auto& sugg = get_sugg(t, line);
			if(std::get<0>(sugg)) {
				StringVec formv = std::get<2>(sugg);
				if(!first_err) {
					os << ",";
				}
				first_err = false;
				os << "[" << json::str(wf)
				   << "," << pos-wf.size()
				   << "," << pos
				   << "," << json::str(etype)
				   << "," << json::str_arr(formv)
				   << "]";
			}
		}
		else {
			// TODO: remove []superblank and \\'s from superblank?
			pos += utf16conv.from_bytes(line).size(); // something untokenised?
			text << line;
			blank = true;
		}
	}
	os << "]"
	   << "," << json::key(u"text") << json::str(utf16conv.from_bytes(text.str()))
	   << "}";
}

void run_cg(std::istream& is, std::ostream& os, const hfst::HfstTransducer *t)
{
	std::string wf;
	std::ostringstream ss;
	for (std::string line; std::getline(is, line);) {
		os << line << std::endl;
		if(line.size()>2 && line[0]=='"' && line[1]=='<') {
			wf = line;
		}
		else if(line.size()>2 && line[0]=='\t' && line[1]=='"') {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			const auto& sugg = get_sugg(t, line);
			if(std::get<0>(sugg)) {
				const auto& ana = std::get<1>(sugg);;
				const auto& formv = std::get<2>(sugg);
				if(formv.empty()) {
					os << ana << "\t" << "?" << std::endl;
				}
				else {
					os << ana << "\t" << join(formv) << std::endl;
				}
			}
		}
	}
}

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer *t, bool json)
{
	if(json) {
		run_json(is, os, t);
	}
	else {
		run_cg(is, os, t);
	}
}

}
