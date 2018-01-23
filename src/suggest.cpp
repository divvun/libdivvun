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

#include "suggest.hpp"

namespace divvun {

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
	"|ADDRELATION[:\(]"
	"|SETRELATION[:\(]"
	"|REMRELATION[:\(]"
	"|ADDRELATIONS[:\(]"
	"|SETRELATIONS[:\(]"
	"|REMRELATIONS[:\(]"
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

const std::basic_regex<char> MSG_TEMPLATE_REL ("^[$][0-9]+$");
const std::basic_regex<char> LEFT_RIGHT_REL ("^(LEFT|RIGHT)$"); // cohort added to the left/right
const std::basic_regex<char> DELETE_REL ("^DELETE[0-9]+");

enum LineType {
	WordformL, ReadingL, BlankL
};



#ifdef HAVE_LIBPUGIXML
const MsgMap readMessagesXml(pugi::xml_document& doc, pugi::xml_parse_result& result)
{
	MsgMap msgs;
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

const MsgMap Suggest::readMessages(const char* buff, const size_t size) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(buff, size);
	return readMessagesXml(doc, result);
#else
	MsgMap msgs;
	return msgs;
#endif
}

const MsgMap Suggest::readMessages(const string& file) {
#ifdef HAVE_LIBPUGIXML
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	return readMessagesXml(doc, result);
#else
	MsgMap msgs;
	return msgs;
#endif
}


const Reading proc_subreading(const string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	Reading r;
	const auto& lemma_beg = line.find("\"");
	const auto& lemma_end = line.find("\" ", lemma_beg);
	const auto& lemma = line.substr(lemma_beg + 1, lemma_end-lemma_beg-1);
	const auto& tags = line.substr(lemma_end+2);
	StringVec gentags;	// tags we generate with
	r.id = 0; // CG-3 id's start at 1, should be safe. Want sum types :-/
	r.wf = "";
	r.suggest = false;
	r.suggestwf = false;
	r.added = NotAdded;
	r.link = false;
	for(auto& tag : allmatches(tags, CG_TAGS_RE)) { // proc_tags
		std::match_results<const char*> result;
		std::regex_match(tag.c_str(), result, CG_TAG_TYPE);
		if (result.empty()) {
			gentags.push_back(tag);
		}
		else if(result[2].length() != 0) {
			if(tag == "&SUGGEST") {
				r.suggest = true;
			}
			else if(tag == "&SUGGESTWF") {
				r.suggestwf = true;
			}
			else if(tag == "&ADDED" || tag == "&ADDED-AFTER-BLANK") {
				r.added = AddedAfterBlank;
			}
			else if(tag == "&ADDED-BEFORE-BLANK") {
				r.added = AddedBeforeBlank;
			}
			else if(tag == "&LINK") {
				r.link = true;
			}
			else {
				r.errtype = utf16conv.from_bytes(result[2]);
			}
		}
		else if(result[3].length() != 0 && result[4].length() != 0) {
			try {
				rel_id target = stoi(result[4]);
				auto rel_name = result[3];
				r.rels[rel_name] = target;
			}
			catch(...) {
				std::cerr << "WARNING: Couldn't parse relation target integer" << std::endl;
			}
		}
		else if(result[5].length() != 0) {
			try {
				r.id = stoi(result[5]);
			}
			catch(...) {
				std::cerr << "WARNING: Couldn't parse ID integer" << std::endl;
			}
		}
		else if(result[6].length() != 0) {
			r.wf = result[6];
		}

	}
	const auto& tagsplus = join(gentags, "+");
	r.ana = lemma+"+"+tagsplus;
	if(r.suggestwf) {
		r.sforms.emplace_back(utf16conv.from_bytes(r.wf));
	}
	return r;
};

const Reading proc_reading(const hfst::HfstTransducer& t, const string& line) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	stringstream ss(line);
	string subline;
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
		r.link = r.link || sub.link;
		r.added = r.added == NotAdded ? sub.added : r.added;
		r.sforms.insert(r.sforms.end(), sub.sforms.begin(), sub.sforms.end());
		r.wf = r.wf.empty() ? sub.wf : r.wf;
	}
	if(r.suggest) {
		const auto& paths = t.lookup_fd({ r.ana }, -1, 10.0);
		for(auto& p : *paths) {
			stringstream form;
			for(auto& symbol : p.second) {
				if(!hfst::FdOperation::is_diacritic(symbol)) {
					form << symbol;
				}
			}
			r.sforms.emplace_back(utf16conv.from_bytes(form.str()));
		}
	}
	return r;
}

