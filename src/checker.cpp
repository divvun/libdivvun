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

#include "checker.hpp"

namespace gtd {

void dbg(const std::string& label, std::stringstream& output) {
	const auto p = output.tellg();
	std::cerr << label << output.str();
	output.seekg(p, output.beg);
}

void
TokenizeCmd::setup (bool verbose)
{
	container->set_verbose(verbose);
	container->set_single_codepoint_tokenization(!tokenize_multichar);
	settings.output_format = hfst_ol_tokenize::giellacg;
	settings.print_weights = true;
	settings.print_all = true;
	settings.dedupe = true;
	settings.max_weight_classes = 2;
}
TokenizeCmd::TokenizeCmd (std::istream& instream, bool verbose)
	: container(new hfst_ol::PmatchContainer(instream))
{
	setup(verbose);
}
TokenizeCmd::TokenizeCmd (std::ifstream instream, bool verbose)
	: container(new hfst_ol::PmatchContainer(instream))
	// : TokenizeCmd(std::istream(instream), verbose) // why can't I just do this?
{
	setup(verbose);
}
TokenizeCmd::TokenizeCmd (const std::string& path, bool verbose)
	: TokenizeCmd(std::ifstream(path.c_str(), std::ifstream::binary), verbose)
{
}
void TokenizeCmd::run(std::stringstream& input, std::stringstream& output) const
{
	hfst_ol_tokenize::process_input(*container, input, output, settings);
}


MweSplitCmd::MweSplitCmd (bool verbose)
	: grammar(MweSplitCmd::load())
	, applicator(new CG3::MweSplitApplicator(grammar->ux_stderr))
{
	if(!grammar){
		throw std::runtime_error("ERROR: Couldn't load the empty MweSplit grammar");
	}
	applicator->setGrammar(&*grammar);
}
void MweSplitCmd::run(std::stringstream& input, std::stringstream& output) const
{
	applicator->runGrammarOnText(input, output);
}

CG3::Grammar *MweSplitCmd::load() {
	CG3::Grammar *grammar = CG3::MweSplitApplicator::emptyGrammar(
		u_finit(stderr, uloc_getDefault(), ucnv_getDefaultName()),
		u_finit(stdout, uloc_getDefault(), ucnv_getDefaultName()));
	return grammar;
}



CGCmd::CGCmd (const char* buff, const size_t size, bool verbose)
	: grammar(CGCmd::load_buffer(buff, size))
	, applicator(new CG3::StreamApplicator(grammar->ux_stderr))
{
	if(!grammar){
		throw std::runtime_error("ERROR: Couldn't load CG grammar");
	}
	applicator->setGrammar(&*grammar);
}
CGCmd::CGCmd (const std::string& path, bool verbose)
	: grammar(CGCmd::load_file(path.c_str()))
	, applicator(new CG3::StreamApplicator(grammar->ux_stderr))
{
	if(!grammar){
		throw std::runtime_error(("ERROR: Couldn't load CG grammar " + path).c_str());
	}
	applicator->setGrammar(&*grammar);
}

void CGCmd::run(std::stringstream& input, std::stringstream& output) const
{
	applicator->runGrammarOnText(input, output);
}

CG3::Grammar *CGCmd::load_buffer(const char* input, const size_t size) {
	// TODO
	// if (!input.read(&CG3::cbuffers[0][0], 4)) {
	// 	std::cerr << "CG3 Error: Error reading first 4 bytes from grammar!\n" << std::endl;
	// 	return 0;
	// }

	CG3::Grammar *grammar = new CG3::Grammar;
	// grammar->ux_stdin = u_finit(stdin, uloc_getDefault(), ucnv_getDefaultName());
	grammar->ux_stderr = u_finit(stderr, uloc_getDefault(), ucnv_getDefaultName());
	grammar->ux_stdout = u_finit(stdout, uloc_getDefault(), ucnv_getDefaultName());

	std::unique_ptr<CG3::IGrammarParser> parser;
	// TODO: Support BinaryGrammar too?
	// if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
	// 	std::cerr << "CG3 Info: Binary grammar detected.\n" << std::endl;
	// 	parser.reset(new CG3::BinaryGrammar(*grammar, grammar->ux_stderr));
	// }
	// else {
	// 	parser.reset(new CG3::TextualParser(*grammar, grammar->ux_stderr));
	// }
	parser.reset(new CG3::TextualParser(*grammar, grammar->ux_stderr));

	if (parser->parse_grammar(input, size)) {
		std::cerr << "CG3 Error: Grammar could not be parsed!\n" << std::endl;
		return 0;
	}

	grammar->reindex();

	return grammar;
}


CG3::Grammar *CGCmd::load_file(const char *filename) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		// TODO: Throw here instead of return 0, since that segfaults
		std::cerr << "CG3 Error: Error opening " << filename << " for reading!\n" << std::endl;
		return 0;
	}
	if (!input.read(&CG3::cbuffers[0][0], 4)) {
		std::cerr << "CG3 Error: Error reading first 4 bytes from grammar!\n" << std::endl;
		return 0;
	}
	input.close();

	CG3::Grammar *grammar = new CG3::Grammar;
	// grammar->ux_stdin = u_finit(stdin, uloc_getDefault(), ucnv_getDefaultName());
	grammar->ux_stderr = u_finit(stderr, uloc_getDefault(), ucnv_getDefaultName());
	grammar->ux_stdout = u_finit(stdout, uloc_getDefault(), ucnv_getDefaultName());

	std::unique_ptr<CG3::IGrammarParser> parser;

	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		std::cerr << "CG3 Info: Binary grammar detected.\n" << std::endl;
		parser.reset(new CG3::BinaryGrammar(*grammar, grammar->ux_stderr));
	}
	else {
		parser.reset(new CG3::TextualParser(*grammar, grammar->ux_stderr));
	}
	if (parser->parse_grammar(filename, uloc_getDefault(), ucnv_getDefaultName())) {
		std::cerr << "CG3 Error: Grammar could not be parsed!\n" << std::endl;
		return 0;
	}

	grammar->reindex();

	return grammar;
}



