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

#include "suggest.hpp"

namespace gtd {

const std::string CG_SUGGEST_TAG = "&SUGGEST";

const std::basic_regex<char> DELIMITERS ("^[.!?]$");

// Anything *not* matched by CG_TAG_TYPE is sent to the generator.
// … or we could make an (h)fst out of these to match on lines :)
const std::basic_regex<char> CG_TAG_TYPE (
	"^"
	"(#"			// Dependencies, comments
	"|&(.+)"		// Group 2: Errtype
	"|R:(.+):([0-9]+)"	// Group 3 & 4: Relation name and target
	"|ID:([0-9]+)"		// Group 5: Relation ID
	"|@"			// Syntactic tag
	"|Sem/"			// Semantic tag
	"|§"			// Semantic role
	"|<"			// Weights (<W:0>) and such
	"|ADD:"			// --trace
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
				      "|(<STREAMCMD:FLUSH>)" // flush, group 7
				      ")");

const std::basic_regex<char> MSG_TEMPLATE_VAR ("^[$][0-9]+$");

#ifdef HAVE_LIBPUGIXML
const msgmap readMessages(pugi::xml_document& doc, pugi::xml_parse_result& result)
{
	msgmap msgs;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	if (result) {
		for (pugi::xml_node def: doc.child("errors").child("defaults").children("default")) {
			// std::cerr << "defaults" << std::endl;
			for (pugi::xml_node child: def.child("header").children("title")) {
				std::ostringstream os;
				for(const auto& cc: child.children())
				{
					cc.print(os, "", pugi::format_raw);
				}
				const auto& msg = utf16conv.from_bytes(os.str());
				const auto& lang = child.attribute("xml:lang").value();
				for (pugi::xml_node e: def.child("ids").children("e")) {
					// e_value assumes we only ever have one PCDATA element here:
					const auto& errtype = utf16conv.from_bytes(e.attribute("id").value());
					// std::cerr << utf16conv.to_bytes(errtype) << std::endl;
					if(msgs[lang].first.count(errtype) != 0) {
						std::cerr << "WARNING: Duplicate titles for " << e.attribute("id").value() << std::endl;
					}
					msgs[lang].first[errtype] = msg;
				}
				for (pugi::xml_node re: def.child("ids").children("re")) {
					std::basic_regex<char> r(re.attribute("v").value());
					msgs[lang].second.push_back(std::make_pair(r, msg));
				}
			}
		}
		for (pugi::xml_node error: doc.child("errors").children("error")) {
			for (pugi::xml_node child: error.child("header").children("title")) {
				// child_value assumes we only ever have one PCDATA element here:
				const auto& errtype = utf16conv.from_bytes(error.attribute("id").value());
				std::ostringstream os;
				for(const auto& cc: child.children())
				{
					cc.print(os, "", pugi::format_raw);
				}
				const auto& msg = utf16conv.from_bytes(os.str());
				const auto& lang = child.attribute("xml:lang").value();
				if(msgs[lang].first.count(errtype) != 0) {
					std::cerr << "WARNING: Duplicate titles for " << error.attribute("id").value() << std::endl;
				}
				msgs[lang].first[errtype] = msg;
			}
		}
	}
	else {
		std::cerr << "(buffer):" << result.offset << " ERROR: " << result.description() << "\n";
	}
	return msgs;
}
#endif

const msgmap readMessages(const char* buff, const size_t size) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(buff, size);
	return readMessages(doc, result);
#else
	msgmap msgs;
	return msgs;
#endif
}

const msgmap readMessages(const std::string& file) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	return readMessages(doc, result);
#else
	msgmap msgs;
	return msgs;
#endif
}