bool cohort_empty(const Cohort& c) {
	return c.form.empty();
}

const Cohort DEFAULT_COHORT = {
	{}, 0, 0, {}, {}, NotAdded
};

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

void rel_on_match(const relations& rels,
		  const std::basic_regex<char>& name,
		  const Sentence& sentence,
		  const std::function<void(const string& relname, size_t i_t, const Cohort& trg)>& fn) {
	for(const auto& rel: rels) {
		std::match_results<const char*> result;
		std::regex_match(rel.first.c_str(), result, name);
		if(result.empty()) { // Other relation
			continue;
		}
		const auto& target_id = rel.second;
		if(sentence.ids_cohorts.find(target_id) == sentence.ids_cohorts.end()) {
			std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
			continue;
		}
		const auto& i_t = sentence.ids_cohorts.at(target_id);
		if(i_t >= sentence.cohorts.size()) {
			std::cerr << "WARNING: Couldn't find relation target for " << rel.first << ":" << rel.second << std::endl;
			continue;
		}
		fn(rel.first,
		   i_t,
		   sentence.cohorts.at(i_t));
	}
}

/*
 * Return possibly altered beg/end indices for the Err coverage
 * (underline), along with a replacement suggestion (or Nothing() if
 * given bad data).
 */
variant<Nothing, pair<pair<size_t, size_t>, u16string>>
proc_LEFT_RIGHT(const ErrId& err_id,
		const size_t src_id,
		const Sentence& sentence,
		const u16string& text,
		size_t beg, // mut: We may return an altered version of beg
		size_t end, // mut: We may return an altered version of end
		const string& relname,
		const size_t i_t,
		const Cohort& trg) {
	if(sentence.ids_cohorts.find(src_id) == sentence.ids_cohorts.end()) {
		std::cerr << "WARNING: Saw &LEFT/&RIGHT on cohort with id 0" << std::endl;
		return Nothing();
	}
	const auto& i_c = sentence.ids_cohorts.at(src_id);
	if(i_c >= sentence.cohorts.size()) {
		std::cerr << "WARNING: Internal error (unexpected i_c" << i_c << " >= sentence.cohorts.size())" << std::endl;
		return Nothing();
	}
	std::map<size_t, u16string> add; // position in text:cohort id in Sentence
	// Loop from the leftmost to the rightmost of source and target cohorts:
	size_t left  = i_c < i_t ? i_c + 1 : i_t;
	size_t right = i_c < i_t ? i_t + 1 : i_c;
	for(size_t i = left; i < right; ++i) {
		const auto& trg = sentence.cohorts[i];
		for(const auto& tr: trg.readings) {
			if(tr.added == NotAdded || tr.errtype != err_id) {
				continue;
			}
			size_t addstart = trg.pos;
			if(tr.added == AddedBeforeBlank) {
				if(i == 0) {
					std::cerr << "WARNING: Saw &ADDED-BEFORE-BLANK on initial word, ignoring" << std::endl;
					continue;
				}
				const auto& pretrg = sentence.cohorts[i-1];
				addstart = pretrg.pos + pretrg.form.size();
			}
			beg = std::min(beg, addstart);
			end = std::max(end, addstart + trg.form.size());
			add[addstart] = trg.form;
		}
	}
	u16string repform = text.substr(beg, end - beg); // mut
	for(auto it = add.rbegin(); it != add.rend(); ++it) {
		size_t at = it->first - beg;
		if(at >= repform.size()) {
			std::cerr << "WARNING: Internal error (trying to splice into pos " << at << " of repform)" << std::endl;
			continue;
		}
		repform = repform.substr(0, at) + it->second + repform.substr(at);
	}
	return std::make_pair(std::make_pair(beg, end), repform);
}

