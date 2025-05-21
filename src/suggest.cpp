/*
* Copyright (C) 2015-2023, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

const std::basic_regex<char> CG_TAGS_RE("\"[^\"]*\"S?|[^ ]+");

// Anything *not* matched by CG_TAG_TYPE is sent to the generator.
// … or we could make an (h)fst out of these to match on lines :)
const std::basic_regex<char> CG_TAG_TYPE(
  "^"
  "(#"               // Group 1: Dependencies, comments
  "|&(.+)"           // Group 2: Errtype
  "|R:(.+):([0-9]+)" // Group 3 & 4: Relation name and target
  "|ID:([0-9]+)"     // Group 5: Relation ID
  "|\"([^\"]+)\"S"   // Group 6: Reading word-form
  "|(<fixedcase>)"   // Group 7: Fixed Casing
  "|\"<([^>]+)>\""   // Group 8: Broken word-form from MWE-split
  "|[cC][oO]&(.+)"   // Group 9: COERROR errtype (for example co&err-agr)
  "|DELETE"          // Shorthand for delete relation to self
  "|@"               // Syntactic tag
  "|Sem/"            // Semantic tag
  "|§"               // Semantic role
  "|<"               // Weights (<W:0>) and such
  "|ADD:"            // --trace
  "|PROTECT:"
  "|UNPROTECT:"
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
  ").*");

const std::basic_regex<char> MSG_TEMPLATE_REL("^[$][0-9]+$");
const std::basic_regex<char> DELETE_REL("^DELETE[0-9]*");
const std::basic_regex<char> LEFT_RIGHT_DELETE_REL(
  "^(LEFT|RIGHT|DELETE[0-9]*)$");


enum LineType { WordformL, ReadingL, BlankL };


#ifdef HAVE_LIBPUGIXML
const MsgMap readMessagesXml(
  pugi::xml_document& doc, pugi::xml_parse_result& result) {
	MsgMap msgs;

	if (result) {
		// <default>'s:
		for (pugi::xml_node def :
		  doc.child("errors").child("defaults").children("default")) {
			// Regexes we first have to store as strings, then put the compiled versions in msgs[lang].second:
			std::unordered_map<Lang, std::unordered_map<string, Msg>> regexes;
			// For all <title>'s and <description>'s, add all their parent <id>/<re>'s:
			for (pugi::xml_node child :
			  def.child("header").children("title")) {
				const u16string& msg = fromUtf8(xml_raw_cdata(child));
				const Lang& lang = child.attribute("xml:lang").value();
				for (pugi::xml_node e : def.child("ids").children("e")) {
					// e_value assumes we only ever have one PCDATA element here:
					const auto& errtype = fromUtf8(e.attribute("id").value());
					if (msgs[lang].first.count(errtype) != 0) {
						std::cerr
						  << "divvun-suggest: WARNING: Duplicate titles for "
						  << e.attribute("id").value() << std::endl;
					}
					// Default to <title> as <description>, may be overridden below:
					msgs[lang].first[errtype] = make_pair(msg, msg);
				}
				for (pugi::xml_node re : def.child("ids").children("re")) {
					regexes[lang][re.attribute("v").value()] =
					  make_pair(msg, msg);
				}
			}
			for (pugi::xml_node child :
			  def.child("body").children("description")) {
				const u16string& msg = fromUtf8(xml_raw_cdata(child));
				const Lang& lang = child.attribute("xml:lang").value();
				for (pugi::xml_node e : def.child("ids").children("e")) {
					const auto& errtype = fromUtf8(e.attribute("id").value());
					auto& langmsgs = msgs[lang].first;
					if (langmsgs.find(errtype) != langmsgs.end()) {
						langmsgs[errtype].second = msg;
					}
					else {
						// No <title> for this language, fallback to <description>:
						langmsgs[errtype] = std::make_pair(msg, msg);
					}
				}
				for (pugi::xml_node re : def.child("ids").children("re")) {
					const auto& rstr = re.attribute("v").value();
					auto& langmsgs = regexes[lang];
					if (langmsgs.find(rstr) != langmsgs.end()) {
						langmsgs[rstr].second = msg;
					}
					else {
						// No <title> for this language, fallback to <description>:
						langmsgs[rstr] = std::make_pair(msg, msg);
					}
				}
			}
			for (const auto& langres : regexes) {
				const Lang& lang = langres.first;
				for (const auto& rstrmsg : langres.second) {
					std::basic_regex<char> r(rstrmsg.first);
					msgs[lang].second.push_back(
					  std::make_pair(r, rstrmsg.second));
				}
			}
		}
		// <error>'s
		for (pugi::xml_node error : doc.child("errors").children("error")) {
			const auto& errtype = fromUtf8(error.attribute("id").value());
			// For all <title>'s and <description>'s, add the <error id> attribute:
			for (pugi::xml_node child :
			  error.child("header").children("title")) {
				// child_value assumes we only ever have one PCDATA element here:
				const auto& msg = fromUtf8(xml_raw_cdata(child));
				const auto& lang = child.attribute("xml:lang").value();
				auto& langmsgs = msgs[lang].first;
				if (langmsgs.count(errtype) != 0) {
					std::cerr
					  << "divvun-suggest: WARNING: Duplicate <title>'s for "
					  << error.attribute("id").value() << std::endl;
				}
				langmsgs[errtype] = make_pair(msg, msg);
			}
			for (pugi::xml_node child :
			  error.child("body").children("description")) {
				const auto& msg = fromUtf8(xml_raw_cdata(child));
				const auto& lang = child.attribute("xml:lang").value();
				auto& langmsgs = msgs[lang].first;
				if (langmsgs.find(errtype) != langmsgs.end()) {
					langmsgs[errtype].second = msg;
				}
				else {
					// No <title> for this language, fallback to <description>:
					langmsgs[errtype] = std::make_pair(msg, msg);
				}
			}
		}
	}
	else {
		std::cerr << "(buffer):" << result.offset
		          << " ERROR: " << result.description() << "\n";
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


const Reading proc_subreading(const string& line, bool generate_all_readings) {
	Reading r;
	const auto& lemma_beg = line.find("\"");
	const auto& lemma_end = line.find("\" ", lemma_beg);
	const auto& lemma = line.substr(lemma_beg + 1, lemma_end - lemma_beg - 1);
	const auto& tags = line.substr(lemma_end + 2);
	StringVec gentags; // tags we generate with
	r.id = 0; // CG-3 id's start at 1, should be safe. Want sum types :-/
	r.wf = "";
	r.suggest = false || generate_all_readings;
	r.suggestwf = false;
	r.added = NotAdded;
	r.coerror = false;
	r.fixedcase = false;
	bool delete_self =
	  false; // may be changed by DELETE tag, but need to know id to set the relation
	for (auto& tag : allmatches(tags, CG_TAGS_RE)) { // proc_tags
		std::match_results<const char*> result;
		std::regex_match(tag.c_str(), result, CG_TAG_TYPE);
		if (tag == "&LINK" || tag == "&COERROR" || tag == "COERROR") { // &LINK and COERROR kept for backward-compatibility
			r.coerror = true;
		}
		else if (tag == "DROP-PRE-BLANK") {
			r.drop_pre_blank = true;
		}
		else if (tag == "&SUGGEST" || tag == "SUGGEST") { // &SUGGEST kept for backward-compatibility
			r.suggest = true;
		}
		else if (tag == "&SUGGESTWF" || tag == "SUGGESTWF") { // &SUGGESTWF kept for backward-compatibility
			r.suggestwf = true;
		}
		else if (tag == "&ADDED" || tag == "ADDED") {
			r.added = AddedEnsureBlanks;
		}
		else if (tag == "&ADDED-AFTER-BLANK" || tag == "ADDED-AFTER-BLANK") {
			r.added = AddedAfterBlank;
		}
		else if (tag == "&ADDED-BEFORE-BLANK" || tag == "ADDED-BEFORE-BLANK") {
			r.added = AddedBeforeBlank;
		}
		else if (result.empty()) {
			gentags.push_back(tag);
		}
		else if (result[2].length() != 0) {
			r.errtypes.insert(fromUtf8(result[2]));
		}
		else if (tag == "DELETE") { // Shorthand: the tag DELETE means R:DELETE:id_of_this_cohort
			delete_self = true;
		}
		else if (result[3].length() != 0 && result[4].length() != 0) {
			try {
				rel_id target = stoi(result[4]);
				auto rel_name = result[3];
				r.rels.insert({ rel_name, target });
			}
			catch (...) {
				std::cerr << "divvun-suggest: WARNING: Couldn't parse "
				             "relation target integer"
				          << std::endl;
			}
		}
		else if (result[5].length() != 0) {
			try {
				r.id = stoi(result[5]);
			}
			catch (...) {
				std::cerr
				  << "divvun-suggest: WARNING: Couldn't parse ID integer"
				  << std::endl;
			}
		}
		else if (result[6].length() != 0) {
			r.wf = result[6];
		}
		else if (result[7].length() != 0) {
			r.fixedcase = true;
		}
		else if (result[8].length() != 0) {
			//            std::cerr << "divvun-suggest: WARNING: Broken MWE wordform in analyses: " << result[8] << std::endl;
			//            this breaks something hard to debug but cannot remember what..?
			r.wf = result[8];
		}
		else if (result[9].length() != 0) {
			r.coerrtypes.insert(fromUtf8(result[9]));
		}
	}
	if (delete_self) {
		r.rels.insert({ "DELETE", r.id });
	}
	const auto& tagsplus = join(gentags, "+");
	r.ana = lemma + "+" + tagsplus;
	if (r.suggestwf) {
		r.sforms.emplace_back(r.wf);
	}
	return r;
};


const Reading proc_reading(const hfst::HfstTransducer& generator,
  const string& line, bool generate_all_readings) {
	stringstream ss(line);
	string subline;
	std::deque<Reading> subs;
	while (std::getline(ss, subline, '\n')) {
		subs.push_front(proc_subreading(subline, generate_all_readings));
	}
	Reading r;
	r.line = line;
	const size_t n_subs = subs.size();
	for (size_t i = 0; i < n_subs; ++i) {
		const auto& sub = subs[i];
		r.ana += sub.ana + (i + 1 == n_subs ? "" : "#");
		r.errtypes.insert(sub.errtypes.begin(), sub.errtypes.end());
		r.coerrtypes.insert(sub.coerrtypes.begin(), sub.coerrtypes.end());
		r.rels.insert(sub.rels.begin(), sub.rels.end());
		// higher sub can override id if set; doesn't seem like cg3 puts ids on them though
		r.id = (r.id == 0 ? sub.id : r.id);
		r.suggest = r.suggest || sub.suggest || generate_all_readings;
		r.suggestwf = r.suggestwf || sub.suggestwf;
		r.coerror = r.coerror || sub.coerror;
		r.added = r.added == NotAdded ? sub.added : r.added;
		r.sforms.insert(r.sforms.end(), sub.sforms.begin(), sub.sforms.end());
		r.wf = r.wf.empty() ? sub.wf : r.wf;
		r.fixedcase |= sub.fixedcase;
		r.drop_pre_blank |= sub.drop_pre_blank;
	}
	dedupe(r.rels);
	if (r.suggest) {
		const HfstPaths1L paths(generator.lookup_fd({ r.ana }, -1, 10.0));
		for (auto& p : *paths) {
			stringstream form;
			for (auto& symbol : p.second) {
				if (!hfst::FdOperation::is_diacritic(symbol)) {
					form << symbol;
				}
			}
			r.sforms.emplace_back(form.str());
		}
	}
	return r;
}

bool cohort_empty(const Cohort& c) {
	return c.form.empty();
}

const Cohort DEFAULT_COHORT = { {}, 0, 0, {}, {}, {}, NotAdded, {} };

// https://stackoverflow.com/a/1464684/69663
template<class Iterator>
Iterator Dedupe(Iterator first, Iterator last) {
	while (first != last) {
		Iterator next(first);
		last = std::remove(++next, last, *first);
		first = next;
	}
	return last;
}

/*
 * If we find a match in relations for `name`, call 'fn' with the
 * match and target cohort, along with the index into the sentence
 * vector.
 */
