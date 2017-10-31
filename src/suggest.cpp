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

namespace divvun {

const std::string CG_SUGGEST_TAG = "&SUGGEST";
const std::string CG_SUGGESTWF_TAG = "&SUGGESTWF";

const std::basic_regex<char> DELIMITERS ("^[.!?]$");

const std::basic_regex<char> CG_TAGS_RE ("\"[^\"]*\"|[^ ]+");

// Anything *not* matched by CG_TAG_TYPE is sent to the generator.
// … or we could make an (h)fst out of these to match on lines :)
const std::basic_regex<char> CG_TAG_TYPE (
	"^"
	"(#"			// Group 1: Dependencies, comments
	"|&(.+)"		// Group 2: Errtype
	"|R:(.+):([0-9]+)"	// Group 3 & 4: Relation name and target
	"|ID:([0-9]+)"		// Group 5: Relation ID
	"|\"<(.+)>\""           // Group 6: Reading word-form
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
				      "|(\t+)(\"[^\"]*\"\\S*)(\\s+\\S+)*" // reading, group 3, 4, 5
				      "|:(.*)" // blank, group 6
				      "|(<STREAMCMD:FLUSH>)" // flush, group 7
				      ")");

const std::basic_regex<char> MSG_TEMPLATE_VAR ("^[$][0-9]+$");


struct Reading {
	bool suggest = false;
	std::string ana;
	std::u16string errtype;
	UStringVector sforms;
	relations rels;	// rels[relname] = target.id
	rel_id id = 0;
	std::string wf;
	bool suggestwf = false;
};

struct Cohort {
	std::u16string form;
	std::map<err_id, UStringVector> err;
	size_t pos;
	rel_id id;
	std::vector<Reading> readings;
};

using CohortMap = std::unordered_map<rel_id, size_t>;

enum RunState {
	flushing,
	eof
};

struct Sentence {
	std::vector<Cohort> cohorts;
	CohortMap ids_cohorts;
	std::ostringstream text;
	RunState runstate;
};

enum LineType {
	WordformL, ReadingL, BlankL
};



#ifdef HAVE_LIBPUGIXML
const msgmap readMessagesXml(pugi::xml_document& doc, pugi::xml_parse_result& result)
{
	msgmap msgs;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	if (result) {
		for (pugi::xml_node def: doc.child("errors").child("defaults").children("default")) {
			// std::cerr << "defaults" << std::endl;
			for (pugi::xml_node child: def.child("header").children("title")) {
				const auto& msg = utf16conv.from_bytes(xml_raw_cdata(child));
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
				const auto& msg = utf16conv.from_bytes(xml_raw_cdata(child));
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

const msgmap Suggest::readMessages(const char* buff, const size_t size) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(buff, size);
	return readMessagesXml(doc, result);
#else
	msgmap msgs;
	return msgs;
#endif
}

const msgmap Suggest::readMessages(const std::string& file) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	return readMessagesXml(doc, result);
#else
	msgmap msgs;
	return msgs;
#endif
}



const hfst::HfstTransducer *Suggest::readTransducer(std::istream& is) {
	hfst::HfstInputStream *in = nullptr;
	try
	{
		in = new hfst::HfstInputStream(is);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "ERROR: Stream not readable." << std::endl;
		return nullptr;
	}
	catch (HfstException& e) {
		std::cerr << "ERROR: HfstException." << std::endl;
		return nullptr;
	}

	hfst::HfstTransducer* t = nullptr;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "ERROR: Stream cannot be read." << std::endl;
			return nullptr;
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == nullptr) {
		std::cerr << "WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}

const hfst::HfstTransducer *Suggest::readTransducer(const std::string& file) {
	hfst::HfstInputStream *in = nullptr;
	try
	{
		in = new hfst::HfstInputStream(file);
	}
	catch (StreamNotReadableException& e)
	{
		std::cerr << "ERROR: File does not exist." << std::endl;
		return nullptr;
	}
	catch (HfstException& e) {
		std::cerr << "ERROR: HfstException." << std::endl;
		return nullptr;
	}

	hfst::HfstTransducer* t = nullptr;
	while (not in->is_eof())
	{
		if (in->is_bad())
		{
			std::cerr << "ERROR: Stream cannot be read." << std::endl;
			return nullptr;
		}
		t = new hfst::HfstTransducer(*in);
		if(not in->is_eof()) {
			std::cerr << "WARNING: >1 transducers in stream! Only using the first." << std::endl;
		}
		break;
	}
	in->close();
	delete in;
	if(t == nullptr) {
		std::cerr << "WARNING: Could not read any transducers!" << std::endl;
	}
	return t;
}

// return <suggen, errtype, gentags, id, wf>
// where suggen is true if we want to suggest based on this
// errtype is the error tag (without leading ampersand)
// gentags are the tags we generate with
// id is 0 if unset, otherwise the relation id of this word
const std::tuple<bool, std::string, StringVec, rel_id, relations, std::string, bool> proc_tags(const std::string& tags) {
	bool suggest = false;
	std::string errtype;
	StringVec gentags;
	rel_id id = 0; // CG-3 id's start at 1, should be safe. Want sum types :-/
	relations rels;
	std::string wf;
	bool suggestwf = false;
	for(auto& tag : allmatches(tags, CG_TAGS_RE)) {
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
			else if(tag == CG_SUGGESTWF_TAG) {
				suggestwf = true;
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
		else if(result[6].length() != 0) {
			wf = result[6];
		}
		// else {
		// 	std::cerr << "\033[1;35mresult.length()=\t" << result[0] << "\033[0m" << std::endl;
		// }

	}
	return std::make_tuple(suggest, errtype, gentags, id, rels, wf, suggestwf);
}

const Reading proc_subreading(const std::string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	Reading r;
	const auto& lemma_beg = line.find("\"");
	const auto& lemma_end = line.find("\" ", lemma_beg);
	const auto& lemma = line.substr(lemma_beg + 1, lemma_end-lemma_beg-1);
	const auto& tags = line.substr(lemma_end+2);
	const auto& suggen = proc_tags(tags);
	r.suggest = std::get<0>(suggen);
	r.errtype = utf16conv.from_bytes(std::get<1>(suggen));
	const auto& gentags = std::get<2>(suggen);
	r.id = std::get<3>(suggen);
	r.rels = std::get<4>(suggen);
	const auto& tagsplus = join(gentags, "+");
	r.ana = lemma+"+"+tagsplus;
	r.wf = std::get<5>(suggen);
	r.suggestwf = std::get<6>(suggen);
	if(r.suggestwf) {
		r.sforms.emplace_back(utf16conv.from_bytes(r.wf));
	}
	return r;
};

const Reading proc_reading(const hfst::HfstTransducer& t, const std::string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	std::stringstream ss(line);
	std::string subline;
	std::deque<Reading> subs;
	while(std::getline(ss, subline, '\n')){
		subs.push_front(proc_subreading(subline));
	}
	Reading r;
	const size_t n_subs = subs.size();
	for(size_t i = 0; i < n_subs; ++i) {
		const auto& sub = subs[i];
		r.ana += sub.ana + (i+1 == n_subs ? "" : "#");
		r.errtype += sub.errtype;
		r.rels.insert(sub.rels.begin(), sub.rels.end());
		// higher sub can override id if set; doesn't seem like cg3 puts ids on them though
		r.id = (r.id == 0 ? sub.id : r.id);
		r.suggest = r.suggest || sub.suggest;
		r.suggestwf = r.suggestwf || sub.suggestwf;
		r.sforms.insert(r.sforms.end(), sub.sforms.begin(), sub.sforms.end());
		r.wf = r.wf.empty() ? sub.wf : r.wf;
	}
	if(r.suggest) {
		const auto& paths = t.lookup_fd({ r.ana }, -1, 10.0);
		if(paths->size() > 0) {
			for(auto& p : *paths) {
				std::stringstream form;
				for(auto& symbol : p.second) {
					if(!hfst::FdOperation::is_diacritic(symbol)) {
						form << symbol;
					}
				}
				r.sforms.emplace_back(utf16conv.from_bytes(form.str()));
			}
		}
	}
	return r;
}

/* If we have an inserted suggestion, then the next word has to be
 * part of that, since we don't want to *replace* the word
**/
std::map<std::u16string, UStringVector> sugg_append(const std::u16string& next_wf, std::map<std::u16string, UStringVector> cohort_err)
{
	std::map<std::u16string, UStringVector> fixed;
	for(auto& err : cohort_err) {
		for(auto& f : err.second) {
			fixed[err.first].emplace_back(f + u" " + next_wf);
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

variant<Nothing, Err> cohort_errs(const std::pair<err_id, UStringVector>& err,
				  const Cohort& c,
				  const Sentence& sentence,
				  const hfst::HfstTransducer& t,
				  const msgmap& msgs
				  )
{
	if(cohort_empty(c) || c.err.empty()) {
		return Nothing();
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& errId = err.first;
	std::u16string msg;
	// TODO: locale, how? One process per locale (command-line-arg) or print all messages?
	std::string locale = "se";
	if(msgs.count(locale) == 0) {
		std::cerr << "WARNING: No message at all for " << locale << std::endl;
	}
	else {
		const auto& lmsgs = msgs.at(locale);
		if(lmsgs.first.count(errId) != 0) {
			msg = lmsgs.first.at(errId);
		}
		else {
			for(const auto& p : lmsgs.second) {
				std::match_results<const char*> result;
				const auto& et = utf16conv.to_bytes(errId.c_str());
				std::regex_match(et.c_str(), result, p.first);
				if(!result.empty()
				   && // Only consider full matches:
				   result.position(0) == 0 && result.suffix().length() == 0) {
					msg = p.second;
					break;
					// TODO: cache results? but then no more constness:
					// lmsgs.first.at(errId) = p.second;
				}

			}
		}
		if(msg.empty()) {
			std::cerr << "WARNING: No message for " << json::str(errId) << std::endl;
			msg = errId;
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
				if(sentence.ids_cohorts.find(target_id) == sentence.ids_cohorts.end()) {
					std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
					continue;
				}
				const auto& i_c = sentence.ids_cohorts.at(target_id);
				if(i_c >= sentence.cohorts.size()) {
					std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
					continue;
				}
				const auto& c_trg = sentence.cohorts.at(i_c);
				replaceAll(msg, utf16conv.from_bytes(rel.first.c_str()), c_trg.form);
			}
		}
	}
	return Err {
		c.form,
			c.pos,
			c.pos+c.form.size(),
			errId,
			msg,
			err.second,
			};
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


// https://stackoverflow.com/a/1464684/69663
template<class Iterator>
Iterator Dedupe(Iterator first, Iterator last)
{
	while (first != last)
	{
		Iterator next(first);
		last = std::remove(++next, last, *first);
		first = next;
	}
	return last;
}

Sentence run_sentence(std::istream& is, const hfst::HfstTransducer& t, const msgmap& msgs) {
	size_t pos = 0;
	std::u16string errtype = u"default";
	LineType prevtype = BlankL;
	bool is_addcohort = true;
	Cohort c = DEFAULT_COHORT;
	Sentence sentence;
	sentence.runstate = eof;

	// TODO: could use http://utfcpp.sourceforge.net, but it's not in macports;
	// and ICU seems overkill just for iterating codepoints
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	std::string line;
	std::string readinglines;
	std::getline(is, line);	// TODO: Why do I need at least one getline before os<< after flushing?
	do {
		// std::cerr << "\033[1;34mline:\t" << line << "\033[0m" << std::endl;
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		// TODO: May have to run this block once after the do-block if last line is reading
		if(!readinglines.empty() && (result.empty() || result[3].length() <= 1)) {
			const auto& reading = proc_reading(t, readinglines);
			readinglines = "";
			if(!reading.errtype.empty()) {
				errtype = reading.errtype;
			}
			if(reading.suggest || reading.suggestwf) { // or unconditionally insert? Then don't need suggestwf on Reading
				auto& cerrs = c.err[errtype];
				cerrs.insert(cerrs.end(),
					     reading.sforms.begin(),
					     reading.sforms.end());
				cerrs.erase(Dedupe(cerrs.begin(), cerrs.end()),
					    cerrs.end());
			}
			else {
				is_addcohort = false; // Seen at least one non-suggestion reading
			}
			if(reading.id != 0) {
				c.id = reading.id;
			}
			c.readings.push_back(reading);
		}
		bool appendsugg = is_addcohort && prevtype != WordformL && !c.err.empty();

		if (!result.empty() && ((result[2].length() != 0 && !appendsugg) // wordform
					|| result[6].length() != 0)) { // blank
			c.pos = pos;
			if(!cohort_empty(c)) {
				sentence.cohorts.push_back(c);
				if(c.id != 0) {
					sentence.ids_cohorts[c.id] = sentence.cohorts.size() - 1;
				}
			}
			// TODO: wrapper for pos-increasing and text-adding, since they should always happen together
			pos += c.form.size();
			sentence.text << utf16conv.to_bytes(c.form);

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
			readinglines += line + "\n";
			prevtype = ReadingL;
		}
		else if(!result.empty() && result[6].length() != 0) { // blank
			const auto blank = clean_blank(result[6]);
			pos += utf16conv.from_bytes(blank).size();
			sentence.text << blank;
			prevtype = BlankL;
		}
		else if(!result.empty() && result[7].length() != 0) { // flush
			sentence.runstate = flushing;
			break;
		}
		else {
			// Blank lines without the prefix don't go into text output!
			prevtype = BlankL;
		}
	} while(std::getline(is, line));

	if(!readinglines.empty()) {
		const auto& reading = proc_reading(t, readinglines);
		readinglines = "";
		if(!reading.errtype.empty()) {
			errtype = reading.errtype;
		}
		if(reading.suggest) {
			auto& cerrs = c.err[errtype];
			cerrs.insert(cerrs.end(),
				     reading.sforms.begin(),
				     reading.sforms.end());
			cerrs.erase(Dedupe(cerrs.begin(), cerrs.end()),
				    cerrs.end());
		}
		if(reading.id != 0) {
			c.id = reading.id;
		}
		c.readings.push_back(reading);
	}

	c.pos = pos;
	sentence.cohorts.push_back(c);
	sentence.text << utf16conv.to_bytes(c.form);
	return sentence;
}

std::vector<Err> Suggest::run_errs(std::istream& is)
{
	Sentence sentence = run_sentence(is, *generator, msgs);

	std::vector<Err> errs;
	for(const auto& c : sentence.cohorts) {
		pickErr(c.err, ignores).match(
			[]      (Nothing) {},
			[&errs, &c, &sentence, this] (std::pair<err_id, UStringVector>& err)
			{
				cohort_errs(err, c, sentence, *generator, msgs).match(
					[]      (Nothing) {},
					[&errs] (Err e)   { errs.push_back(e); });
			});
	}
	return errs;
}


RunState run_json(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t, const msgmap& msgs, const std::set<err_id>& ignores)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	json::sanity_test();
	Sentence sentence = run_sentence(is, t, msgs);

	// All processing done, output:
	os << "{"
	   << json::key(u"errs")
	   << "[";
	bool wantsep = false;
	for(const auto& c : sentence.cohorts) {
		pickErr(c.err, ignores).match(
			[]      (Nothing) {},
			[&os, &wantsep, &c, &sentence, &t, &msgs] (std::pair<err_id, UStringVector>& err)
			{
				cohort_errs(err, c, sentence, t, msgs).match(
					[] (Nothing) {},
					[&os, &wantsep] (Err& e)   {
						if(wantsep) {
							os << ",";
						}
						os << "[" << json::str(e.form)
						   << "," << std::to_string(e.beg)
						   << "," << std::to_string(e.end)
						   << "," << json::str(e.err)
						   << "," << json::str(e.msg)
						   << "," << json::str_arr(e.rep)
						   << "]";
						wantsep = true;
					});
			});
	}
	os << "]"
	   << "," << json::key(u"text") << json::str(utf16conv.from_bytes(sentence.text.str()))
	   << "}";
	if(sentence.runstate == flushing) {
		os << '\0';
		os.flush();
		os.clear();
	}
	return sentence.runstate;
}


void print_cg_reading(const std::string& readinglines, std::ostream& os, const hfst::HfstTransducer& t, std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>& utf16conv) {
	os << readinglines;
	const auto& reading = proc_reading(t, readinglines);
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

void run_cg(std::istream& is, std::ostream& os, const hfst::HfstTransducer& t)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	// Simple debug function; only subreading state kept between lines
	std::string readinglines;
	for (std::string line;std::getline(is, line);) {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if(!readinglines.empty() && (result.empty() || result[3].length() <= 1)) {
			print_cg_reading(readinglines, os, t, utf16conv);
			readinglines = "";
		}

		if(!result.empty() && result[3].length() != 0) {
			readinglines += line + "\n";
		}
		else if(!result.empty() && result[7].length() != 0) {
			// TODO: Can we ever get a flush in the middle of readings?
			os.flush();
			os << line << std::endl;
		}
		else {
			os << line << std::endl;
		}
	}
	if(!readinglines.empty()) {
		print_cg_reading(readinglines, os, t, utf16conv);
	}
}

void Suggest::run(std::istream& is, std::ostream& os, bool json)
{
	if(json) {
		while(run_json(is, os, *generator, msgs, ignores) == flushing);
	}
	else {
		run_cg(is, os, *generator); // ignores ignores
	}
}

Suggest::Suggest (const hfst::HfstTransducer* generator_, divvun::msgmap msgs_, bool verbose)
	: msgs(msgs_)
	, generator(generator_)
{
}
Suggest::Suggest (const std::string& gen_path, const std::string& msg_path, bool verbose)
	: msgs(readMessages(msg_path))
	, generator(readTransducer(gen_path))
{
}

void Suggest::setIgnores(const std::set<err_id>& ignores_)
{
	ignores = ignores_;
}

}
