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
	"|&(.+)"		// errtype
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


const std::basic_regex<char> PUNCT_NOPRESPC_HACK ("^[.,)?!]+$");

const msgmap readMessages(const std::string& file) {
	msgmap msgs;
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	if (result) {
		for (pugi::xml_node error: doc.child("errors").children("error")) {
			for (pugi::xml_node child: error.child("header").children("title")) {
				// child_value assumes we only ever have one PCDATA element here:
				const auto& errtype = utf16conv.from_bytes(error.attribute("id").value());
				const auto& msg = utf16conv.from_bytes(child.child_value());
				const auto& lang = child.attribute("xml:lang").value();
				if(msgs[lang].count(errtype) != 0) {
					std::cerr << "WARNING: Duplicate titles for " << error.attribute("id").value() << std::endl;
				}
				msgs[lang][errtype] = msg;
			}
		}
	}
	else {
		std::cerr << file << ":" << result.offset << " ERROR: " << result.description() << "\n";
	}
	return msgs;
}

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
	catch (HfstException e) {
		std::cerr << "ERROR: HfstException." << std::endl;
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

const std::tuple<bool, std::string, StringVec> get_gentags(const std::string& tags) {
	bool suggest = false;
	std::string errtype;
	StringVec gentags;
	for(auto& tag : split(tags)) {
		std::match_results<const char*> result;
		std::regex_match(tag.c_str(), result, CG_SKIP_TAG);
		if (result.empty()) {
			gentags.push_back(tag);
		}
		else if(result[2].length() != 0) {
			if(tag == CG_SUGGEST_TAG) {
				suggest = true;
			}
			else {
				errtype = result[2];
			}
		}
	}
	return std::make_tuple(suggest, errtype, gentags);
}

const std::tuple<bool, std::string, std::u16string, UStringSet>
get_sugg(const hfst::HfstTransducer& t, const std::string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& lemma_end = line.find("\" ");
	const auto& lemma = line.substr(2, lemma_end-2);
	const auto& tags = line.substr(lemma_end+2);
	const auto& suggen = get_gentags(tags);
	const auto& suggest = std::get<0>(suggen);
	const auto& errtype = utf16conv.from_bytes(std::get<1>(suggen));
	const auto& gentags = std::get<2>(suggen);
	UStringSet forms;
	const auto& tagsplus = join(gentags, "+");
	const auto& ana = lemma+"+"+tagsplus;

	if(!suggest) {
		return std::make_tuple(suggest, ana, errtype, forms);
	}

	const auto& paths = t.lookup_fd({ ana }, -1, 10.0);
	std::stringstream form;
	if(paths->size() > 0) {
		for(auto& p : *paths) {
			for(auto& symbol : p.second) {
				// TODO: this is a hack to avoid flag diacritics; is there a way to make lookup skip them?
				if(symbol.size()>0 && symbol[0]!='@') {
					form << symbol;
				}
			}
			forms.insert(utf16conv.from_bytes(form.str()));
		}
	}
	return std::make_tuple(suggest, ana, errtype, forms);
}

bool wants_prespc(std::string wf, bool blank, bool first_word) {
	std::match_results<const char*> punct_prespc;
	std::regex_match(wf.c_str(), punct_prespc, PUNCT_NOPRESPC_HACK);
	// TODO: should actually check whether we've seen a real
	// blank, but current input format throws away that info, so
	// instead we just have this stupid wordform-check:
	return !first_word && punct_prespc.empty(); // && !blank
}

void run_json(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& msgs)
{
	json::sanity_test();
	int pos = 0;
	std::u16string errtype = u"default";
	bool first_err = true;
	bool first_word = true;
	std::u16string wf;
	std::ostringstream text;
	bool blank = false;
	std::map<std::u16string, UStringSet> cohort_err;

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
			if(!cohort_err.empty()) { // TODO: in a function, since we need to do this at EOF as well
				if(!first_err) {
					os << ",";
				}
				first_err = false;
				// TODO: currently we just pick one if there are several error types:
				auto const& err = cohort_err.begin();
				std::u16string msg = err->first;
				if(msgs.count("se") != 0
				   && msgs.at("se").count(msg) != 0) {
					msg = msgs.at("se").at(msg);
				}
				os << "[" << json::str(wf)
				   << "," << pos-wf.size()
				   << "," << pos
				   << "," << json::str(err->first)
				   << "," << json::str(msg)
				   << "," << json::str_arr(err->second)
				   << "]";
				cohort_err.clear();
			}
			if(wants_prespc(result[2], blank, first_word)) {
				text << " ";
				pos += 1;
			}
			first_word = false;
			blank = false;
			wf = utf16conv.from_bytes(result[2]);
			pos += wf.size();
			text << utf16conv.to_bytes(wf);
			// TODO: wrapper for pos-increasing and text-adding, since they should always happen together
		}
		else if(!result.empty() && result[3].length() != 0) {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			const auto& sugg = get_sugg(t, line);
			if(!std::get<2>(sugg).empty()) {
				errtype = std::get<2>(sugg);
			}
			if(std::get<0>(sugg)) {
				cohort_err[errtype].insert(std::get<3>(sugg).begin(),
							   std::get<3>(sugg).end());

			}
			blank = false;
		}
		else {
			// TODO: remove []superblank and \\'s from superblank?
			// TODO: Uncommented for now since current input format throws away blanks
			// pos += utf16conv.from_bytes(line).size();
			// text << line;
			blank = true;
		}
	}
	os << "]"
	   << "," << json::key(u"text") << json::str(utf16conv.from_bytes(text.str()))
	   << "}";
}

void run_cg(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	// Simple debug function
	std::ostringstream ss;
	for (std::string line; std::getline(is, line);) {
		os << line << std::endl;
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		if(!result.empty() && result[3].length() != 0) {
			const auto& sugg = get_sugg(t, line);
			if(std::get<0>(sugg)) {
				const auto& ana = std::get<1>(sugg);;
				const auto& formv = std::get<3>(sugg);
				if(formv.empty()) {
					os << ana << "\t" << "?" << std::endl;
				}
				else {
					os << ana << "\t" << u16join(formv) << std::endl;
				}
			}
			else {
				const auto& errtype = std::get<2>(sugg);;
				if(!errtype.empty()) {
					os << utf16conv.to_bytes(errtype) << std::endl;
				}
			}
		}
	}
}

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& m, bool json)
{
	if(json) {
		run_json(is, os, t, m);
	}
	else {
		run_cg(is, os, t);
	}
}

}