variant<Nothing, Err> Suggest::cohort_errs(const ErrId& err_id,
					   const Cohort& c,
					   const Sentence& sentence,
					   const u16string& text)
{
	if(cohort_empty(c) || c.added || ignores.find(err_id) != ignores.end()) {
		return Nothing();
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	u16string msg;
	for(const auto& mlang : sortedmsglangs) {
		if(msg.empty() && mlang != locale) {
			std::cerr << "WARNING: No message for " << json::str(err_id) << " in xml:lang '" << locale << "', trying '" << mlang << "'" << std::endl;
		}
		const auto& lmsgs = msgs.at(mlang);
		if(lmsgs.first.count(err_id) != 0) {
			msg = lmsgs.first.at(err_id);
		}
		else {
			for(const auto& p : lmsgs.second) {
				std::match_results<const char*> result;
				const auto& et = utf16conv.to_bytes(err_id.c_str());
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
		if(!msg.empty()) {
			break;
		}
	}
	if(msg.empty()) {
		std::cerr << "WARNING: No message for " << json::str(err_id) << " in any xml:lang" << std::endl;
		msg = err_id;
	}
	// TODO: Make suitable structure on creating MsgMap instead?
	replaceAll(msg, u"$1", c.form);
	for(const auto& r: c.readings) {
		if((!r.errtype.empty()) && err_id != r.errtype) {
			continue;
		}
		rel_on_match(r.rels, MSG_TEMPLATE_REL, sentence,
			     [&] (const string& relname, size_t i_t, const Cohort& trg) {
				     replaceAll(msg, utf16conv.from_bytes(relname.c_str()), trg.form);
			     });
	}
	auto beg = c.pos;
	auto end = c.pos + c.form.size();
	UStringVector rep;
	for(const auto& r: c.readings) {
		if((!r.errtype.empty()) && err_id != r.errtype) {
			continue;
		}
		rep.insert(rep.end(),
			   r.sforms.begin(),
			   r.sforms.end());
		// If there are LEFT/RIGHT added relations, add suggestions with those concatenated to our form
		// TODO: What about our current suggestions of the same error tag? Currently just using wordform
		rel_on_match(r.rels, LEFT_RIGHT_REL, sentence,
			     [&] (const string& relname, size_t i_t, const Cohort& trg) {
				     proc_LEFT_RIGHT(err_id, c.id, sentence, text, beg, end, relname, i_t, trg).match(
					     []  (Nothing) {},
					     [&] (pair<pair<size_t, size_t>, u16string> p)   {
						     beg = p.first.first;
						     end = p.first.second;
						     rep.push_back(p.second);
					     });
			     });
		std::map<size_t, size_t> deleted;
		rel_on_match(r.rels, DELETE_REL, sentence,
			     [&] (const string& relname, size_t i_t, const Cohort& trg) {
				     if(c.errtypes.size() > 1) {
					     // Only treat delete relations into targets that have a reading with the same error type
					     // (Unfortunately, CG3 can't do relations from reading to reading, but requiring the same error type lets us work around that)
					     auto found = std::find_if(trg.readings.begin(), trg.readings.end(),
								       [&](const Reading& r) -> bool { return r.errtype == err_id; });
					     if(found == trg.readings.end()) {
						     return;
					     }
				     }
				     size_t del_beg = trg.pos;
				     size_t del_end = del_beg + trg.form.size();
				     // Expand (unless we've already expanded more in that direction):
				     if(trg.pos < beg) {
					     beg = trg.pos;
				     }
				     else if(del_end > end) {
					     end = del_end;
				     }
				     if(trg.pos < c.pos && text.substr(del_end, 1) == u" ") { // trim right if left
					     ++del_end;
				     }
				     if(trg.pos > c.pos && text.substr(del_beg - 1, 1) == u" ") { // trim left if right
					     --del_beg;
				     }
				     deleted[del_beg] = del_end - del_beg;
			     });
		if(!deleted.empty()) {
			auto repform = text.substr(beg, end - beg);
			// go from end of ordered map so we can chop off without indices changing:
			for(auto it = deleted.rbegin(); it != deleted.rend(); ++it) {
				auto del_beg = it->first - beg;
				auto del_len = it->second;
				repform = repform.erase(del_beg, del_len);
			}
			rep.push_back(repform);
		}
	}
	auto form = text.substr(beg, end - beg);
	rep.erase(std::remove_if(rep.begin(),
				 rep.end(),
				 [&](const u16string& r) { return r == form; }),
		  rep.end());
	rep.erase(Dedupe(rep.begin(), rep.end()),
		  rep.end());
	return Err {
		form,
		beg,
		end,
		err_id,
		msg,
		rep
	};
}

/**
 * Remove unescaped [ and ] (superblank delimiters from
 * apertium-deformatter), turn \n into literal newline, unescape all
 * other escaped chars.
 */
const string clean_blank(const string& raw)
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


Sentence run_sentence(std::istream& is, const hfst::HfstTransducer& t, const MsgMap& msgs) {
	size_t pos = 0;
	Cohort c = DEFAULT_COHORT;
	Sentence sentence;
	sentence.runstate = eof;

	// TODO: could use http://utfcpp.sourceforge.net, but it's not in macports;
	// and ICU seems overkill just for iterating codepoints
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	string line;
	string readinglines;
	std::getline(is, line);	// TODO: Why do I need at least one getline before os<< after flushing?
	do {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if(!readinglines.empty() && (result.empty() || result[3].length() <= 1)) {
			const auto& reading = proc_reading(t, readinglines);
			readinglines = "";
			if(!reading.errtype.empty()) {
				c.errtypes.insert(reading.errtype);
			}
			if(reading.id != 0) {
				c.id = reading.id;
			}
			c.added = reading.added == NotAdded ? c.added : reading.added;
			c.readings.push_back(reading);
		}

		if (!result.empty() && ((result[2].length() != 0) // wordform or blank: reset Cohort
					|| result[6].length() != 0)) {
			c.pos = pos;
			if(!cohort_empty(c)) {
				sentence.cohorts.push_back(c);
				if(c.id != 0) {
					sentence.ids_cohorts[c.id] = sentence.cohorts.size() - 1;
				}
			}
			if(!c.added) {
				pos += c.form.size();
				sentence.text << utf16conv.to_bytes(c.form);
			}
			c = DEFAULT_COHORT;
		}

		if (!result.empty() && result[2].length() != 0) { // wordform
			c.form = utf16conv.from_bytes(result[2]);
		}
		else if(!result.empty() && result[3].length() != 0) { // reading
			readinglines += line + "\n";
		}
		else if(!result.empty() && result[6].length() != 0) { // blank
			const auto blank = clean_blank(result[6]);
			pos += utf16conv.from_bytes(blank).size();
			sentence.text << blank;
		}
		else if(!result.empty() && result[7].length() != 0) { // flush
			sentence.runstate = flushing;
			break;
		}
		else {
			// Blank lines without the prefix don't go into text output!
		}
	} while(std::getline(is, line));

	if(!readinglines.empty()) {
		const auto& reading = proc_reading(t, readinglines);
		readinglines = "";
		if(!reading.errtype.empty()) {
			c.errtypes.insert(reading.errtype);
		}
		if(reading.id != 0) {
			c.id = reading.id;
		}
		c.added = reading.added == NotAdded ? c.added : reading.added;
		c.readings.push_back(reading);
	}

	c.pos = pos;
	if(!cohort_empty(c)) {
		sentence.cohorts.push_back(c);
		if(c.id != 0) {
			sentence.ids_cohorts[c.id] = sentence.cohorts.size() - 1;
		}
	}
	if(!c.added) {
		pos += c.form.size();
		sentence.text << utf16conv.to_bytes(c.form);
	}
	return sentence;
}

bool compareByBeg(const Err &a, const Err &b)
{
    return a.beg < b.beg;
}

bool compareByEnd(const Err &a, const Err &b)
{
    return a.end < b.end;
}

/**
 * Errors may overlap, in which case we want to expand their forms,
 * replacements and beg/end indices so we never have *partial* overlaps.
 *
 * Ie. if we have
 *
 * [["dego lávvomuorran",0,17,"syn-not-dego","Remove dego when essive",["lávvomuorran"]]
 * ,["lávvomuorran",5,17,"syn-dego-nom","Nominative after dego",["lávvomuorra"]]]
 *
 * then we want
 *
 * [["dego lávvomuorran",0,17,"syn-not-dego","Remove dego when essive",["lávvomuorran"]]
 * ,["dego lávvomuorran",0,17,"syn-dego-nom","Nominative after dego",["dego lávvomuorran"]]]
 *
 * Unfortunately, naïve interval-checking is O(2N²). We can perhaps
 * mitigate the blow-up by sending shorter Sentence's in (errors
 * shouldn't cross paragraph boundaries?). Or we could first create an
 * Interval Tree or similar, if this turns out to be a bottleneck.
 * (e.g.
 * https://github.com/ekg/intervaltree/blob/master/IntervalTree.h )
 */
void expand_errs(vector<Err>& errs, const u16string& text) {
	const auto n = errs.size();
	if(n < 2) {
		return;
	}
	// First expand "backwards" towards errors with lower beg's:
	// Since we sort errs by beg, we only have to compare
	// this.beg against the set of lower beg, and filter out
	// those that have a lower end than this.beg (or the exact same beg)
	std::sort(errs.begin(), errs.end(), compareByBeg);
	for(size_t i = 1; i < n; ++i) {
		auto& e = errs[i]; // mut
		for(size_t j = 0; j < i; ++j) {
			const auto& f = errs[j];
			if(f.beg < e.beg &&
			   f.end >= e.beg) {
				const auto len = e.beg - f.beg;
				const auto& add = text.substr(f.beg, len);
				e.form = add + e.form;
				e.beg = e.beg - len;
				for(auto& r : e.rep) {
					r = add + r;
				}
			}
		}
	}
	// Then expand "forwards" towards errors with higher end's:
	std::sort(errs.begin(), errs.end(), compareByEnd);
	for(size_t i = n - 1; i > 0; --i) {
		auto& e = errs[i-1]; // mut
		for(size_t j = n; j > i; --j) {
			const auto& f = errs[j-1];
			if(f.end > e.end &&
			   f.beg <= e.end) {
				const auto len = f.end - e.end;
				const auto& add = text.substr(e.end, len);
				e.form = e.form + add;
				e.end = e.end + len;
				for(auto& r : e.rep) {
					r = r + add;
				}
			}
		}
	}
}

vector<Err> Suggest::mk_errs(const Sentence &sentence) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto& text = utf16conv.from_bytes(sentence.text.str());
	vector<Err> errs;
	for(const auto& c : sentence.cohorts) {
		std::map<ErrId, vector<size_t>> c_errs;
		for(size_t i = 0; i < c.readings.size(); ++i) {
			const auto& r = c.readings[i];
			if(r.link) {
				continue;
			}
			c_errs[r.errtype].push_back(i);
		}
		for(const auto& e : c_errs) {
			if(e.first.empty()) {
				continue;
			}
			cohort_errs(e.first, c, sentence, text).match(
				[]  (Nothing) {},
				[&] (Err e)   { errs.push_back(e); });
		}
	}
	expand_errs(errs, text);
	return errs;
}

vector<Err> Suggest::run_errs(std::istream& is)
{
	return mk_errs(run_sentence(is, *generator, msgs));
}


RunState Suggest::run_json(std::istream& is, std::ostream& os)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	json::sanity_test();
	Sentence sentence = run_sentence(is, *generator, msgs);

	// All processing done, output:
	os << "{"
	   << json::key(u"errs")
	   << "[";
	bool wantsep = false;
	vector<Err> errs = mk_errs(sentence);
	for(const auto& e : errs) {
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


void print_cg_reading(const string& readinglines, std::ostream& os, const hfst::HfstTransducer& t, std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>& utf16conv) {
	os << readinglines;
	const auto& reading = proc_reading(t, readinglines);
	if(reading.suggest) {
		// std::cerr << "\033[1;35mreading.suggest=\t" << reading.suggest << "\033[0m" << std::endl;

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
	string readinglines;
	for (string line;std::getline(is, line);) {
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
		while(run_json(is, os) == flushing);
	}
	else {
		run_cg(is, os, *generator); // ignores ignores
	}
}

SortedMsgLangs sortMessageLangs(const MsgMap& msgs, const string& prefer) {
	SortedMsgLangs s;
	if(msgs.count(prefer) != 0) {
		s.push_back(prefer);
	}
	for(const auto& lm : msgs) {
		if(lm.first != prefer) {
			s.push_back(lm.first);
		}
	}
	return s;
}

Suggest::Suggest (const hfst::HfstTransducer* generator_, divvun::MsgMap msgs_, const string& locale_, bool verbose)
	: msgs(msgs_)
	, locale(locale_)
	, sortedmsglangs(sortMessageLangs(msgs, locale))
	, generator(generator_)
{
}
Suggest::Suggest (const string& gen_path, const string& msg_path, const string& locale_, bool verbose)
	: msgs(readMessages(msg_path))
	, locale(locale_)
	, sortedmsglangs(sortMessageLangs(msgs, locale))
	, generator(readTransducer(gen_path))
{
}
Suggest::Suggest (const string& gen_path, const string& locale_, bool verbose)
	: locale(locale_)
	, generator(readTransducer(gen_path))
{
}

void Suggest::setIgnores(const std::set<ErrId>& ignores_)
{
	ignores = ignores_;
}

}