void rel_on_match(const relations& rels, const std::basic_regex<char>& name,
  const Sentence& sentence,
  const std::function<void(
    const string& relname, size_t i_t, const Cohort& trg)>& fn) {
	for (const auto& rel : rels) {
		std::match_results<const char*> result;
		std::regex_match(rel.first.c_str(), result, name);
		if (result.empty()) { // Other relation
			continue;
		}
		const auto& target_id = rel.second;
		if (sentence.ids_cohorts.find(target_id) ==
		    sentence.ids_cohorts.end()) {
			std::cerr
			  << "divvun-suggest: WARNING: Couldn't find relation target for "
			  << rel.first << ":" << rel.second << std::endl;
			continue;
		}
		const auto& i_t = sentence.ids_cohorts.at(target_id);
		if (i_t >= sentence.cohorts.size()) {
			std::cerr
			  << "divvun-suggest: WARNING: Couldn't find relation target for "
			  << rel.first << ":" << rel.second << std::endl;
			continue;
		}
		fn(rel.first, i_t, sentence.cohorts.at(i_t));
	}
}

/**
 * Calculate the left/right bounds of the error underline, as indices into sentence.
 */
const std::pair<size_t, size_t> squiggle_bounds(const relations& rels,
  const Sentence& sentence, const size_t& i_fallback, const Cohort& fallback) {
	size_t left = i_fallback;
	size_t right = i_fallback;
	// If we have several relation targets, prefer leftmost if LEFT, rightmost if RIGHT:
	rel_on_match(rels, LEFT_RIGHT_DELETE_REL, sentence,
	  [&](const string& relname, size_t i_trg, const Cohort& trg) {
		  if (trg.id == 0) {
			  return; // unexpected, CG should always give id's to relation targets
		  }
		  if (i_trg < left) {
			  left = i_trg;
		  }
		  if (i_trg > right) {
			  right = i_trg;
		  }
	  });
	if (left < 0) {
		std::cerr << "divvun-suggest: WARNING: Left underline boundary "
		             "relation target "
		          << left << " out of bounds " << std::endl;
		left = 0;
	}
	if (right >= sentence.cohorts.size()) {
		std::cerr
		  << "divvun-suggest: WARNING: Right underline relation target "
		  << right << " out of bounds " << std::endl;
		right = sentence.cohorts.size() - 1;
	}
	return std::make_pair(left, right);
}