SuggestCmd::SuggestCmd (const hfst::HfstTransducer* generator, gtd::msgmap msgs, bool verbose)
	: generator(generator), msgs(msgs)
{
	if (!generator) {
		throw std::runtime_error("ERROR: Suggest command couldn't read generator");
	}
	if (msgs.empty()) {
		throw std::runtime_error("ERROR: Suggest command couldn't read messages xml");
	}
}
SuggestCmd::SuggestCmd (const std::string& gen_path, const std::string& msg_path, bool verbose)
	: generator(gtd::readTransducer(gen_path.c_str()))
	, msgs(gtd::readMessages(msg_path))
{
	if (!generator) {
		throw std::runtime_error("ERROR: Suggest command couldn't read transducer " + gen_path);
	}
	if (msgs.empty()) {
		throw std::runtime_error("ERROR: Suggest command couldn't read messages xml " + msg_path);
	}
}
void SuggestCmd::run(std::stringstream& input, std::stringstream& output) const
{
	gtd::run(input, output, *generator, msgs, true);
}



const size_t AR_BLOCK_SIZE = 10240;

template<typename Ret>
using ArEntryHandler = std::function<Ret(const std::string& ar_path, const void* buff, const size_t size)>;

template<typename Ret>
Ret archiveExtract(const std::string& ar_path,
		   archive *ar,
		   const std::string& entry_pathname,
		   ArEntryHandler<Ret>procFile)
{
	struct archive_entry* entry = 0;
	for (int rr = archive_read_next_header(ar, &entry);
	     rr != ARCHIVE_EOF;
	     rr = archive_read_next_header(ar, &entry))
	{
		if (rr != ARCHIVE_OK)
		{
			throw std::runtime_error("Archive not OK");
		}
		std::string filename(archive_entry_pathname(entry));
		if (filename == entry_pathname) {
			size_t fullsize = 0;
			const struct stat* st = archive_entry_stat(entry);
			size_t buffsize = st->st_size;
			if (buffsize == 0) {
				std::cerr << archive_error_string(ar) << std::endl;
				throw std::runtime_error("ERROR: Got a zero length archive entry for " + filename);
			}
			std::string buff(buffsize, 0);
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
Ret readArchiveExtract(const std::string& ar_path,
		       const std::string& entry_pathname,
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
		std::stringstream msg;
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


Pipeline::Pipeline(const std::unique_ptr<ArPipeSpec>& ar_spec, const std::u16string& pipename, bool v) : verbose(v)
{
	auto& spec = ar_spec->spec;
	const pugi::xml_node& pipeline = spec->pnodes.at(pipename);
	if (!cg3_init(stdin, stdout, stderr)) {
		// TODO: Move into a lib-general init function? Or can I call this safely once per CGCmd?
		throw std::runtime_error("ERROR: Couldn't initialise ICU for vislcg3!");
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const pugi::xml_node& cmd: pipeline.children()) {
		const auto& name = utf16conv.from_bytes(cmd.name());
		std::vector<std::string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.push_back(argn);
		}
		if(name == u"tokenise" || name == u"tokenize") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <tokenize> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<TokenizeCmd*> f = [v] (const std::string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return new TokenizeCmd(is, v);
			};
			TokenizeCmd* s = readArchiveExtract(ar_spec->ar_path, args[0], f);
			cmds.emplace_back(s);
		}
		else if(name == u"cg") {
			if(args.size() != 1) {
				throw std::runtime_error("Wrong number of arguments to <cg> command (expected 1), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<CGCmd*> f = [v] (const std::string& ar_path, const void* buff, const size_t size) {
				return new CGCmd((char*)buff, size, v);
			};
			CGCmd* s = readArchiveExtract(ar_spec->ar_path, args[0], f);
			cmds.emplace_back(s);
		}
		else if(name == u"mwesplit") {
			if(args.size() != 0) {
				throw std::runtime_error("Wrong number of arguments to <mwesplit> command (expected 0), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"suggest") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <suggest> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			ArEntryHandler<const hfst::HfstTransducer*> procGen = [] (const std::string& ar_path, const void* buff, const size_t size) {
				OneShotReadBuf osrb((char*)buff, size);
				std::istream is(&osrb);
				return gtd::readTransducer(is);
			};
			ArEntryHandler<gtd::msgmap> procMsgs = [] (const std::string& ar_path, const void* buff, const size_t size) {
				return gtd::readMessages((char*)buff, size);
			};
			auto* s = new SuggestCmd(readArchiveExtract(ar_spec->ar_path, args[0], procGen),
						 readArchiveExtract(ar_spec->ar_path, args[1], procMsgs),
						 verbose);
			cmds.emplace_back(s);
		}
		else if(name == u"sh") {
			throw std::runtime_error("<sh> command not implemented yet!");
			// const auto& prog = utf16conv.from_bytes(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			std::cerr << "\033[0;35mTODO: implement <prefs>\033[0m" << std::endl;
		}
		else {
			throw std::runtime_error("Unknown command '" + utf16conv.to_bytes(name) + "'");
		}
	}
}

Pipeline::Pipeline(const std::unique_ptr<PipeSpec>& spec, const std::u16string& pipename, bool v) : verbose(v)
{
	const pugi::xml_node& pipeline = spec->pnodes.at(pipename);
	if (!cg3_init(stdin, stdout, stderr)) {
		// TODO: Move into a lib-general init function? Or can I call this safely once per CGCmd?
		throw std::runtime_error("ERROR: Couldn't initialise ICU for vislcg3!");
	}
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	for (const pugi::xml_node& cmd: pipeline.children()) {
		const auto& name = utf16conv.from_bytes(cmd.name());
		std::vector<std::string> args;
		for (const pugi::xml_node& arg: cmd.children("arg")) {
			const auto& argn = arg.attribute("n").value();
			args.push_back(argn);
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
		else if(name == u"mwesplit") {
			if(args.size() != 0) {
				throw std::runtime_error("Wrong number of arguments to <mwesplit> command (expected 0), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new MweSplitCmd(verbose));
		}
		else if(name == u"suggest") {
			if(args.size() != 2) {
				throw std::runtime_error("Wrong number of arguments to <suggest> command (expected 2), at byte offset " + std::to_string(cmd.offset_debug()));
			}
			cmds.emplace_back(new SuggestCmd(args[0], args[1], verbose));
		}
		else if(name == u"sh") {
			throw std::runtime_error("<sh> command not implemented yet!");
			// const auto& prog = utf16conv.from_bytes(cmd.attribute("prog").value());
			// cmds.emplace_back(new ShCmd(prog, args, verbose));
		}
		else if(name == u"prefs") {
			std::cerr << "\033[0;35mTODO: implement <prefs>\033[0m" << std::endl;
		}
		else {
			throw std::runtime_error("Unknown command '" + utf16conv.to_bytes(name) + "'");
		}
	}
}
void Pipeline::proc(std::stringstream& input, std::stringstream& output) {
	std::stringstream cur_in;
	std::stringstream cur_out(input.str());
	for(const auto& cmd: cmds) {
		cur_in.swap(cur_out);
		cur_out.clear();
		cur_out.str(std::string());
		cmd->run(cur_in, cur_out);
		// if(DEBUG) { dbg("cur_out after run", cur_out); }
	}
	output << cur_out.str();
}
cg3_status Pipeline::cg3_init(FILE *in, FILE *out, FILE *err) {
	UErrorCode status = U_ZERO_ERROR;
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		fprintf(err, "CG3 Error: Cannot initialize ICU. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");

	uloc_setDefault("en_US_POSIX", &status);
	if (U_FAILURE(status)) {
		fprintf(err, "CG3 Error: Failed to set default locale. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	return CG3_SUCCESS;
}

std::unique_ptr<PipeSpec> readPipeSpec(const std::string& file) {
	std::unique_ptr<PipeSpec> spec = std::unique_ptr<PipeSpec>(new PipeSpec);
	pugi::xml_parse_result result = spec->doc.load_file(file.c_str());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
	if (result) {
		for (pugi::xml_node pipeline: spec->doc.child("pipespec").children("pipeline")) {
			const std::u16string& pipename = utf16conv.from_bytes(pipeline.attribute("name").value());
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

std::unique_ptr<ArPipeSpec> readArchiveSpec(const std::string& ar_path) {
	ArEntryHandler<std::unique_ptr<ArPipeSpec>> f = [] (const std::string& ar_path, const void* buff, const size_t size) {
		std::unique_ptr<ArPipeSpec> ar_spec = std::unique_ptr<ArPipeSpec>(new ArPipeSpec(ar_path));
		pugi::xml_parse_result result = ar_spec->spec->doc.load_buffer((pugi::char_t*)buff, size);
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
		if (result) {
			for (pugi::xml_node pipeline: ar_spec->spec->doc.child("pipespec").children("pipeline")) {
				const std::u16string& pipename = utf16conv.from_bytes(pipeline.attribute("name").value());
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

}
