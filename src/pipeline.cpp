/*
 * Copyright (C) 2017, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#include <cstdlib>

#include "pipeline.hpp"

namespace divvun {

void dbg(const string& label, stringstream& output) {
	const auto p = output.tellg();
	std::cerr << label << output.str();
	output.seekg(p, output.beg);
}

hfst_ol::PmatchContainer*
TokenizeCmd::mkContainer(std::istream& instream, bool verbose) {
	settings.output_format = hfst_ol_tokenize::giellacg;
	settings.tokenize_multichar = false; // TODO: https://github.com/hfst/hfst/issues/367#issuecomment-334922284
	settings.print_weights = true;
	settings.print_all = true;
	settings.dedupe = true;
	settings.max_weight_classes = 2;
	auto* c = new hfst_ol::PmatchContainer(instream);
	c->set_verbose(verbose);
	return c;
}
TokenizeCmd::TokenizeCmd (std::istream& instream, bool verbose)
	: container(mkContainer(instream, verbose))
{
}
TokenizeCmd::TokenizeCmd (const string& path, bool verbose)
	: istream(unique_ptr<std::istream>(new std::ifstream(path.c_str())))
	, container(mkContainer(*istream, verbose))
{
}
void TokenizeCmd::run(stringstream& input, stringstream& output) const
{
	hfst_ol_tokenize::process_input(*container, input, output, settings);
}


MweSplitCmd::MweSplitCmd (bool verbose)
	: applicator(cg3_mwesplitapplicator_create())
{
}
void MweSplitCmd::run(stringstream& input, stringstream& output) const
{
	cg3_run_mwesplit_on_text(applicator.get(), (std_istream*)&input, (std_ostream*)&output);
}



CGCmd::CGCmd (const char* buff, const size_t size, bool verbose)
	: grammar(cg3_grammar_load_buffer(buff, size))
	, applicator(cg3_applicator_create(grammar.get()))
{
	if(!grammar){
		throw std::runtime_error("ERROR: Couldn't load CG grammar");
	}
}
CGCmd::CGCmd (const string& path, bool verbose)
	: grammar(cg3_grammar_load(path.c_str()))
	, applicator(cg3_applicator_create(grammar.get()))
{
	if(!grammar){
		throw std::runtime_error(("ERROR: Couldn't load CG grammar " + path).c_str());
	}
}
void CGCmd::run(stringstream& input, stringstream& output) const
{
	cg3_run_grammar_on_text(applicator.get(), (std_istream*)&input, (std_ostream*)&output);
}


CGSpellCmd::CGSpellCmd (hfst_ospell::Transducer* errmodel, hfst_ospell::Transducer* acceptor, bool verbose)
	: speller(new Speller(errmodel, acceptor, verbose, max_analysis_weight, max_weight, real_word, limit, beam, time_cutoff))
{
	if (!acceptor) {
		throw std::runtime_error("ERROR: CGSpell command couldn't read acceptor");
	}
	if (!errmodel) {
		throw std::runtime_error("ERROR: CGSpell command couldn't read errmodel");
	}
}
CGSpellCmd::CGSpellCmd (const string& err_path, const string& lex_path, bool verbose)
	: speller(new Speller(err_path, lex_path, verbose, max_analysis_weight, max_weight, real_word, limit, beam, time_cutoff))
{
}
void CGSpellCmd::run(stringstream& input, stringstream& output) const
{
	divvun::run_cgspell(input, output, *speller);
}

BlanktagCmd::BlanktagCmd (const hfst::HfstTransducer* analyser, bool verbose)
	: blanktag(new Blanktag(analyser, verbose))
{
}
BlanktagCmd::BlanktagCmd (const string& ana_path, bool verbose)
	: blanktag(new Blanktag(ana_path, verbose))
{
}
void BlanktagCmd::run(stringstream& input, stringstream& output) const
{
	blanktag->run(input, output);
}

SuggestCmd::SuggestCmd (const hfst::HfstTransducer* generator, divvun::msgmap msgs, bool verbose)
	: suggest(new Suggest(generator, msgs, verbose))
{
}
SuggestCmd::SuggestCmd (const string& gen_path, const string& msg_path, bool verbose)
	: suggest(new Suggest(gen_path, msg_path, verbose))
{
}
void SuggestCmd::run(stringstream& input, stringstream& output) const
{
	suggest->run(input, output, true);
}
vector<Err> SuggestCmd::run_errs(stringstream& input) const
{
	return suggest->run_errs(input);
}
void SuggestCmd::setIgnores(const std::set<err_id>& ignores)
{
	suggest->setIgnores(ignores);
}
const msgmap& SuggestCmd::getMsgs()
{
	return suggest->msgs;
}




const size_t AR_BLOCK_SIZE = 10240;

template<typename Ret>
using ArEntryHandler = std::function<Ret(const string& ar_path, const void* buff, const size_t size)>;

template<typename Ret>
Ret archiveExtract(const string& ar_path,
		   archive *ar,
		   const string& entry_pathname,
		   ArEntryHandler<Ret>procFile)
{
	struct archive_entry* entry = nullptr;
	for (int rr = archive_read_next_header(ar, &entry);
	     rr != ARCHIVE_EOF;
	     rr = archive_read_next_header(ar, &entry))
	{
		if (rr != ARCHIVE_OK)
		{
			throw std::runtime_error("Archive not OK");
		}
		string filename(archive_entry_pathname(entry));
		if (filename == entry_pathname) {
			size_t fullsize = 0;
			const struct stat* st = archive_entry_stat(entry);
			size_t buffsize = st->st_size;
			if (buffsize == 0) {
				std::cerr << archive_error_string(ar) << std::endl;
				throw std::runtime_error("ERROR: Got a zero length archive entry for " + filename);
			}
			string buff(buffsize, 0);
			for (;;) {
				ssize_t curr = archive_read_data(ar, &buff[0] + fullsize, buffsize - fullsize);
				if (0 == curr) {
					break;
				}
				else if (ARCHIVE_RETRY == curr) {
					continue;
				}
				else if (ARCHIVE_FAILED == curr) {
					throw std::runtime_error("ERROR: Archive broken (ARCHIVE_FAILED)");
				}
				else if (curr < 0) {
					throw std::runtime_error("ERROR: Archive broken " + std::to_string(curr));
				}
				else {
					fullsize += curr;
				}
			}
			return procFile(ar_path, buff.c_str(), fullsize);
		}
	} // while r != ARCHIVE_EOF
	throw std::runtime_error("ERROR: Couldn't find " + entry_pathname + " in archive");
}

template<typename Ret>
Ret readArchiveExtract(const string& ar_path,
		       const string& entry_pathname,
		       ArEntryHandler<Ret>procFile)
{
	struct archive* ar = archive_read_new();
#if USE_LIBARCHIVE_2
	archive_read_support_compression_all(ar);
#else
	archive_read_support_filter_all(ar);
#endif // USE_LIBARCHIVE_2

	archive_read_support_format_all(ar);
	int rr = archive_read_open_filename(ar, ar_path.c_str(), AR_BLOCK_SIZE);
	if (rr != ARCHIVE_OK)
	{
		stringstream msg;
		msg << "Archive path not OK: '" << ar_path << "'" << std::endl;
		std::cerr << msg.str(); // TODO: why does the below cut off the string?
		throw std::runtime_error(msg.str());
	}
	Ret ret = archiveExtract(ar_path, ar, entry_pathname, procFile);

#if USE_LIBARCHIVE_2
	archive_read_close(ar);
	archive_read_finish(ar);
#else
	archive_read_free(ar);
#endif // USE_LIBARCHIVE_2
	return ret;
}

Pipeline::Pipeline(LocalisedPrefs prefs_,
		   vector<unique_ptr<PipeCmd>> cmds_,
		   SuggestCmd* suggestcmd_,
		   bool verbose_)
	: verbose(verbose_)
	, prefs(std::move(prefs_))
	, cmds(std::move(cmds_))
	, suggestcmd(suggestcmd_)
{
};

Pipeline::Pipeline (const unique_ptr<PipeSpec>& spec, const u16string& pipename, bool verbose)
	: Pipeline(mkPipeline(spec, pipename, verbose))
{
};

Pipeline::Pipeline (const unique_ptr<ArPipeSpec>& ar_spec, const u16string& pipename, bool verbose)
	: Pipeline(mkPipeline(ar_spec, pipename, verbose))
{
};

Pipeline Pipeline::mkPipeline(const unique_ptr<ArPipeSpec>& ar_spec, const u16string& pipename, bool verbose)
{
	LocalisedPrefs prefs;
	vector<unique_ptr<PipeCmd>> cmds;
	SuggestCmd* suggestcmd = nullptr;
	auto& spec = ar_spec->spec;
	if (!cg3_init(stdin, stdout, stderr)) {
		// TODO: Move into a lib-general init function? Or can I call this safely once per CGCmd?
		throw std::runtime_error("ERROR: Couldn't initialise ICU for vislcg3!");
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const pugi::xml_node& cmd: spec->pnodes.at(pipename).children()) {
		const auto& name = utf16conv.from_bytes(cmd.name());
		vector<string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.emplace_back(argn);
		}
		if(name == u"tokenise" || name == u"tokenize") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <tokenize> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<TokenizeCmd*> f = [verbose] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return new TokenizeCmd(is, verbose);
			};
			TokenizeCmd* s = readArchiveExtract(ar_spec->ar_path, args[0], f);
			cmds.emplace_back(s);
		}
		else if(name == u"cg") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <cg> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<CGCmd*> f = [verbose] (const string& ar_path, const void* buff, const size_t size) {
				return new CGCmd((char*)buff, size, verbose);
			};
			CGCmd* s = readArchiveExtract(ar_spec->ar_path, args[0], f);
			cmds.emplace_back(s);
		}
		else if(name == u"cgspell") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <cg> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<hfst_ospell::Transducer*> f = [] (const string& ar_path, const void* buff, const size_t size) {
				return new hfst_ospell::Transducer((char*)buff);
			};
			auto* s = new CGSpellCmd(readArchiveExtract(ar_spec->ar_path, args[0], f),
						 readArchiveExtract(ar_spec->ar_path, args[1], f),
						 verbose);
			cmds.emplace_back(s);
		}
		else if(name == u"mwesplit") {
			if(args.size() != 0) {
				throw std::runtime_error("Wrong number of arguments to <mwesplit> command (expected 0), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"blanktag") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <blanktag> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<const hfst::HfstTransducer*> f = [] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return readTransducer(is);
			};
			auto* s = new BlanktagCmd(readArchiveExtract(ar_spec->ar_path, args[0], f),
						  verbose);
			cmds.emplace_back(s);
		}
		else if(name == u"suggest") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <suggest> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<const hfst::HfstTransducer*> procGen = [] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return readTransducer(is);
			};
			ArEntryHandler<divvun::msgmap> procMsgs = [] (const string& ar_path, const void* buff, const size_t size) {
				return Suggest::readMessages((char*)buff, size);
			};
			auto* s = new SuggestCmd(readArchiveExtract(ar_spec->ar_path, args[0], procGen),
						 readArchiveExtract(ar_spec->ar_path, args[1], procMsgs),
						 verbose);
			cmds.emplace_back(s);
			mergePrefsFromMsgs(prefs, s->getMsgs());
			suggestcmd = s;
		}
		else if(name == u"sh") {
			throw std::runtime_error("<sh> command not implemented yet!");
			// const auto& prog = utf16conv.from_bytes(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			parsePrefs(prefs, cmd);
		}
		else {
			throw std::runtime_error("Unknown command '" + utf16conv.to_bytes(name) + "'");
		}
	}
	return Pipeline(std::move(prefs), std::move(cmds), suggestcmd, verbose);
}

Pipeline Pipeline::mkPipeline(const unique_ptr<PipeSpec>& spec, const u16string& pipename, bool verbose)
{
	LocalisedPrefs prefs;
	vector<unique_ptr<PipeCmd>> cmds;
	SuggestCmd* suggestcmd = nullptr;
	if (!cg3_init(stdin, stdout, stderr)) {
		// TODO: Move into a lib-general init function? Or can I call this safely once per CGCmd?
		throw std::runtime_error("ERROR: Couldn't initialise ICU for vislcg3!");
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const pugi::xml_node& cmd: spec->pnodes.at(pipename).children()) {
		const auto& name = utf16conv.from_bytes(cmd.name());
		vector<string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.emplace_back(argn);
		}
		if(name == u"tokenise" || name == u"tokenize") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <tokenize> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new TokenizeCmd(args[0], verbose));
		}
		else if(name == u"cg") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <cg> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new CGCmd(args[0], verbose));
		}
		else if(name == u"cgspell") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <cgspell> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new CGSpellCmd(args[0], args[1], verbose));
		}
		else if(name == u"mwesplit") {
			if(args.size() != 0) {
				throw std::runtime_error("Wrong number of arguments to <mwesplit> command (expected 0), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"blanktag") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <mwesplit> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new BlanktagCmd(args[0], verbose));
		}
		else if(name == u"suggest") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <suggest> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			auto *s = new SuggestCmd(args[0], args[1], verbose);
			cmds.emplace_back(s);
			mergePrefsFromMsgs(prefs, s->getMsgs());
			suggestcmd = s;
		}
		else if(name == u"sh") {
			throw std::runtime_error("<sh> command not implemented yet!");
			// const auto& prog = utf16conv.from_bytes(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			parsePrefs(prefs, cmd);
		}
		else {
			throw std::runtime_error("Unknown command '" + utf16conv.to_bytes(name) + "'");
		}
	}
	return Pipeline(std::move(prefs), std::move(cmds), suggestcmd, verbose);
}

void Pipeline::proc(stringstream& input, stringstream& output) {
	stringstream cur_in;
	stringstream cur_out(input.str());
	for(const auto& cmd: cmds) {
		cur_in.swap(cur_out);
		cur_out.clear();
		cur_out.str(string());
		cmd->run(cur_in, cur_out);
		// if(DEBUG) { dbg("cur_out after run", cur_out); }
	}
	output << cur_out.str();
}

vector<Err> Pipeline::proc_errs(stringstream& input) {
	if(suggestcmd == nullptr || cmds.empty() || suggestcmd != cmds.back().get()) {
		throw std::runtime_error("Can't create cohorts without a SuggestCmd as the final Pipeline command!");
	}
	stringstream cur_in;
	stringstream cur_out(input.str());
	size_t i_last = cmds.size()-1;
	for (size_t i = 0; i < i_last; ++i) {
		const auto& cmd = cmds[i];
		cur_in.swap(cur_out);
		cur_out.clear();
		cur_out.str(string());
		cmd->run(cur_in, cur_out);
	}
	cur_in.swap(cur_out);
	return suggestcmd->run_errs(cur_in);
}
void Pipeline::setIgnores(const std::set<err_id>& ignores) {
	if(suggestcmd != nullptr) {
		suggestcmd->setIgnores(ignores);
	}
	else if(!ignores.empty()) {
		throw std::runtime_error("ERROR: Can't set ignores when last command of pipeline is not a SuggestCmd");
	}
}

unique_ptr<PipeSpec> readPipeSpec(const string& file) {
	unique_ptr<PipeSpec> spec = unique_ptr<PipeSpec>(new PipeSpec);
	pugi::xml_parse_result result = spec->doc.load_file(file.c_str());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	if (result) {
		for (pugi::xml_node pipeline: spec->doc.child("pipespec").children("pipeline")) {
			const u16string& pipename = utf16conv.from_bytes(pipeline.attribute("name").value());
			auto pr = std::make_pair(pipename, pipeline);
			spec->pnodes[pipename] = pipeline;
		}
	}
	else {
		std::cerr << file << ":" << result.offset << " ERROR: " << result.description() << "\n";
		throw std::runtime_error("ERROR: Couldn't load the pipespec xml \"" + file + "\"");
	}
	return spec;
}

unique_ptr<ArPipeSpec> readArPipeSpec(const string& ar_path) {
	ArEntryHandler<unique_ptr<ArPipeSpec>> f = [] (const string& ar_path, const void* buff, const size_t size) {
		unique_ptr<ArPipeSpec> ar_spec = unique_ptr<ArPipeSpec>(new ArPipeSpec(ar_path));
		pugi::xml_parse_result result = ar_spec->spec->doc.load_buffer((pugi::char_t*)buff, size);
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		if (result) {
			for (pugi::xml_node pipeline: ar_spec->spec->doc.child("pipespec").children("pipeline")) {
				const u16string& pipename = utf16conv.from_bytes(pipeline.attribute("name").value());
				auto pr = std::make_pair(pipename, pipeline);
				ar_spec->spec->pnodes[pipename] = pipeline;
			}
		}
		else {
			std::cerr << "pipespec.xml:" << result.offset << " ERROR: " << result.description() << "\n";
			throw std::runtime_error("ERROR: Couldn't load the pipespec.xml");
		}
		return ar_spec;
	};
	return readArchiveExtract(ar_path, "pipespec.xml", f);
}

string makeDebugSuff(string name, vector<string> args) {
	if(name == "cg" && args.size() > 0) {
		if(args[0].substr(0, 14) == "grammarchecker") {
			return "gc";
		}
		if(args[0].substr(0, 7) == "mwe-dis") {
			return "mwe-dis";
		}
		if(args[0].substr(0, 7) == "valency") {
			return "val";
		}
		if(args[0].substr(0, 13) == "disambiguator") {
			return "disam";
		}
	}
	if(name == "tokenize" || name == "tokenise") {
		return "morph";
	}
	if(name == "suggest") {
		return "suggest";
	}
	if(name == "mwesplit") {
		return "mwe-split";
	}
	if(name == "blanktag") {
		return "blanktag";
	}
	if(name == "cgspell") {
		return "spell";
	}
	return name;
}

vector<std::pair<string,string>> toPipeSpecShVector(const string& dir, const pugi::xml_node& pipeline, const u16string& pipename, bool trace) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	vector<std::pair<string, string>> cmds = {};
	for (const pugi::xml_node& cmd: pipeline.children()) {
		const auto& name = std::string(cmd.name());
		string prog;
		vector<string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.emplace_back(argn);
		}
		if(name == "tokenise" || name == "tokenize") {
			prog = "hfst-tokenise -g";
		}
		else if(name == "cg") {
			prog = "vislcg3 -g";
			if(trace) {
				prog += " --trace";
			}
		}
		else if(name == "cgspell") {
			prog = "divvun-cgspell";
		}
		else if(name == "mwesplit") {
			prog = "cg-mwesplit";
		}
		else if(name == "blanktag") {
			prog = "divvun-blanktag";
		}
		else if(name == "suggest") {
			prog = "divvun-suggest";
		}
		else if(name == "sh") {
			prog = cmd.attribute("prog").value();
		}
		else if(name == "prefs") {
			// TODO: Do prefs make sense here?
		}
		else {
			throw std::runtime_error("Unknown command '" + name + "'");
		}
		if(!prog.empty()) {
			std::ostringstream part;
			part << prog;
			for(auto& a : args) {
				// Wrap the whole thing in single-quotes, but put existing single-quotes in double-quotes
				replaceAll(a, "'", "'\"'\"'");
				// os << " '" << (dir / a).string() << "'"; // <experimental/filesystem>
				part << " '" << dir << a << "'";
			}
			cmds.push_back(std::make_pair(part.str(),
						      makeDebugSuff(name, args)));
		}
	}
	return cmds;
}

void writePipeSpecSh(const string& specfile, const u16string& pipename, std::ostream& os) {
	const auto spec = readPipeSpec(specfile);
	// const auto dir = std::experimental::filesystem::absolute(specfile).remove_filename(); // <experimental/filesystem>
        char specabspath[PATH_MAX];
	realpath(specfile.c_str(), specabspath);
	const auto dir = dirname(string(specabspath));
	const auto& pipeline = spec->pnodes.at(pipename);
	const auto cmds = toPipeSpecShVector(dir, pipeline, pipename, false);
	bool first = true;
	os << "#!/bin/sh" << std::endl << std::endl;
	for (const auto& cmd: cmds) {
		if(!first) {
			os << " \\" << std::endl << " | ";
		}
		first = false;
		os << cmd.first;
	}
}

void writePipeSpecShDirOne(const vector<std::pair<string, string>> cmds, const string& pipename, const string& modesdir) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	// TODO: (modesdir / …) when we get <experimental/filesystem>
	for (size_t i = 0; i < cmds.size(); ++i) {
		string debug_suff = "";
		if(i < cmds.size() - 1) {
			debug_suff = std::to_string(i) + "-" + cmds[i].second;
		}
		const auto path = modesdir + "/" + pipename + debug_suff + ".mode";
		std::cout << path << std::endl;
		std::ofstream ofs;
		ofs.open(path, std::ofstream::out | std::ofstream::trunc);
		if(!ofs) {
			throw std::runtime_error("ERROR: Couldn't open " + path + " for writing! " + std::strerror(errno));
		}
		bool first = true;
		ofs << "#!/bin/sh" << std::endl << std::endl;
		for (size_t j = 0; j <= i; ++j) {
			const auto& cmd = cmds[j];
			if(!first) {
				ofs << " \\" << std::endl << " | ";
			}
			first = false;
			ofs << cmd.first;
		}
	}
}

void writePipeSpecShDir(const string& specfile, const string& modesdir) {
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	const auto spec = readPipeSpec(specfile);
	// const auto dir = std::experimental::filesystem::absolute(specfile).remove_filename(); // <experimental/filesystem>
        char specabspath[PATH_MAX];
	realpath(specfile.c_str(), specabspath);
	const auto dir = dirname(string(specabspath));
	for(const auto& p : spec->pnodes) {
		const auto& pipename = utf16conv.to_bytes(p.first);
		writePipeSpecShDirOne(toPipeSpecShVector(dir, p.second, p.first, false),
				      pipename,
				      modesdir);
		writePipeSpecShDirOne(toPipeSpecShVector(dir, p.second, p.first, true),
				      "trace-" + pipename,
				      modesdir);
	}
}

}