/**
 * Return the readings of Cohort trg that have ErrId err_id and apply
 * some change; fallback to all the readings if no match.
 *
 * The reason we want readings with a certain error type, is if we
 * have overlapping errors where e.g. one wants to DELETE a word. We
 * don't want to suggest a deletion on the suggestions of the *other*
 * error types.
 *
 * The reason we fallback to all the readings is that some times
 * people write CG rules that delete the COERROR readings or similar –
 * we don't want to fail to find the cohort that's supposed to be
 * underlined in that case.
 *
 * TODO: return references, not copies
 */
vector<Reading> readings_with_errtype(
  const Cohort& trg, const ErrId& err_id, bool applies_deletion) {
	vector<Reading> filtered(trg.readings.size());
	auto it = std::copy_if(trg.readings.begin(), trg.readings.end(),
	  filtered.begin(), [&](const Reading& tr) {
		  bool has_our_errtag =
		    tr.errtypes.find(err_id) != tr.errtypes.end() ||
		    tr.coerrtypes.find(err_id) != tr.coerrtypes.end();
		  bool applies_change =
		    tr.added != NotAdded || !tr.sforms.empty() || applies_deletion;
		  return has_our_errtag && applies_change;
	  });
	filtered.resize(std::distance(filtered.begin(), it));
	if (filtered.empty()) {
		vector<Reading> not_just_other_errtype(trg.readings.size());
		auto it = std::copy_if(trg.readings.begin(), trg.readings.end(),
		  not_just_other_errtype.begin(), [&](const Reading& tr) {
			  bool has_our_errtag =
			    tr.errtypes.find(err_id) != tr.errtypes.end() ||
			    tr.coerrtypes.find(err_id) != tr.coerrtypes.end();
			  bool no_errtags = tr.errtypes.empty() && tr.coerrtypes.empty();
			  return no_errtags || has_our_errtag;
		  });
		not_just_other_errtype.resize(
		  std::distance(not_just_other_errtype.begin(), it));
		return not_just_other_errtype;
	}
	else {
		return filtered;
	}
}

/**
 * CG relations unfortunately go from Cohort to Cohort. But we may
 * have several readings on one cohort with different error tags, only
 * one of which should apply the R:DELETE1 relation. We check the
 * target of the delete relation, if it has the same error tag, then
 * the relation applies. (If there's no ambiguity, we can always
 * delete).
 */
