/*
 * Copyright (C) 2017-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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
		throw std::runtime_error("libdivvun: ERROR: Couldn't load CG grammar");
	}
}
CGCmd::CGCmd (const string& path, bool verbose)
	: grammar(cg3_grammar_load(path.c_str()))
	, applicator(cg3_applicator_create(grammar.get()))
{
	if(!grammar){
		throw std::runtime_error(("libdivvun: ERROR: Couldn't load CG grammar " + path).c_str());
	}
}
void CGCmd::run(stringstream& input, stringstream& output) const
{
	cg3_run_grammar_on_text(applicator.get(), (std_istream*)&input, (std_ostream*)&output);
}

#ifdef HAVE_CGSPELL
CGSpellCmd::CGSpellCmd (hfst_ospell::Transducer* errmodel, hfst_ospell::Transducer* acceptor, bool verbose)
	: speller(new Speller(errmodel, acceptor, verbose, max_analysis_weight, max_weight, real_word, limit, beam, time_cutoff))
{
	if (!acceptor) {
		throw std::runtime_error("libdivvun: ERROR: CGSpell command couldn't read acceptor");
	}
	if (!errmodel) {
		throw std::runtime_error("libdivvun: ERROR: CGSpell command couldn't read errmodel");
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
#endif

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

SuggestCmd::SuggestCmd (const hfst::HfstTransducer* generator, divvun::MsgMap msgs, const string& locale, bool verbose)
	: suggest(new Suggest(generator, msgs, locale, verbose))
{
}
SuggestCmd::SuggestCmd (const string& gen_path, const string& msg_path, const string& locale, bool verbose)
	: suggest(new Suggest(gen_path, msg_path, locale, verbose))
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
void SuggestCmd::setIgnores(const std::set<ErrId>& ignores)
{
	suggest->setIgnores(ignores);
}
const MsgMap& SuggestCmd::getMsgs()
{
	return suggest->msgs;
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
		throw std::runtime_error("libdivvun: ERROR: Couldn't initialise ICU for vislcg3!");
	}
	string locale = spec->language;
	for (const pugi::xml_node& cmd: spec->pnodes.at(pipename).children()) {
		const auto& name = fromUtf8(cmd.name());
		std::unordered_map<string, string> args;
		for (const pugi::xml_node& arg: cmd.children()) {
			args[arg.name()] = arg.attribute("n").value();
		}
		validatePipespecCmd(cmd, args);
		if(name == u"tokenise" || name == u"tokenize") {
			ArEntryHandler<TokenizeCmd*> f = [verbose] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return new TokenizeCmd(is, verbose);
			};
			TokenizeCmd* s = readArchiveExtract(ar_spec->ar_path, args["tokenizer"], f);
			cmds.emplace_back(s);
		}
		else if(name == u"cg") {
			ArEntryHandler<CGCmd*> f = [verbose] (const string& ar_path, const void* buff, const size_t size) {
				return new CGCmd((char*)buff, size, verbose);
			};
			CGCmd* s = readArchiveExtract(ar_spec->ar_path, args["grammar"], f);
			cmds.emplace_back(s);
		}
		else if(name == u"cgspell") {
#ifdef HAVE_CGSPELL
			ArEntryHandler<hfst_ospell::Transducer*> f = [] (const string& ar_path, const void* buff, const size_t size) {
				return new hfst_ospell::Transducer((char*)buff);
			};
			auto* s = new CGSpellCmd(readArchiveExtract(ar_spec->ar_path, args["errmodel"], f),
						 readArchiveExtract(ar_spec->ar_path, args["lexicon"], f),
						 verbose);
			cmds.emplace_back(s);
#else
			throw std::runtime_error("libdivvun: ERROR: Tried to run pipeline with cgspell, but was compiled without cgspell support!");
#endif
		}
		else if(name == u"mwesplit") {
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"blanktag") {
			ArEntryHandler<const hfst::HfstTransducer*> f = [] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return readTransducer(is);
			};
			auto* s = new BlanktagCmd(readArchiveExtract(ar_spec->ar_path, args["blanktagger"], f),
						  verbose);
			cmds.emplace_back(s);
		}
		else if(name == u"suggest") {
			ArEntryHandler<const hfst::HfstTransducer*> procGen = [] (const string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return readTransducer(is);
			};
			ArEntryHandler<divvun::MsgMap> procMsgs = [] (const string& ar_path, const void* buff, const size_t size) {
				return Suggest::readMessages((char*)buff, size);
			};
			auto* s = new SuggestCmd(readArchiveExtract(ar_spec->ar_path, args["generator"], procGen),
						 readArchiveExtract(ar_spec->ar_path, args["messages"], procMsgs),
						 locale,
						 verbose);
			cmds.emplace_back(s);
			mergePrefsFromMsgs(prefs, s->getMsgs());
			suggestcmd = s;
		}
		else if(name == u"sh") {
			// const auto& prog = fromUtf8(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			parsePrefs(prefs, cmd);
		}
		else {
			throw std::runtime_error("libdivvun: ERROR: Unknown <pipeline> element " + toUtf8(name));
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
		throw std::runtime_error("libdivvun: ERROR: Couldn't initialise ICU for vislcg3!");
	}
	string locale = spec->language;
	for (const pugi::xml_node& cmd: spec->pnodes.at(pipename).children()) {
		const auto& name = fromUtf8(cmd.name());
		std::unordered_map<string, string> args;
		for (const pugi::xml_node& arg: cmd.children()) {
			args[arg.name()] = arg.attribute("n").value();
		}
		validatePipespecCmd(cmd, args);
		if(name == u"tokenise" || name == u"tokenize") {
			cmds.emplace_back(new TokenizeCmd(args["tokenizer"], verbose));
		}
		else if(name == u"cg") {
			cmds.emplace_back(new CGCmd(args["grammar"], verbose));
		}
		else if(name == u"cgspell") {
#ifdef HAVE_CGSPELL
			cmds.emplace_back(new CGSpellCmd(args["errmodel"],
							 args["lexicon"],
							 verbose));
#else
			throw std::runtime_error("libdivvun: ERROR: Tried to run pipeline with cgspell, but was compiled without cgspell support!");
#endif
		}
		else if(name == u"mwesplit") {
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"blanktag") {
			cmds.emplace_back(new BlanktagCmd(args["blanktagger"], verbose));
		}
		else if(name == u"suggest") {
			auto *s = new SuggestCmd(args["generator"],
						 args["messages"],
						 locale,
						 verbose);
			cmds.emplace_back(s);
			mergePrefsFromMsgs(prefs, s->getMsgs());
			suggestcmd = s;
		}
		else if(name == u"sh") {
			// const auto& prog = fromUtf8(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			parsePrefs(prefs, cmd);
		}
		else {
			throw std::runtime_error("libdivvun: ERROR: Unknown <pipeline> element " + toUtf8(name));
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
void Pipeline::setIgnores(const std::set<ErrId>& ignores) {
	if(suggestcmd != nullptr) {
		suggestcmd->setIgnores(ignores);
	}
	else if(!ignores.empty()) {
		throw std::runtime_error("libdivvun: ERROR: Can't set ignores when last command of pipeline is not a SuggestCmd");
	}
}

}