const hfst::HfstTransducer *readTransducer(std::istream& is) {
	hfst::HfstInputStream *in = NULL;
	try
	{
		in = new hfst::HfstInputStream(is);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "ERROR: Stream not readable." << std::endl;
		return NULL;
	}
	catch (HfstException& e) {
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

const hfst::HfstTransducer *readTransducer(const std::string& file) {
	hfst::HfstInputStream *in = NULL;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "ERROR: File does not exist." << std::endl;
		return NULL;
	}
	catch (HfstException& e) {
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

// return <suggen, errtype, gentags, id>
// where suggen is true if we want to suggest based on this
// errtype is the error tag (without leading ampersand)
// gentags are the tags we generate with
// id is 0 if unset, otherwise the relation id of this word
const std::tuple<bool, std::string, StringVec, rel_id, relations> proc_tags(const std::string& tags) {
	bool suggest = false;
	std::string errtype;
	StringVec gentags;
	rel_id id = 0; // CG-3 id's start at 1, should be safe. Want sum types :-/
	relations rels;
	for(auto& tag : split(tags)) {
		std::match_results<const char*> result;
		std::regex_match(tag.c_str(), result, CG_TAG_TYPE);
		if (result.empty()) {
			gentags.push_back(tag);
			// std::cerr << "\033[1;35mgentag=\t" << tag << "\033[0m" << std::endl;
		}
		else if(result[2].length() != 0) {
			// std::cerr << "\033[1;35msugtag=\t" << result[2] << "\033[0m" << std::endl;
			if(tag == CG_SUGGEST_TAG) {
				suggest = true;
			}
			else {
				errtype = result[2];
			}
		}
		else if(result[3].length() != 0 && result[4].length() != 0) {
			// std::cerr << "\033[1;35mresult[3] (name)=\t" << result[3] << "\033[0m" << std::endl;
			// std::cerr << "\033[1;35mresult[4] (ID)=\t" << result[4] << "\033[0m" << std::endl;
			try {
				rel_id target = stoi(result[4]);
				auto rel_name = result[3];
				rels[rel_name] = target;
			}
			catch(...) {
				std::cerr << "WARNING: Couldn't parse relation target integer" << std::endl;
			}
		}
		else if(result[5].length() != 0) {
			try {
				id = stoi(result[5]);
			}
			catch(...) {
				std::cerr << "WARNING: Couldn't parse ID integer" << std::endl;
			}
			// std::cerr << "\033[1;35mresult[5] (ID)=\t" << result[5] << "\033[0m" << std::endl;
		}
		// else {
		// 	std::cerr << "\033[1;35mresult.length()=\t" << result[0] << "\033[0m" << std::endl;
		// }

	}
	return std::make_tuple(suggest, errtype, gentags, id, rels);
}

const Reading proc_reading(const hfst::HfstTransducer& t, const std::string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& lemma_end = line.find("\" ");
	const auto& lemma = line.substr(2, lemma_end-2);
	const auto& tags = line.substr(lemma_end+2);
	const auto& suggen = proc_tags(tags);
	const auto& suggest = std::get<0>(suggen);
	const auto& errtype = utf16conv.from_bytes(std::get<1>(suggen));
	const auto& gentags = std::get<2>(suggen);
	const auto& id = std::get<3>(suggen);
	const auto& rels = std::get<4>(suggen);
	UStringSet sforms;
	const auto& tagsplus = join(gentags, "+");
	const auto& ana = lemma+"+"+tagsplus;

	if(suggest) {
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
	}
	return {suggest, ana, errtype, sforms, rels, id};
}

/* If we have an inserted suggestion, then the next word has to be
 * part of that, since we don't want to *replace* the word
**/
std::map<std::u16string, UStringSet> sugg_append(const std::u16string& next_wf, std::map<std::u16string, UStringSet> cohort_err)
{
	std::map<std::u16string, UStringSet> fixed;
	for(auto& err : cohort_err) {
		for(auto& f : err.second) {
			fixed[err.first].insert(f + u" " + next_wf);
		}
	}
	return fixed;
}

bool cohort_empty(const Cohort& c) {
	return c.form.empty();
}

const Cohort DEFAULT_COHORT = {
	{}, {}, 0, 0, {}
};

std::string cohort_errs_json(const Cohort& c,
			     const CohortMap& ids_cohorts,
			     const std::vector<Cohort>& sentence,
			     const hfst::HfstTransducer& t,
			     const msgmap& msgs)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::string s;
	// TODO: currently we just pick one if there are several error types:
	const auto& err = c.err.begin();
	const auto& errtype = err->first;
	std::u16string msg;
	// TODO: locale, how? One process per locale (command-line-arg) or print all messages?
	std::string locale = "se";
	if(msgs.count(locale) == 0) {
		std::cerr << "WARNING: No message at all for " << locale << std::endl;
	}
	else {
		const auto& lmsgs = msgs.at(locale);
		if(lmsgs.first.count(errtype) != 0) {
			msg = lmsgs.first.at(errtype);
		}
		else {
			for(const auto& p : lmsgs.second) {
				std::match_results<const char*> result;
				const auto& et = utf16conv.to_bytes(errtype.c_str());
				std::regex_match(et.c_str(), result, p.first);
				if(!result.empty()
				   && // Only consider full matches:
				   result.position(0) == 0 && result.suffix().length() == 0) {
					msg = p.second;
					break;
					// TODO: cache results? but then no more constness:
					// lmsgs.first.at(errtype) = p.second;
				}

			}
		}
		if(msg.empty()) {
			std::cerr << "WARNING: No message for " << json::str(err->first) << std::endl;
			msg = errtype;
		}
		// TODO: Make suitable structure on creating msgmap instead?
		replaceAll(msg, u"$1", c.form);
		for(const auto& r: c.readings) {
			for(const auto& rel: r.rels) {
				if(rel.first == "$1") {
					std::cerr << "WARNING: $1 relation overwritten by CG rule" << std::endl;
				}
				std::match_results<const char*> result;
				std::regex_match(rel.first.c_str(), result, MSG_TEMPLATE_VAR);
				if(result.empty()) { // Other relation
					continue;
				}
				// TODO: Should check here that rel_name matches /[$][0-9]+/
				const auto& target_id = rel.second;
				if(ids_cohorts.find(target_id) == ids_cohorts.end()) {
					std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
					continue;
				}
				const auto& i_c = ids_cohorts.at(target_id);
				if(i_c >= sentence.size()) {
					std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
					continue;
				}
				const auto& c_trg = sentence.at(i_c);
				replaceAll(msg, utf16conv.from_bytes(rel.first.c_str()), c_trg.form);
			}
		}
	}
	s += "["; s += json::str(c.form);
	s += ","; s += std::to_string(c.pos);
	s += ","; s += std::to_string(c.pos+c.form.size());
	s += ","; s += json::str(err->first);
	s += ","; s += json::str(msg);
	s += ","; s += json::str_arr(err->second);
	s += "]";
	return s;
}

void proc_cohort_json(bool& first_err,
		      const Cohort& c,
		      std::ostream& os,
		      const CohortMap& ids_cohorts,
		      const std::vector<Cohort>& sentence,
		      const hfst::HfstTransducer& t,
		      const msgmap& msgs)
{
	if(cohort_empty(c)) {
		return;
	}
	if(!c.err.empty()) {
		if(!first_err) {
			os << ",";
		}
		first_err = false;
		os << cohort_errs_json(c, ids_cohorts, sentence, t, msgs);
	}
	// TODO: wrapper for pos-increasing and text-adding, since they should always happen together
}

/**
 * Remove unescaped [ and ] (superblank delimiters from
 * apertium-deformatter), turn \n into literal newline, unescape all
 * other escaped chars.
 */
const std::string clean_blank(const std::string& raw)
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

enum RunState {
    flushing,
    eof
};

RunState run_json(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& msgs)
{
	json::sanity_test();
	int pos = 0;
	std::u16string errtype = u"default";
	bool first_err = true;
	LineType prevtype = BlankL;
	bool is_addcohort = true;
	std::ostringstream text;
	Cohort c = DEFAULT_COHORT;
	RunState runstate = eof;
	std::vector<Cohort> sentence;
	CohortMap ids_cohorts;

	// TODO: could use http://utfcpp.sourceforge.net, but it's not in macports;
	// and ICU seems overkill just for iterating codepoints
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	std::string line;
	std::getline(is, line);	// TODO: Why do I need at least one getline before os<< after flushing?
	os << "{"
	   << json::key(u"errs")
	   << "[";
	do {
		// std::cerr << "\033[1;34mline:\t" << line << "\033[0m" << std::endl;
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		bool appendsugg = is_addcohort && prevtype != WordformL && !c.err.empty();

		if (!result.empty() && ((result[2].length() != 0 && !appendsugg) // wordform
					|| result[6].length() != 0)) { // blank
			c.pos = pos;
			if(!cohort_empty(c)) {
				sentence.push_back(c);
				if(c.id != 0) {

					ids_cohorts[c.id] = sentence.size() - 1;
				}
			}
			pos += c.form.size();
			text << utf16conv.to_bytes(c.form);

			c = DEFAULT_COHORT;
		}

		if (!result.empty() && result[2].length() != 0) { // wordform
			if(appendsugg) {
				c.err = sugg_append(utf16conv.from_bytes(result[2]),
						    c.err);
			}
			is_addcohort = true;
			c.form = utf16conv.from_bytes(result[2]);
			prevtype = WordformL;
		}
		else if(!result.empty() && result[3].length() != 0) { // reading
			// TODO: doesn't do anything with subreadings yet
			const auto& reading = proc_reading(t, line);

			if(!reading.errtype.empty()) {
				errtype = reading.errtype;
			}
			if(reading.suggest) {
				c.err[errtype].insert(reading.sforms.begin(),
						      reading.sforms.end());
			}
			else {
				is_addcohort = false; // Seen at least one non-suggestion reading
			}
			if(reading.id != 0) {
				c.id = reading.id;
			}
			c.readings.push_back(reading);
			prevtype = ReadingL;
		}
		else if(!result.empty() && result[6].length() != 0) { // blank
			const auto blank = clean_blank(result[6]);
			pos += utf16conv.from_bytes(blank).size();
			text << blank;
			prevtype = BlankL;
		}
		else if(!result.empty() && result[7].length() != 0) { // flush
			runstate = flushing;
			break;
		}
		else {
			// Blank lines without the prefix don't go into text output!
			prevtype = BlankL;
		}
	} while(std::getline(is, line));
	c.pos = pos;
	sentence.push_back(c);
	pos += c.form.size();
	text << utf16conv.to_bytes(c.form);
	for(const auto& c : sentence) {
		proc_cohort_json(first_err,
				 c,
				 os,
				 ids_cohorts,
				 sentence,
				 t,
				 msgs);
	}
	os << "]"
	   << "," << json::key(u"text") << json::str(utf16conv.from_bytes(text.str()))
	   << "}";
	if(runstate == flushing) {
		os << '\0';
		os.flush();
		os.clear();
	}
	return runstate;
}

void run_cg(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	// Simple debug function; no state kept between lines
	for (std::string line; std::getline(is, line);) {
		os << line << std::endl;
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		if(!result.empty() && result[3].length() != 0) {
			const auto& reading = proc_reading(t, line);
			if(reading.suggest) {
				const auto& ana = reading.ana;
				const auto& formv = reading.sforms;
				if(formv.empty()) {
					os << ana << "\t" << "?" << std::endl;
				}
				else {
					os << ana << "\t" << u16join(formv) << std::endl;
				}
			}
			else {
				const auto& errtype = reading.errtype;
				if(!errtype.empty()) {
					os << utf16conv.to_bytes(errtype) << std::endl;
				}
			}
		}
		else if(!result.empty() && result[7].length() != 0) {
			os.flush();
		}
	}
}

void run(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& m, bool json)
{
	if(json) {
		while(run_json(is, os, t, m) == flushing);
	}
	else {
		run_cg(is, os, t);
	}
}

}