bool do_delete(const Cohort& trg, const ErrId& err_id,
  const std::set<u16string>& src_errtypes, const std::set<rel_id>& deletions) {
	if (deletions.find(trg.id) == deletions.end()) {
		// There is no deletion of this target cohort
		return false;
	}
	if (src_errtypes.size() < 2) {
		// Just one error type, no need to disambiguate which one has the relation
		return true;
	}
	// There are several err_id's on src; we should only delete
	// trg in err_id replacement if trg has err_id
	for (const auto& tr : trg.readings) {
		if (tr.errtypes.find(err_id) != tr.errtypes.end() ||
		    tr.coerrtypes.find(err_id) != tr.coerrtypes.end()) {
			return true;
		}
	}
	// But what if source and target have no matching errtypes at all?
	std::set<std::u16string> trg_errtypes_w_co;
	trg_errtypes_w_co.insert(trg.errtypes.begin(), trg.errtypes.end());
	trg_errtypes_w_co.insert(trg.coerrtypes.begin(), trg.coerrtypes.end());
	std::set<std::u16string> errtypes_isect;
	std::set_intersection(trg_errtypes_w_co.begin(), trg_errtypes_w_co.end(),
	  src_errtypes.begin(), src_errtypes.end(),
	  std::inserter(errtypes_isect, errtypes_isect.begin()));
	if (errtypes_isect.empty()) {
		// No matching err types at all on trg, we can't filter on errtype, allow deletion
		return true;
	}
	else {
		// Not found with this errtype, but there is another possible match, don't allow deletion:
		return false;
	}
}

bool both_spaces(char16_t lhs, char16_t rhs) {
	return (lhs == rhs) && (lhs == u' ');
}

/*
 * Return possibly altered beg/end indices for the Err coverage
 * (underline), along with a replacement suggestion (or Nothing() if
 * given bad data).
 */
variant<Nothing, pair<pair<size_t, size_t>, UStringVector>>
build_squiggle_replacement(const Reading& r, const ErrId& err_id,
  const size_t i_c, const Cohort& src, const Sentence& sentence,
  const size_t orig_beg, const size_t orig_end, const size_t i_left,
  const size_t i_right, bool verbose) {
	size_t beg = orig_beg;
	size_t end = orig_end;
	std::set<rel_id> deletions;
	bool src_applies_deletion = false;
	rel_on_match(r.rels, DELETE_REL, sentence,
	  [&](const string& relname, size_t i_t, const Cohort& trg) {
		  deletions.insert(trg.id);
		  if (trg.errtypes.find(err_id) != trg.errtypes.end()) {
			  src_applies_deletion = true;
		  }
		  if (trg.coerrtypes.find(err_id) != trg.coerrtypes.end()) {
			  src_applies_deletion = true;
		  }
	  });
	std::map<pair<size_t, size_t>, pair<u16string, Reading>>
	  add; // position in text:cohort in Sentence
	// Loop from the leftmost to the rightmost of source and target cohorts:
	if (verbose) std::cerr << "\033[1;31m=== err_id=\t" << toUtf8(err_id) << " ===\033[0m" << std::endl;
	if (verbose) std::cerr << "\033[1;33mr.id=\t" << r.id << "\033[0m" << std::endl;
	if (verbose) if(r.drop_pre_blank) std::cerr << "\033[1;33mr.drop_pre_blank=\t" << r.drop_pre_blank << "\033[0m" << std::endl;
	if (verbose) std::cerr << "\033[1;33msrc.id=\t" << src.id << "\033[0m" << std::endl;
	if (verbose) std::cerr << "\033[1;33mi_c=\t" << i_c << "\033[0m" << std::endl;
	if (verbose) std::cerr << "\033[1;33mleft=\t" << i_left << "\033[0m" << std::endl;
	if (verbose) std::cerr << "\033[1;33mright=\t" << i_right << "\033[0m" << std::endl;
	UStringVector reps = { u"" };
	string prev_added_before_blank = "";
	std::optional<Casing> addedcasing = std::nullopt;
	for (size_t i = i_left; i <= i_right; ++i) {
		const auto& trg = sentence.cohorts[i];
		Casing casing = getCasing(toUtf8(trg.form));

		if (verbose) std::cerr << "\033[1;34mi=\t" << i << "\033[0m" << std::endl;
		if (verbose) std::cerr << "\033[1;34mtrg.form=\t'" << toUtf8(trg.form) << "'\033[0m" << std::endl;
		if (verbose) std::cerr << "\033[1;34mtrg.id=\t" << trg.id << "\033[0m" << std::endl;
		if (verbose) std::cerr << "\033[1;35mtrg.raw_pre_blank=\t'" << trg.raw_pre_blank << "'\033[0m" << std::endl;

		UStringVector rep_this_trg;
		const bool del = do_delete(trg, err_id, src.errtypes, deletions);
		if (del) {
			rep_this_trg.push_back(u"");
			if (verbose) std::cerr << "\t\t\033[1;36mdelete=\t" << toUtf8(trg.form) << "\033[0m" << std::endl;
		}

		if (trg.added) {
			// This word was added, get casing from a non-added word to the right:
			for (size_t j = i; j <= i_right; j++) {
				const auto& right_of_trg = sentence.cohorts[j];
				if (!right_of_trg.added) {
					addedcasing = casing;
					casing = getCasing(toUtf8(right_of_trg.form));
					break;
				}
			}
		}
		else if (addedcasing.has_value() && !del) {
			// This word was not &ADDED, but is preceded by an added word:
			casing = addedcasing.value();
			addedcasing = std::nullopt;
		}

		bool added_before_blank = false;
		bool fixedcase = false;
		bool applies_deletion = trg.id == src.id && src_applies_deletion;
		size_t trg_beg = trg.pos;
		size_t trg_end = trg.pos + trg.form.size();
		for (const Reading& tr : readings_with_errtype(trg, err_id, applies_deletion)) {
			if (verbose) std::cerr << "\033[1;32mtr.line=\t" << tr.line << "\033[0m" << std::endl;
			// Update beg/end:
			if (tr.added == AddedBeforeBlank) {
				if (i == 0) {
					std::cerr << "divvun-suggest: WARNING: Saw &ADDED-BEFORE-BLANK on " "initial word, ignoring" << std::endl;
					continue;
				}
				const auto& pretrg = sentence.cohorts[i - 1];
				trg_beg = pretrg.pos + pretrg.form.size();
				added_before_blank = true;
			}
			added_before_blank |= tr.drop_pre_blank;
			if (tr.added != NotAdded) { // Don't replace existing form if Added/AddedBeforeBlank
				trg_end = trg_beg;
			}
			if (verbose) std::cerr << "\t\033[1;35mr.wf='" << tr.wf << "'\033[0m";
			if (verbose) std::cerr << "\t\033[0;35mr.coerror=" << tr.coerror << "\033[0m";
			if (verbose) std::cerr << "\t\033[0;35mr.suggestwf=" << tr.suggestwf << "\033[0m";
			if (verbose) std::cerr << "\t\033[0;35mr.suggest=" << tr.suggest << "\033[0m" << "\t" << tr.line;
			// Collect SUGGEST/SUGGESTWF:
			if (!del)
				for (const auto& sf : tr.sforms) {
					const auto cased_sf =
					  fromUtf8(withCasing(tr.fixedcase, casing, sf));
					rep_this_trg.push_back(cased_sf);
					if (verbose) std::cerr << "\t\t\033[1;36msform=\t'" << sf << "'\033[0m" << std::endl;
				}
			fixedcase |= tr.fixedcase; // for the surface form
		} // end for readings of target

		if (rep_this_trg.empty()) {
			const auto cased_sf =
			  fromUtf8(withCasing(fixedcase, casing, toUtf8(trg.form)));
			rep_this_trg.push_back(cased_sf);
		}
		beg = std::min(beg, trg_beg);
		end = std::max(end, trg_end);

		UStringVector reps_next;
		for (auto& rep : reps) {
			// Prepend blank unless at left edge:
			const auto pre_blank = i == i_left || added_before_blank || r.drop_pre_blank ?
				"" :
				clean_blank(prev_added_before_blank + trg.raw_pre_blank);
			// For &ADDED, enclose in blanks (unneeded blanks will get cleaned later):
			const auto post_blank = trg.added ? u" " : u"";
			for (const auto& sform : rep_this_trg) {
				reps_next.push_back(
				  rep + fromUtf8(pre_blank) + sform + post_blank);
			}
		}
		reps.swap(reps_next);
		prev_added_before_blank = added_before_blank ? trg.raw_pre_blank : "";
	} // end for target cohorts
	// We never want to add whitespace to ends of suggestions (typically deleted words)
	// and we never want double spaces in suggestions
	for (auto& rep : reps) {
		rep.erase(std::unique(rep.begin(), rep.end(), both_spaces),
		  rep.end()); // remove double spaces
		rep.erase(1 + rep.find_last_not_of(' '));
		rep.erase(0, rep.find_first_not_of(' '));
	}
	if (verbose) for (const auto& sf : reps) {std::cerr << "\033[1;35mreps sf=\t'" << toUtf8(sf) << "'\033[0m\t" << beg << "," << end << std::endl;}
	return std::make_pair(std::make_pair(beg, end), reps);
}

