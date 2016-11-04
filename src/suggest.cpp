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
	"|@"			// Syntactic tags
	"|Sem/"			// Semantic tags
	"|<"			// E.g. weights <W:0>
	"|R:"			// Relations
	"|ID:"			// Relation ID's
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
				      "|:(.*)" // blank, group 6
				      ")");


const msgmap readMessages(const std::string& file) {
	msgmap msgs;
#ifdef HAVE_LIBPUGIXML
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
#endif
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

const std::tuple<bool, std::string, StringVec> proc_tags(const std::string& tags) {
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

struct Reading {
		bool suggest;
		std::string ana;
		std::u16string errtype;
		UStringSet sforms;
};

const Reading proc_line(const hfst::HfstTransducer& t, const std::string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& lemma_end = line.find("\" ");
	const auto& lemma = line.substr(2, lemma_end-2);
	const auto& tags = line.substr(lemma_end+2);
	const auto& suggen = proc_tags(tags);
	const auto& suggest = std::get<0>(suggen);
	const auto& errtype = utf16conv.from_bytes(std::get<1>(suggen));
	const auto& gentags = std::get<2>(suggen);
	UStringSet sforms;
	const auto& tagsplus = join(gentags, "+");
	const auto& ana = lemma+"+"+tagsplus;

	if(!suggest) {
		return {suggest, ana, errtype, sforms};
	}

	const auto& paths = t.lookup_fd({ ana }, -1, 10.0);
	if(paths->size() > 0) {
		for(auto& p : *paths) {
			std::stringstream form;
			for(auto& symbol : p.second) {
				// TODO: this is a hack to avoid flag diacritics; is there a way to make lookup skip them?
				if(symbol.size()>0 && symbol[0]!='@') {
					form << symbol;
				}
			}
			sforms.insert(utf16conv.from_bytes(form.str()));
		}
	}
	return {suggest, ana, errtype, sforms};
}

/* If we have an inserted suggestion, then the next word has to be
 * part of that, since we don't want to *replace* the word
**/
std::map<std::u16string, UStringSet> sugg_append(std::u16string next_wf, std::map<std::u16string, UStringSet> cohort_err)
{
	std::map<std::u16string, UStringSet> fixed;
	for(auto& err : cohort_err) {
		for(auto& f : err.second) {
			fixed[err.first].insert(f + u" " + next_wf);
		}
	}
	return fixed;
}

struct Cohort {
	std::u16string form;
	std::map<std::u16string, UStringSet> err;
};

bool cohort_empty(const Cohort& c) {
	return c.form.empty();
}

std::string cohort_errs_json(const Cohort& c,
			     const int pos,
			     const hfst::HfstTransducer& t,
			     const msgmap& msgs)
{
	std::string s;
	// TODO: currently we just pick one if there are several error types:
	auto const& err = c.err.begin();
	std::u16string msg = err->first;
	// TODO: locale, how? One process per locale (command-line-arg) or print all messages?
	if(msgs.count("se") != 0
	   && msgs.at("se").count(msg) != 0) {
		msg = msgs.at("se").at(msg);
	}
	else {
		std::cerr << "WARNING: No message for " << json::str(err->first) << std::endl;
	}
	s += "["; s += json::str(c.form);
	s += ","; s += std::to_string(pos);
	s += ","; s += std::to_string(pos+c.form.size());
	s += ","; s += json::str(err->first);
	s += ","; s += json::str(msg);
	s += ","; s += json::str_arr(err->second);
	s += "]";
	return s;
}

void proc_cohort(int& pos,
		 bool& first_err,
		 const Cohort& c,
		 std::ostringstream& text,
		 std::ostream& os,
		 const hfst::HfstTransducer& t,
		 const msgmap& msgs,
		 std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>& utf16conv)
{
	if(cohort_empty(c)) {
		return;
	}
	std::string wfs = utf16conv.to_bytes(c.form);
	if(!c.err.empty()) {
		if(!first_err) {
			os << ",";
		}
		first_err = false;
		os << cohort_errs_json(c, pos, t, msgs);
	}
	pos += c.form.size();
	text << wfs;
	// TODO: wrapper for pos-increasing and text-adding, since they should always happen together
}

/**
 * Remove unescaped [ and ] (superblank delimiters from
 * apertium-deformatter), turn \n into literal newline, unescape all
 * other escaped chars.
 */
const std::string clean_blank(const std::string raw)
{
	bool escaped = false;
	std::ostringstream text;
	for(const auto& c: raw) {
		if(escaped) {
			if(c == 'n') {
				// hfst-tokenize escapes newlines like this; make
				// them literal before jsoning
				text << '\n';
			}
			else {
				// Unescape anything else
				text << c;
			}
			escaped = false;
		}
		// Skip the superblank delimiters
		else if(c == '\\') {
			escaped = true;
		}
		else if(c != '[' && c != ']') {
			text << c;
			escaped = false;
		}
	}
	return text.str();
}

void run_json(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& msgs)
{
	json::sanity_test();
	int pos = 0;
	std::u16string errtype = u"default";
	bool first_err = true;
	LineType prevtype = BlankL;
	bool is_addcohort = true;
	std::ostringstream text;
	Cohort c;

	// TODO: could use http://utfcpp.sourceforge.net, but it's not in macports;
	// and ICU seems overkill just for iterating codepoints
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	os << "{"
	   << json::key(u"errs")
	   << "[";
	for (std::string line; std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		bool appendsugg = is_addcohort && prevtype != WordformL && !c.err.empty();

		if (!result.empty() && ((result[2].length() != 0 && !appendsugg)
					|| result[6].length() != 0)) {
			proc_cohort(pos,
				    first_err,
				    c,
				    text,
				    os,
				    t,
				    msgs,
				    utf16conv);
			c = Cohort();
		}

		if (!result.empty() && result[2].length() != 0) {
			if(appendsugg) {
				c.err = sugg_append(utf16conv.from_bytes(result[2]),
							 c.err);
			}
			is_addcohort = true;
			c.form = utf16conv.from_bytes(result[2]);
			prevtype = WordformL;
		}
		else if(!result.empty() && result[3].length() != 0) {
			// TODO: doesn't do anything with subreadings yet; needs to keep track of previous line(s) for that
			const auto& sugg = proc_line(t, line);
			if(!sugg.errtype.empty()) {
				errtype = sugg.errtype;
			}
			if(sugg.suggest) {
				c.err[errtype].insert(sugg.sforms.begin(),
						      sugg.sforms.end());
			}
			else {
				is_addcohort = false; // Seen at least one non-suggestion reading
			}
			prevtype = ReadingL;
		}
		else if(!result.empty() && result[6].length() != 0) {
			const auto blank = clean_blank(result[6]);
			pos += utf16conv.from_bytes(blank).size();
			text << blank;
			prevtype = BlankL;
		}
		else {
			// Blank lines without the prefix don't go into text output!
			prevtype = BlankL;
		}
	}
	proc_cohort(pos,
		    first_err,
		    c,
		    text,
		    os,
		    t,
		    msgs,
		    utf16conv);
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
			const auto& sugg = proc_line(t, line);
			if(sugg.suggest) {
				const auto& ana = sugg.ana;
				const auto& formv = sugg.sforms;
				if(formv.empty()) {
					os << ana << "\t" << "?" << std::endl;
				}
				else {
					os << ana << "\t" << u16join(formv) << std::endl;
				}
			}
			else {
				const auto& errtype = sugg.errtype;
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