variant<Nothing, Err> Suggest::cohort_errs(const ErrId& err_id, size_t i_c,
  const Cohort& c, const Sentence& sentence, const u16string& text) {
	if (cohort_empty(c) || c.added != NotAdded) {
		return Nothing();
	}
	else if (ignores.find(err_id) != ignores.end()) {
		return Nothing();
	}
	else if (!includes.empty() && includes.find(err_id) == includes.end()) {
		return Nothing();
	}
	// Begin set msg:
	Msg msg;
	for (const auto& mlang : sortedmsglangs) {
		if (msg.second.empty() && mlang != locale) {
			std::cerr << "divvun-suggest: WARNING: No <description> for "
			          << json::str(err_id) << " in xml:lang '" << locale
			          << "', trying '" << mlang << "'" << std::endl;
		}
		const auto& lmsgs = msgs.at(mlang);
		if (lmsgs.first.count(err_id) != 0) {
			msg = lmsgs.first.at(err_id);
		}
		else {
			for (const auto& p : lmsgs.second) {
				std::match_results<const char*> result;
				const auto& et = toUtf8(err_id.c_str());
				std::regex_match(et.c_str(), result, p.first);
				if (!result.empty() && // Only consider full matches:
				    result.position(0) == 0 && result.suffix().length() == 0) {
					msg = p.second;
					break;
					// TODO: cache results? but then no more constness:
					// lmsgs.first.at(errId) = p.second;
				}
			}
		}
		if (!msg.second.empty()) {
			break;
		}
	}
	if (msg.second.empty()) {
		std::cerr << "divvun-suggest: WARNING: No <description> for "
		          << json::str(err_id) << " in any xml:lang" << std::endl;
		msg.second = err_id;
	}
	if (msg.first.empty()) {
		msg.first = err_id;
	}
	// TODO: Make suitable structure on creating MsgMap instead?
	replaceAll(msg.first, u"$1", c.form);
	replaceAll(msg.second, u"$1", c.form);
	for (const auto& r : c.readings) {
		if ((!r.errtypes.empty()) &&
		    r.errtypes.find(err_id) == r.errtypes.end()) {
			continue; // there is some other error on this reading
		}
		// Since we can have multiple relation targets, we first collect them, then apply them:
		std::unordered_map<string, u16string> msg_replacements;
		rel_on_match(r.rels, MSG_TEMPLATE_REL, sentence,
		  [&](const string& relname, size_t i_t, const Cohort& trg) {
			  if (msg_replacements.find(relname) == msg_replacements.end()) {
				  msg_replacements[relname] = trg.form;
			  }
			  else {
				  msg_replacements[relname] =
				    msg_replacements[relname] + u", " + trg.form;
			  }
		  });
		for (const auto& rep : msg_replacements) {
			replaceAll(msg.first, fromUtf8(rep.first), rep.second);
			replaceAll(msg.second, fromUtf8(rep.first), rep.second);
		}
	}
	// End set msg
	// Begin set beg, end, form, rep:
	auto beg = c.pos;
	auto end = c.pos + c.form.size();
	UStringVector rep;
	for (const Reading& r : c.readings) {
		if (r.errtypes.find(err_id) == r.errtypes.end()) {
			continue; // We consider sforms of SUGGEST readings in build_squiggle_replacement
		}
		// If there are LEFT/RIGHT added relations, add suggestions with those concatenated to our form
		// TODO: What about our current suggestions of the same error tag? Currently just using wordform
		const auto squiggle = squiggle_bounds(r.rels, sentence, i_c, c);
		std::visit(
		  [&](auto&& arg) {
			  using T = std::decay_t<decltype(arg)>;
			  if constexpr (std::is_same_v<T, Nothing>) {
			  }
			  if constexpr (std::is_same_v<T,
			                  pair<pair<size_t, size_t>, UStringVector>>) {
				  beg = arg.first.first;
				  end = arg.first.second;
				  rep.insert(rep.end(), arg.second.begin(), arg.second.end());
			  }
		  },
		  build_squiggle_replacement(r, err_id, i_c, c, sentence, beg, end,
		    squiggle.first, squiggle.second, verbose));
	}
	// Avoid unchanging replacements:
	auto form = text.substr(beg, end - beg);
	rep.erase(std::remove_if(rep.begin(), rep.end(),
	            [&](const u16string& r) { return r == form; }),
	  rep.end());
	// No duplicates:
	rep.erase(Dedupe(rep.begin(), rep.end()), rep.end());
	if (!rep.empty()) {
		replaceAll(msg.first, u"€1", rep[0]);
		replaceAll(msg.second, u"€1", rep[0]);
	}
	return Err{ form, beg, end, err_id, msg, rep };
}

/**
 * Remove unescaped [ and ] (superblank delimiters from
 * apertium-deformatter), turn \n into literal newline, unescape all
 * other escaped chars.
 */
const string clean_blank(const string& raw) {
	bool escaped = false;
	bool bol = true; // at beginning of line
	std::ostringstream text;
	for (const auto& c : raw) {
		if (bol && c == ':') {
			bol = false; // skip initial :
		}
		else if (escaped) {
			if (c == 'n') {
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
		else if (c == '\\') {
			escaped = true;
		}
		else if (c != '[' && c != ']') {
			text << c;
			escaped = false;
		}
		else if (c == '\n') {
			text << c;
			bol = true;
		}
	}
	return text.str();
}


Sentence Suggest::run_sentence(std::istream& is, FlushOn flush_on) {
	size_t pos = 0;
	Cohort c = DEFAULT_COHORT;
	Sentence sentence;
	sentence.runstate = Eof;

	string line;
	string raw_blank; // for CG output format
	string readinglines;
	std::getline(is,
	  line); // TODO: Why do I need at least one getline before os<< after flushing?
	do {
		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);

		if (!readinglines.empty() && // Reached end of readings
		    (result.empty() ||
		      (result[3].length() <= 1 && result[8].length() <= 1))) {
			const auto& reading =
			  proc_reading(*generator, readinglines, generate_all_readings);
			readinglines = "";
			c.errtypes.insert(
			  reading.errtypes.begin(), reading.errtypes.end());
			c.coerrtypes.insert(
			  reading.coerrtypes.begin(), reading.coerrtypes.end());
			if (reading.id != 0) {
				c.id = reading.id;
			}
			c.added = reading.added == NotAdded ? c.added : reading.added;
			c.readings.push_back(reading);
			if (flush_on == NulAndDelimiters) {
				if (delimiters.find(c.form) != delimiters.end()) {
					sentence.runstate = Flushing;
				}
				if (sentence.cohorts.size() >= hard_limit) {
					// We only respect hard_limit when flushing on delimiters (for the Nul only case we assume the calling API ensures requests are of reasonable size)
					std::cerr << "divvun-suggest: WARNING: Hard limit of "
					          << hard_limit
					          << " cohorts reached - forcing break."
					          << std::endl;
					sentence.runstate = Flushing;
				}
			}
		}

		if (!result.empty() &&
		    ((result[2].length() != 0) // wordform or blank: reset Cohort
		      || result[6].length() != 0)) {
			c.pos = pos;
			if (!cohort_empty(c)) {
				std::swap(c.raw_pre_blank, raw_blank);
				raw_blank.clear();
				sentence.cohorts.push_back(c);
				if (c.id != 0) {
					sentence.ids_cohorts[c.id] = sentence.cohorts.size() - 1;
				}
			}
			if (c.added == NotAdded) {
				pos += c.form.size();
				sentence.text << toUtf8(c.form);
			}
			c = DEFAULT_COHORT;
		}

		if (!result.empty() && result[2].length() != 0) { // wordform
			c.form = fromUtf8(result[2]);
		}
		else if (!result.empty() && result[3].length() != 0) { // reading
			readinglines += line + "\n";
		}
		else if (!result.empty() && result[6].length() != 0) { // blank
			raw_blank.append(line);
			const auto blank = clean_blank(result[6]);
			pos += fromUtf8(blank).size();
			sentence.text << blank;
		}
		else if (!result.empty() && result[7].length() != 0) { // flush
			sentence.runstate = Flushing;
		}
		else if (!result.empty() &&
		         result[8].length() != 0) { // traced removed reading
			c.trace_removed_readings += line + "\n";
		}
		else {
			// Blank lines without the prefix don't go into text output!
		}
		if (sentence.runstate == Flushing) {
			break;
		}
	} while (std::getline(is, line));

	if (!readinglines.empty()) {
		const auto& reading =
		  proc_reading(*generator, readinglines, generate_all_readings);
		readinglines = "";
		c.errtypes.insert(reading.errtypes.begin(), reading.errtypes.end());
		c.coerrtypes.insert(
		  reading.coerrtypes.begin(), reading.coerrtypes.end());
		if (reading.id != 0) {
			c.id = reading.id;
		}
		c.added = reading.added == NotAdded ? c.added : reading.added;
		c.readings.push_back(reading);
	}

	c.pos = pos;
	if (!cohort_empty(c)) {
		std::swap(c.raw_pre_blank, raw_blank);
		raw_blank.clear();
		sentence.cohorts.push_back(c);
		if (c.id != 0) {
			sentence.ids_cohorts[c.id] = sentence.cohorts.size() - 1;
		}
	}
	if (c.added == NotAdded) {
		pos += c.form.size();
		sentence.text << toUtf8(c.form);
	}
	sentence.raw_final_blank = raw_blank;

	mk_errs(sentence);
	return sentence;
}

bool compareByBeg(const Err& a, const Err& b) {
	return a.beg < b.beg;
}

bool compareByEnd(const Err& a, const Err& b) {
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
	if (n < 2) {
		return;
	}
	// First expand "backwards" towards errors with lower beg's:
	// Since we sort errs by beg, we only have to compare
	// this.beg against the set of lower beg, and filter out
	// those that have a lower end than this.beg (or the exact same beg)
	std::sort(errs.begin(), errs.end(), compareByBeg);
	for (size_t i = 1; i < n; ++i) {
		Err& e = errs[i]; // mut
		for (size_t j = 0; j < i; ++j) {
			const Err& f = errs[j];
			if (f.beg < e.beg && f.end >= e.beg) {
				const size_t len = e.beg - f.beg;
				const u16string& add = text.substr(f.beg, len);
				e.form = add + e.form;
				e.beg = e.beg - len;
				for (u16string& r : e.rep) {
					r = add + r;
				}
			}
		}
	}
	// Then expand "forwards" towards errors with higher end's:
	std::sort(errs.begin(), errs.end(), compareByEnd);
	for (size_t i = n - 1; i > 0; --i) {
		Err& e = errs[i - 1]; // mut
		for (size_t j = n; j > i; --j) {
			const auto& f = errs[j - 1];
			if (f.end > e.end && f.beg <= e.end) {
				const size_t len = f.end - e.end;
				const u16string& add = text.substr(e.end, len);
				e.form = e.form + add;
				e.end = e.end + len;
				for (u16string& r : e.rep) {
					r = r + add;
				}
			}
		}
	}
}

void Suggest::mk_errs(Sentence& sentence) {
	const u16string& text = fromUtf8(sentence.text.str());
	// Preprocessing, demote target &error to co&error:
	// Sometimes input has &errortag on relation targets instead of
	// co&errortag. We allow that, but we should then treat it as a
	// co&errortag (since the relation source is the "main" error):
	for (size_t i_c = 0; i_c < sentence.cohorts.size(); i_c++) {
		Cohort& source = sentence.cohorts[i_c];
		for (const auto& r : source.readings) {
			std::set<int> targets;
			rel_on_match(r.rels, LEFT_RIGHT_DELETE_REL, sentence,
			  [&](const string&, size_t i_trg, const Cohort&) {
				  targets.insert(i_trg);
			  });
			for (int i_t : targets) {
				auto& target = sentence.cohorts.at(i_t);
				demote_error_to_coerror(
				  source, target.errtypes, target.coerrtypes);
				for (Reading& tr : target.readings) {
					demote_error_to_coerror(
					  source, tr.errtypes, tr.coerrtypes);
				}
			}
		}
	}
	// Now actually find and mark up all the errors and suggestions:
	for (size_t i_c = 0; i_c < sentence.cohorts.size(); i_c++) {
		Cohort& c = sentence.cohorts[i_c];
		std::set<ErrId> c_errtypes;
		for (size_t i = 0; i < c.readings.size(); ++i) {
			const Reading& r = c.readings[i];
			if (
			  r.coerror) { // Needed for backwards-compatibility with `COERROR &errtag` readings
				continue;
			}
			c_errtypes.insert(r.errtypes.begin(), r.errtypes.end());
		}
		for (const auto& errtype : c_errtypes) {
			if (errtype.empty()) {
				continue;
			}

			std::visit(
			  [&](auto&& arg) {
				  using T = std::decay_t<decltype(arg)>;
				  if constexpr (std::is_same_v<T, Nothing>) {
				  }
				  if constexpr (std::is_same_v<T, Err>) {
					  c.errs.push_back(arg);
					  sentence.errs.push_back(arg);
				  }
			  },
			  cohort_errs(errtype, i_c, c, sentence, text));
		}
	}
	// Postprocessing for overlapping errors:
	expand_errs(sentence.errs, text);
}

vector<Err> Suggest::run_errs(std::istream& is) {
	try {
		auto _old = std::locale::global(std::locale(""));
	}
	catch (const std::runtime_error& e) {
		std::cerr
		  << "divvun-suggest: WARNING: Couldn't set global locale \"\" "
		     "(locale-specific native environment): "
		  << e.what() << std::endl;
	}
	return run_sentence(is, FlushOn::Nul).errs;
}


RunState Suggest::run_json(std::istream& is, std::ostream& os) {
	json::sanity_test();
	Sentence sentence = run_sentence(is, FlushOn::Nul);

	// All processing done, output:
	os << "{" << json::key(u"errs") << "[";
	bool wantsep = false;
	for (const auto& e : sentence.errs) {
		if (wantsep) {
			os << ",";
		}
		os << "[" << json::str(e.form) << "," << std::to_string(e.beg) << ","
		   << std::to_string(e.end) << "," << json::str(e.err) << ","
		   << json::str(e.msg.second) << "," << json::str_arr(e.rep) << ","
		   << json::str(e.msg.first) << "]";
		wantsep = true;
	}
	os << "]"
	   << "," << json::key(u"text") << json::str(fromUtf8(sentence.text.str()))
	   << "}";
	if (sentence.runstate == Flushing) {
		os << '\0';
		os.flush();
		os.clear();
	}
	return sentence.runstate;
}

RunState Suggest::run_autocorrect(std::istream& is, std::ostream& os) {
	json::sanity_test();
	Sentence sentence = run_sentence(is, FlushOn::Nul);

	size_t offset = 0;
	u16string text = fromUtf8(sentence.text.str());
	for (const auto& e : sentence.errs) {
		if (e.beg > offset) {
			os << toUtf8(text.substr(offset, e.beg - offset));
		}
		bool printed = false;
		for (const auto& r : e.rep) {
			os << toUtf8(r);
			printed = true;
			break;
		}
		if (!printed) {
			os << toUtf8(e.form);
		}
		offset = e.end;
	}
	os << toUtf8(text.substr(offset));

	if (sentence.runstate == Flushing) {
		os << '\0';
		os.flush();
		os.clear();
	}
	return sentence.runstate;
}

RunState Suggest::run_cg(std::istream& is, std::ostream& os) {
	Sentence sentence = run_sentence(is, FlushOn::NulAndDelimiters);
	size_t prev_err_start = 0;
	string colour_cur = "\033[35m";
	string colour_alt = "\033[36m";
	for (const Cohort& cohort : sentence.cohorts) {
		if (!cohort.raw_pre_blank.empty()) {
			os << cohort.raw_pre_blank << std::endl;
		}
		os << "\"<" << toUtf8(cohort.form) << ">\"";
		if (cohort.errs.size() > 0) {
			os << "\t";
		}
		for (const Err& err : cohort.errs) {
			if (prev_err_start != err.beg) {
				std::swap(colour_cur, colour_alt);
			}
			os << "\t\033[0;31m\033[4m" << toUtf8(err.form) << "\033[0m";
			for (const auto& rep : err.rep) {
				os << "\t→  \033[0;32m\033[3m" << toUtf8(rep) << "\033[0m";
			}
		}
		os << std::endl;
		for (const Reading& reading : cohort.readings) {
			os << reading.line; // includes final newline
			// TODO:
			if (reading.suggest) {
				const auto& ana = reading.ana;
				const auto& formv = reading.sforms;
				if (formv.empty()) {
					os << ana << "\t" << "\033[1m?\033[0m" << std::endl;
				}
				else {
					// TODO: get cased forms from errs
					os << ana << "\t" << join(formv, ",") << std::endl;
				}
			}
			else {
				for (const auto& e : reading.errtypes) {
					os << toUtf8(e) << std::endl;
				}
			}
		}
		os << cohort.trace_removed_readings;
	}
	if (!sentence.raw_final_blank.empty()) {
		os << sentence.raw_final_blank << std::endl;
	}
	return sentence.runstate;
}

void Suggest::run(std::istream& is, std::ostream& os, RunMode mode) {
	try {
		auto _old = std::locale::global(std::locale(""));
	}
	catch (const std::runtime_error& e) {
		std::cerr
		  << "divvun-suggest: WARNING: Couldn't set global locale \"\" "
		     "(locale-specific native environment): "
		  << e.what() << std::endl;
	}
	switch (mode) {
	case RunJson:
		while (run_json(is, os) == Flushing)
			;
		break;
	case RunAutoCorrect:
		while (run_autocorrect(is, os) == Flushing)
			;
		break;
	case RunCG:
		while (run_cg(is, os) == Flushing)
			;
		break;
	}
}

SortedMsgLangs sortMessageLangs(const MsgMap& msgs, const string& prefer) {
	SortedMsgLangs s;
	if (msgs.count(prefer) != 0) {
		s.push_back(prefer);
	}
	for (const auto& lm : msgs) {
		if (lm.first != prefer) {
			s.push_back(lm.first);
		}
	}
	return s;
}

Suggest::Suggest(const hfst::HfstTransducer* generator_, divvun::MsgMap msgs_,
  const string& locale_, bool verbose_, bool genall)
  : msgs(msgs_)
  , locale(locale_)
  , sortedmsglangs(sortMessageLangs(msgs, locale))
  , generator(generator_)
  , delimiters(defaultDelimiters())
  , generate_all_readings(genall)
  , verbose(verbose_) {}
Suggest::Suggest(const string& gen_path, const string& msg_path,
  const string& locale_, bool verbose_, bool genall)
  : msgs(readMessages(msg_path))
  , locale(locale_)
  , sortedmsglangs(sortMessageLangs(msgs, locale))
  , generator(readTransducer(gen_path))
  , delimiters(defaultDelimiters())
  , generate_all_readings(genall)
  , verbose(verbose_) {}
Suggest::Suggest(const string& gen_path, const string& locale_, bool verbose_)
  : locale(locale_)
  , generator(readTransducer(gen_path))
  , delimiters(defaultDelimiters())
  , verbose(verbose_) {}

void Suggest::setIgnores(const std::set<ErrId>& ignores_) {
	ignores = ignores_;
}

void Suggest::setIncludes(const std::set<ErrId>& includes_) {
	includes = includes_;
}

}
