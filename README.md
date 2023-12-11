
# Table of Contents

1.  [Description](#org4fcc88a)
2.  [Tools](#org56275a9)
3.  [Install from packages](#org9a6527d)
4.  [Simple build from git on Mac](#org96ee39f)
5.  [Prerequisites](#org6558baf)
    1.  [For just `divvun-suggest` and `divvun-blanktag`](#org52997e2)
    2.  [If you also want `divvun-checker`](#org7916c8e)
    3.  [If you also want `divvun-cgspell`](#orge31ee76)
    4.  [If you also want the Python library](#org494f256)
6.  [Building](#org1e607c1)
7.  [Command-line usage](#org32fa957)
    1.  [`divvun-suggest`](#orgb9f4fb0)
    2.  [`divvun-blanktag`](#org5e9008d)
    3.  [`divvun-cgspell`](#orgb2d7c60)
    4.  [`divvun-checker`](#orge4c3771)
8.  [JSON format](#orge91d3d5)
9.  [Pipespec XML](#org4724bf8)
    1.  [Mapping from XML preferences to UI](#org055a209)
10. [Writing grammar checkers](#org4e0bfc0)
    1.  [XML pipeline specification](#org6bea8af)
    2.  [Simple blanktag rules](#org19dbac0)
        1.  [Troubleshooting](#orgde13873)
    3.  [Simple grammarchecker.cg3 rules](#org0955ce1)
    4.  [More complex grammarchecker.cg3 rules (spanning over several words)](#org6b5053d)
    5.  [Deleting words](#org4d2c8ef)
    6.  [Alternative suggestions for complex errors altering different parts of the error](#orge26043f)
    7.  [Adding words](#orgb76a5c9)
    8.  [Adding literal word forms, altering existing wordforms](#orge23663a)
    9.  [Including spelling errors](#org26182db)
    10. [Summary of special tags and relations](#orgb55740d)
        1.  [Tags](#org67d1b58)
        2.  [Relations](#orga4550ed)
11. [Troubleshooting](#orge03c2e9)
12. [References / more documentation](#org7a81616)

[![img](https://www.travis-ci.com/divvun/libdivvun.svg?branch=master "Buoöd status")](https://app.travis-ci.com/github/divvun/libdivvun)

[![img](https://scan.coverity.com/projects/13737/badge.svg "Coverity static analysis")](https://scan.coverity.com/projects/divvun-divvun-gramcheck)


<a id="org4fcc88a"></a>

# Description

Libdivvun is a library for handling Finite-State Morphology and Constraint
Grammar based NLP tools in GiellaLT. The tools are used for tokenisation,
normalisation, grammar-checking and correction, and other NLP tasks.


<a id="org56275a9"></a>

# Tools

This repository contains the library `libdivvun`, and following
executables:

`divvun-checker`: This program opens a grammar checker pipeline XML
specification and lets you run grammar checking on strings. It can
also open zip files containing the XML and all required language
data. The C++ library `libdivvun` (headers installed to
`$PREFIX/include/divvun/*.hpp`) allows for the same features.

`divvun-suggest`: This program does FST lookup on forms specified as
Constraint Grammar format readings, and looks up error-tags in an XML
file with human-readable messages. It is meant to be used as a late
stage of a grammar checker pipeline.

The main output format of `divvun-suggest` is JSON, although it can
also simply annotate readings in CG stream format.

`divvun-cgspell`: This program spells unknown word forms from
Constraint Grammar format readings, adding them as new readings.

`divvun-blanktag`: This program takes an FST as argument, reads CG
input and uses the FST to add readings to cohorts that match on the
wordform surrounded by the preceding and following blanks. Use cases
include adding error tags that are dependent on spaces before/after,
or tagging the first word after a linebreak or certain formatting.

`divvun-phon`: This program takes FSAs, reads CG input and uses
the FSAs to add phonetic readings to each analysis lines

`divvun-normalise`: This program takes FSAs, reads CG input and uses
the FSAs to normalise the text towards TTS friendlier, read-out forms

There are also some helper programs for validating XML
(`divvun-validate-suggest`, `divvun-validate-pipespec`,
`divvun-gen-xmlschemas`) and for generating shell scripts from
pipeline specifications (`divvun-gen-sh`).


<a id="org9a6527d"></a>

# Install from packages

Tino Didriksen has kindly packaged this as both `.deb` and `.rpm`.

For `.deb` (Debian, Ubuntu and derivatives), add the repo and install
the package with:

    wget https://apertium.projectjj.com/apt/install-nightly.sh
    sudo bash install-nightly.sh
    sudo apt install divvun-gramcheck

For `.rpm` (openSUSE, Fedora, CentOS and derivatives), add the repo
and install the package with:

    wget https://apertium.projectjj.com/rpm/install-nightly.sh
    sudo bash install-nightly.sh
    sudo dnf install divvun-gramcheck

(See also the [deb build logs](https://apertium.projectjj.com/apt/logs/divvun-gramcheck/) and [rpm build status](https://build.opensuse.org/package/show/home:TinoDidriksen:nightly/divvun-gramcheck).)


<a id="org96ee39f"></a>

# Simple build from git on Mac

There is a script that will download prerequisites and compile and
install for Mac.

You don't even need to checkout this repository; just run:

    curl https://raw.githubusercontent.com/divvun/divvun-gramcheck/master/scripts/mac-build | bash

and enter your `sudo` password when it asks you to.

It does not (yet) enable `divvun-checker`, since that has yet more
dependencies. It assumes you've got `xmllint` installed.


<a id="org6558baf"></a>

# Prerequisites

*Note: Mac users probably just want to follow the steps in [Simple
build from git on Mac](#org96ee39f).*

This section lists the prerequisites for building the various tools in
this package.


<a id="org52997e2"></a>

## For just `divvun-suggest` and `divvun-blanktag`

-   gcc >=5.0.0 with libstdc++-5-dev (or similarly recent version of
    clang, with full C++11 support)
-   libxml2-utils (just for xmllint)
-   libhfst >=3.12.2
-   libpugixml >=1.7.2 (optional)

Tested with gcc-5.2.0, gcc-5.3.1 and clang-703.0.29. On Mac OS X, the
newest XCode includes a modern C++ compiler.

If you can't easily install libpugixml, you can run
[scripts/get-pugixml-and-build](scripts/get-pugixml-and-build) which will download libpugixml into this
directory, build that (with cmake) and configure this program to use
that library. Alternatively, you can run ./configure with
`--disable-xml` if you don't care about human-readable error messages.


<a id="org7916c8e"></a>

## If you also want `divvun-checker`

-   gcc >=5.0.0 with libstdc++-5-dev (or similarly recent version of
    clang, with full C++11 support)
-   libxml2-utils (just for xmllint)
-   libhfst >=3.12.2
-   libpugixml >=1.7.2
-   libcg3-dev >=1.1.2.12327
-   libarchive >=3.2.2-2

Tested with gcc-5.2.0, gcc-5.3.1 and clang-703.0.29. On Mac OS X, the
newest XCode includes a modern C++ compiler.

If you can't easily install libpugixml, you can run
[scripts/get-pugixml-and-build](scripts/get-pugixml-and-build) which will download libpugixml into this
directory, build that (with cmake) and configure this program to use
that library.

Now when building, pass `--enable-checker` to configure.


<a id="orge31ee76"></a>

## If you also want `divvun-cgspell`

-   hfst-ospell-dev >=0.4.5 (compiled with either libxml or tinyxml)

You can pass `--enable-cgspell` to `./configure` if you would like to
get an error if any of the `divvun-cgspell` dependencies are missing.


<a id="org494f256"></a>

## If you also want the Python library

The Python 3 library is used by the LibreOffice plugin. It will build
if it finds both of:

-   SWIG >=3.0 (install `python-swig` if you're using MacPorts)
-   Python >=3.0

You can pass `--enable-python-bindings` to `./configure` if you would
like to get an error if any of the `divvun-python-bindings`
dependencies are missing.


<a id="org1e607c1"></a>

# Building

    ./autogen.sh
    ./configure --enable-checker  # or just "./configure" if you don't need divvun-checker
    make
    make install # with sudo if you didn't specify a --prefix to ./configure

On OS X, you may have to do this:

    sudo port install pugixml
    export CC=clang CXX=clang++ "CXXFLAGS=-std=gnu++11 -stdlib=libc++"
    ./autogen.sh
    ./configure  LDFLAGS=-L/opt/local/lib --enable-checker
    make
    make install # with sudo if you didn't specify a --prefix to ./configure


<a id="org32fa957"></a>

# Command-line usage


<a id="orgb9f4fb0"></a>

## `divvun-suggest`

`divvun-suggest` takes two arguments: a generator FST (in HFST
optimised lookup format), and an error message XML file (see [the one
for North Saami](https://gtsvn.uit.no/langtech/trunk/langs/sme/tools/grammarcheckers/errors.xml) for an example), with input/output as stdin and
stdout:

    src/divvun-suggest --json generator-gt-norm.hfstol errors.xml < input > output

More typically, it'll be in a pipeline after various runs of `vislcg3`:

    echo words go here | hfst-tokenise --giella-cg tokeniser.pmhfst | … | vislcg3 … \
      | divvun-suggest --json generator-gt-norm.hfstol errors.xml


<a id="org5e9008d"></a>

## `divvun-blanktag`

`divvun-blanktag` takes one argument: an FST (in HFST
optimised lookup format), with input/output as stdin and
stdout:

    src/divvun-blanktag analyser.hfstol < input > output

More typically, it'll be in a pipeline after `cg-mwesplit`:

    echo words go here | hfst-tokenise … | … | cg-mwesplit \
      | src/divvun-blanktag analyser.hfstol < input > output

See the file [test/blanktag/blanktagger.xfst](test/blanktag/blanktagger.xfst) for an example blank
tagging FST (the other files in [test/blanktag](test/blanktag) show test input and
expected output, as well as how to compile the FST).


<a id="orgb2d7c60"></a>

## `divvun-cgspell`

`divvun-cgspell` takes options similar to [hfst-ospell](https://github.com/hfst/hfst-ospell/). You can give it
a single zhfst speller archive with the `-a` option, or specify
unzipped error model and lexicon with `-m` and `-l` options.

There are some options for limiting suggestions too, see
`--help`. You'll probably want to use `--limit` at least.

    src/divvun-cgspell --limit 5 se.zhfst < input > output

More typically, it'll be in a pipeline before/after various runs of `vislcg3`:

    echo words go here | hfst-tokenise --giella-cg tokeniser.pmhfst | … | vislcg3 … \
      | src/divvun-cgspell --limit 5 se.zhfst | vislcg3 …

You can also use it with unzipped, plain analyser and error model, e.g.

    src/divvun-cgspell --limit 5 -l analyser.hfstol -m errmodel.hfst < input > output


<a id="orge4c3771"></a>

## `divvun-checker`

`divvun-checker` is an example command-line interface to `libdivvun`.
You can use it to test a `pipespec.xml` or a zip archive containing
both the pipespec and langauge data, e.g.

    $ divvun-checker -a sme.zhfst
    Please specify a pipeline variant with the -n/--variant option. Available variants in archive:
    smegram
    smepunct
    
    $ echo ballat ođđa dieđuiguin | src/divvun-checker -a sme.zhfst -n smegram
    {"errs":[["dieđuiguin",12,22,"msyn-valency-loc-com","Wrong valency or something",["diehtukorrekt"]]],"text":"ballat ođđa dieđuiguin"}
    
    $ divvun-checker -s pipespec.xml
    Please specify a pipeline variant with the -n/--variant option. Available variants in pipespec:
    smegram
    smepunct
    
    $ echo ballat ođđa dieđuiguin | src/divvun-checker -s pipespec.xml -n smegram
    {"errs":[["dieđuiguin",12,22,"msyn-valency-loc-com","Wrong valency or something",["diehtukorrekt"]]],"text":"ballat ođđa dieđuiguin"}

When using the `-s/--spec pipespec.xml` option, relative paths in the
pipespec are relative to the current directory.

See the `test/` folder for an example of zipped archives.

See the [examples folder](examples/using-checker-lib-from-cpp) for how to link into libdivvun and use
it as a library, getting out either the JSON-formatted list of errors,
or a simple [data structure](src/checkertypes.hpp) that contains the same information as the
JSON. The next section describes the JSON format.


<a id="orge91d3d5"></a>

# JSON format

The JSON output of `divvun-suggest` is meant to be sent to a client
such as <https://github.com/divvun/divvun-webdemo>. The current format
is:

    {errs:[[str:string, beg:number, end:number, typ:string, exp:string, [rep:string]]], text:string}

The string `text` is the input, for sanity-checking.

The array-of-arrays `errs` has one array per error. Within each
error-array, `beg/end` are offsets in `text`, `typ` is the (internal)
error type, `exp` is the human-readable explanation, and each `rep` is
a possible suggestion for replacement of the text between `beg/end` in
`text`.

The index `beg` is inclusive, `end` exclusive, and both indices are
based on a UTF-16 encoding (which is what JavaScript uses, so e.g. the
emoji "🇳🇴" will increase the index of the following errors by 4).

Example output:

    {
      "errs": [
        [
          "badjel",
          37,
          43,
          "lex-bokte-not-badjel",
          "\"bokte\" iige \"badjel\"",
          [
            "bokte"
          ]
        ]
      ],
      "text": "🇳🇴sáddejuvvot báhpirat interneahta badjel.\n"
    }


<a id="org4724bf8"></a>

# Pipespec XML

The `divvun-checker` program and `libdivvun` (`divvun/checker.hpp`)
API has an XML format for specifying what programs go into the checker
pipelines, and metadata about the pipelines.

A `pipespec.xml` defines a set of grammar checker (or really any text
processing) pipelines.

There is a main language for each pipespec, but individual pipelines
may override with variants.

Each pipeline may define certain a set of mutually exclusive (radio
button) preferences, and if there's a `<suggest>` element referring to
an `errors.xml` file in the pipeline, error tags from that may be used
to populate UI's for hiding certain errors.


<a id="org055a209"></a>

## Mapping from XML preferences to UI

The mapping from preferences in the XML to a user interface should be
possible to do automatically, so the UI writer doesn't have to know
anything about what preferences the pipespec defines, but can just ask
the API for a list of preferences.

Preferences in the UI are either checkboxes [X] or radio buttons (\*).

We might for example get the following preferences UI:

    (*) Nordsamisk, Sverige
    ( ) Nordsamisk, Noreg
    …
    [X] Punctuation
        (*) punktum som tusenskilje
        ( ) mellomrom som tusenskilje
    [-] Grammar errors
        [X] ekteordsfeil
        [ ] syntaksfeil

Here, the available languages are scraped from the pipespec.xml
using `//pipeline/@language`.

A language is selected, so we create a Main Category of error types from

    pipespec.xml //[@language=Sverige|@language=""]/prefs/@type
    pipespec.xml //pipeline[@language=Sverige|@language=""]/@type
    errors.xml   //default/@type
    errors.xml   //error/@type

in this case giving the set { Punctuation, Grammar errors }.

One Main Category type is Punctuation; the radio buttons under
this main category are those defined in

    pipespec.xml //prefs[@type="Punctuation"]

The other Main Category type is Grammar errors; maybe we didn't have anything
in

    pipespec.xml //prefs[@type="Grammar errors"]

but there are checkboxes for errors that we can hide in

    errors.xml //defaults/default/title

It should be possible for the UI to hide which underlying
`<pipeline>`'s are chosen, and only show the preferences (picking a
pipeline based on preferences). But there is an edge case: Say the
pipe named smegram<sub>SE</sub> with language sme<sub>SE</sub> and main type "Grammar
errors" has a

    pref[@type="Punctuation"]

and there's another pipe named smepunct with main type "Punctuation".
Now, assuming we select the language sme<sub>SE</sub>, we'll never use smepunct,
since smegram defines error types that smepunct doesn't, but not the
other way around. Hopefully this is not a problem in practice.


<a id="org4e0bfc0"></a>

# Writing grammar checkers

Grammar checkers written for use in `libdivvun` consist of a
pipeline, at a high level typically looking like:

    tokenisation/morphology | multiword handling | disambiguation | error rules | generation

There are often other modules in here too, e.g. for adding spelling
suggestions, annotating valency, disambiguation and splitting
multiwords, or annotating surrounding whitespace.

Below we go through some of the different parts of the checker, using
the Giellatekno/Divvun North Sámi package (from
<https://victorio.uit.no/langtech/trunk/langs/sme/>) as an example.


<a id="org6bea8af"></a>

## XML pipeline specification

Each grammar checker needs a pipeline specification with all the
different modules and their data files in order. This is written in a
file `pipespec.xml`, which should follow the . Each such
file may have several `<pipeline>` elements (in case there are
alternative pipeline variants in your grammar checker package), with a
name and some metadata.

Here is the `pipespec.xml` for North Sámi:

    <pipespec language="se"
              developer="Divvun"
              copyright="…"
              version="0.42"
              contact="Divvun divvun@uit.no">
    
      <pipeline name="smegram"
                language="se"
                type="Grammar error">
        <tokenize><arg n="tokeniser-gramcheck-gt-desc.pmhfst"/></tokenize>
        <cg><arg n="valency.bin"/></cg>
        <cg><arg n="mwe-dis.bin"/></cg>
        <mwesplit/>
        <blanktag>
          <arg n="analyser-gt-whitespace.hfst"/>
        </blanktag>
        <cgspell>
          <arg n="errmodel.default.hfst"/>
          <arg n="acceptor.default.hfst"/>
        </cgspell>
        <cg><arg n="disambiguator.bin"/></cg>
        <cg><arg n="grammarchecker.bin"/></cg>
        <suggest>
          <arg n="generator-gt-norm.hfstol"/>
          <arg n="errors.xml"/>
        </suggest>
      </pipeline>
    
      <!-- other variants ommitted -->
    
    </pipespec>

This is what happens when text is sent through the `smegram` pipeline:

-   First, `<tokenize>` turns plain text into morphologically analysed
    tokens, using an FST compiled with `hfst-pmatch2fst`. These tokens
    may be ambiguous both wrt. to morphology and tokenisation.
-   Then, a `<cg>` module adds valency tags to readings, enriching the
    morphological analysis with context-sensitive information on
    argument structure.
-   Another `<cg>` module disambiguates cohorts that are ambiguous
    wrt. tokenisation, like multiwords and punctuation.
-   The `<mwesplit>` module splits now-disambiguated multiwords into
    separate tokens.
-   Then `<blanktag>` adds some tags to readings based on the
    surrounding whitespace (or other types of non-token
    blanks/formatting), using an FST which matches sequences of
    blank–wordform–blank.
-   The `<cgspell>` module adds readings with spelling suggestions to
    unknown words. The suggestions appear as wordform-tags.
-   Then a `<cg>` disambiguator, with rules modified a bit to let
    through more errors.
-   The main `<cg>` grammar checker module can now add error tags to
    readings, as well as new readings for generating suggestions, or
    special tags for deleting words or expanding underlines (and, as in
    the other `<cg>` modules, we can use the full range of CG features
    to add information that may be helpful in these tasks, such as
    dependency annotation and semantic role analysis)
-   Finally, `<suggest>` uses a generator FST to turn suggestion
    readings into forms, and an XML file of error descriptions to look
    up error messages from the tags added by the `<cg>` grammar checker
    module. These are used to output errors with suggestions, as well as
    readable error messages and the correct indices for underlines.

The program `divvun-gen-sh` in this package creates shell scripts from
the specification that you can use to test your grammar checker. In
the North Sámi checker, these should appear in
`tools/grammarcheckers/modes` when you type `make`, but you can also
create a single script for the above pipeline manually. If we do
`divvun-gen-sh -s pipespec.xml -n smegram > test.sh` with the above
XML, `test.sh` will contain something like

    #!/bin/sh
    
    hfst-tokenise -g '/home/me/gtsvn/langs/sme/tools/grammarcheckers/tokeniser-gramcheck-gt-desc.pmhfst' \
     | vislcg3 -g '/home/me/gtsvn/langs/sme/tools/grammarcheckers/valency.bin' \
     | vislcg3 -g '/home/me/gtsvn/langs/sme/tools/grammarcheckers/mwe-dis.bin' \
     | cg-mwesplit \
     | divvun-blanktag '/home/me/gtsvn/langs/sme/tools/grammarcheckers/analyser-gt-whitespace.hfst' \
     | divvun-cgspell '/home/me/gtsvn/langs/sme/tools/grammarcheckers/errmodel.default.hfst' '/home/me/gtsvn/langs/sme/tools/grammarcheckers/acceptor.default.hfst' \
     | vislcg3 -g '/home/me/gtsvn/langs/sme/tools/grammarcheckers/disambiguator.bin' \
     | vislcg3 -g '/home/me/gtsvn/langs/sme/tools/grammarcheckers/grammarchecker.bin' \
     | divvun-suggest '/home/me/gtsvn/langs/sme/tools/grammarcheckers/generator-gt-norm.hfstol' '/home/me/gtsvn/langs/sme/tools/grammarcheckers/errors.xml'

We can send words through this pipeline with `echo "words here" | sh
test.sh`.

Using `divvun-gen-sh` manually like this is good for checking if
you've written your XML correctly, but if you're working within the
Giellatekno projects, you'll typically just type `make` and use the
scripts that end up in `modes`.

Do

    $ ls modes

in `tools/grammarcheckers` to list all the scripts. These contain not
just the full pipeline (for every `<pipeline>` in the XML), but also
"debug" versions that are chopped off at various points (with numbers
to show how far they go), as well as versions with CG rule tracing
turned on. So if you'd like to check up until disambiguation, before
the `grammarchecker` CG, you'd do something like

    echo "words go here" | sh modes/trace-smegram6-disam.mode


<a id="org19dbac0"></a>

## Simple blanktag rules

The `divvun-blanktag` program will tag a cohort with a user-specified
tag if it finds a match on the input wordform and its surrounding
blanks.

The wordform includes the CG wordform delimiters

    "<

and

    >"

The surrounding blanks do *not* include the start-of-line colon. The
rule file is an FST with blank-wordform-blank on the input side, and
the tag on the output-side, typically written in the XFST regex
format.

As an example (with spaces changed to underscores for readability), if
the `input.txt` contains

    :_
    "<)>"
            ")" RPAREN @EOP
            ")" RPAREN @EMO
    "<.>"
            "." PUNCT
    :\n
    :\n

then `divvun-blanktag` will try to match twice, first on the string

    _"<)>"

then on the string

    "<.>"\n\n

If the rule file `ws.regex` (here in XFST regex format) contains

    [ {_} {"<)>"} ?* ]:[%<spaceBeforeParenEnd%>]

then we will get

    hfst-regexp2fst --disjunct ws.regex | hfst-fst2fst -O -o ws.hfst
    divvun-blanktag ws.hfst < input.txt

    :_
    "<)>"
    	")" RPAREN @EOP <spaceBeforeParenEnd>
    	")" RPAREN @EMO <spaceBeforeParenEnd>
    "<.>"
    	"." PUNCT
    :\n
    :\n

The matching goes from the start of the preceding blank, across the
wordform and to the end of the following blank. In this input, there
was no blank following the right-parens, so the rule could just as
well have been

    [ {_} {"<)>"} ]:[%<spaceBeforeParenEnd%>]

– this would **require** that there is no following blank. However, if
you want it to also match the input

    :_
    "<)>"
            ")" RPAREN @EOP
            ")" RPAREN @EMO
    :\n

then you need the final match-all `?*`.


<a id="orgde13873"></a>

### Troubleshooting

If you get

    terminate called after throwing an instance of 'FunctionNotImplementedException'                                                [68/660]
    Aborted (core dumped)

check how you compiled the HFST file – it should be in unweighted HFST
optimized lookup format.


<a id="org0955ce1"></a>

## Simple grammarchecker.cg3 rules

In our North Sámi checker, the

    <cg><arg n="grammarchecker.bin"></cg>

file is created with from the source file
`$GTHOME/langs/sme/tools/grammarcheckers/grammarchecker.cg3`, which
adds error tags and suggestion-readings.

A simple rule looks like:

    ADD:msyn-hallan (&real-hallan) TARGET (Imprt Pl1 Dial/-KJ) IF (0 HALLA-PASS-V) (NEGATE *1 ("!")) ;

This simply adds an error tag `real-hallan` to words that are tagged
`Imprt Pl1 Dial/-KJ` and match the context conditions after the
`IF`. This will put an underline under the word in the user
interface. If `errors.xml` in the same folder has a nice description
for that tag, the user will see that description in the user
interface.

We can add a suggestion as well with a `COPY` rule:

    COPY:msyn-hallan (Inf &SUGGEST) EXCEPT (Imprt Pl1 Dial/-KJ) TARGET (Imprt Pl1 Dial/-KJ &real-hallan) ;

This creates a new reading where the tags `Imprt Pl1 Dial/-KJ` have
been changed into `Inf &SUGGEST` (and other tags are unchanged). The
`&SUGGEST` tag is necessary to get `divvun-suggest` (the `<suggest>`
module) to try to generate a form from that reading. It is smart
enough to skip things like weights, tracing and syntax tags when
trying to suggest, but all morphological tags need to be correct and
in the right order for generation to work.


<a id="org6b5053d"></a>

## More complex grammarchecker.cg3 rules (spanning over several words)

The error is considered to have a central part and one or more less central parts.
The less central parts need the `&COERROR` tag (without numbering) if all parts are to be underlined as one error. The words can be adjacent. If there are words in between that are not part of the error, they are still underlined.

In the first line of the following example only "soaitá" and "boađán" are part of the error and are underlined. However, if "mun" ("I") is inserted in between then it is also underlined. 

    Mun soaitá boađán. `Maybe I come.'
    Soaitá mun boađán. `Maybe I come.'

You can refer to the word form of the "central" cohort of the error
using `$1` in errors.xml, e.g.

    <description xml:lang="en">The word "$1" seems to be in the wrong case.</description>

You can refer to the word form of the first correction / suggestion
using `€1` in errors.xml, e.g.

    <description xml:lang="en">Please don't write "$1", it sounds much nicer if you use "€1" instead.</description>

---

To refer to other words, you add relations named `$2` and so on:

    ADDRELATION ($2) Ess TO (*-1 ("dego" &syn-not-dego) BARRIER Ess);

which you can refer to just like with `$1`:

    <title xml:lang="en">there should not be "$2" if "$1" is essive</title>


<a id="org4d2c8ef"></a>

## Deleting words

If you want to delete a word from a CG rule, it's typically enough to
add an error tag to the word you want to *keep*, and add a relation
`DELETE1` to the word you want to delete. This will make an underline
that covers both those words, where the suggestion is the same string
without the target of the `DELETE1` relation.

    ADD (&one-word-too-many) KeepThisWord;
    ADDRELATION (DELETE1) DeleteThisWord TO (-1 KeepThisWord);

The cohort matching `KeepThisWord` is now the central one of the
error, so if e.g. `errors.xml` uses templates like

    Don't use "$2" before "$1"

the word form of `KeepThisWord` will be substituted for `$1`.

You may delete more words from the same suggestion using `DELETE2`
etc.

In South Sámi sometimes phrasal verbs are used (due to a literal translation from Scandinavian languages) 
where the verb alone already expresses the concept. 
This is the case for "tjuedtjelh bæjjese" (verb + adverb) meaning "stand.up up". 
With the following rule we first annotate the error and then delete the adverb "bæjjese".

    ADD (&syn-delete-adv-phrasal-verb) TARGET (V) IF (0 ("tjuedtjielidh") OR ("fulkedh")) (*0 ("bæjjese") BARRIER (*) - Pcle) ;
    ADD (&syn-delete-adv-phrasal-verb) TARGET (Adv) IF (0 ("bæjjese")) (*0 ("tjuedtjielidh") OR ("fulkedh") BARRIER (*) - Pcle) ;
    
    ADDRELATION (DELETE1) (V &syn-delete-adv-phrasal-verb) TO (*0 (Adv &syn-delete-adv-phrasal-verb) BARRIER (*) - Pcle) ;


<a id="orge26043f"></a>

## Alternative suggestions for complex errors altering different parts of the error

However, sometimes you have several possible suggestions on the same
word, which might partially overlap. For example, you might also have

    ADD (&other-error) KeepThisWord;
    COPY (Nom &other-error) EXCEPT (Acc) TARGET (&other-error) ;

where you want to keep the suggestions for `&one-word-too-many`
separate from the suggestions for `&other-error`.
Each suggestion for an error with several parts requires that all parts receive the same error tag and the less central parts receive the &COERROR tag. In the following case there are two possible corrections.

    Soaitá boađán.
    
    Soaittán boahtit.
    Kánske boađán.

The alternative corrections have different central parts of the error.
In the first case both parts are changed. The first part ("soaitá" 3.Sg.) is changed to "soaittán" (1.Sg.) based on the person and number of the second word ("boađán" 1.Sg.). Subsequently "boađán" (1.Sg.) is changed to "boahtit" (infinitive).
Alternatively, only the first part is changed and the second part remains unchanged. In this case we can change the "soaitá" (3.Sg.) to the adverb "kánske".
This requires that the error receives two error tags for the parts that are two be changed, i.e. `&msyn-kánske` to "soaitá" and `&msyn-fin_fin-fin_inf to both "soaitá" and "boađán"`.

    Dåaktere veanhta dïhte aktem aajla-hirremem åtneme, dan åvteste tjarke svæjmadi jïh 
    {ij mujhti} satne lij vaedtsieminie skuvleste gåatan.

In this sentence for sma there are two alternative suggestions:

-   one regarding the second cohort only &#x2013; `mujhti > mujhtieh`
-   the other one regarding both cohorts &#x2013; `ij mujhti > idtji mujhtieh`

Unfortunately, relations in CG are cohort-to-cohort, not
reading-to-reading. The workaround is to put the error tag also on the
relation target (the word to be deleted), along with the `&COERROR` tag
to say that this is not the central word of the error:

    ADD (&COERROR &one-word-too-many) DeleteThisWord IF (1 KeepThisWord);

Without `&COERROR`, this would be treated as a separate error, while
without `&one-word-too-many`, we would suggest deleting this word in
the suggestions for `&other-error` too.

Similarly, the `&SUGGEST` reading for the `&other-error` retains the
`&other-error` tag, which avoids generating that suggestion for the
`&one-word-too-many` error.

A real example of this in the North Sámi checker is the error
`dego lávvomuorran`, which has the suggestions `lávvomuorran` or `dego
lávvomuorra` – one error type alters just the form, and one removes
just the preceding word.


<a id="orgb76a5c9"></a>

## Adding words

To add a word as a suggestion, use `ADDCOHORT`, adding both reading
tags (lemma, part-of-speech etc.), a wordform tag (including a space)
and `&ADDED` to mark it as something that didn't appear in the input;
and then a `LEFT` or `RIGHT` relation from the central cohort of the
error to the added word:

    ADD (&msyn-valency-go-not-fs) IF (…);
    ADDCOHORT ("<go >" "go" CS &ADDED &msyn-valency-go-not-fs) BEFORE &msyn-valency-go-not-fs;
    ADDRELATION (LEFT) (&msyn-valency-go-not-fs) TO (-1 (&ADDED)) ;

Because of `&ADDED`, `divvun-suggest` will treat this as a non-central
word of the error (just like with the `&COERROR` tag).

Note that we include the space in the wordform, and we put it at the
*end* of the wordform. This is because vislcg3 always adds new cohorts
*after* the blank of the preceding cohort. In some cases, e.g. with
punctuation, we want the new cohort to come before the blank of the
preceding cohort; then we use the tag `&ADDED-BEFORE-BLANK`, and
`divvun-suggest` will ensure it ends up in the right place, e.g.:

    ADD:punct-rihkku (&punct-rihkku) TARGET (Inf) IF (-1 Inf LINK -1 COMMA LINK -1 Inf …);
    ADDCOHORT:punct-rihkku ("<,>" "," CLB &ADDED-BEFORE-BLANK &punct-rihkku) BEFORE (V &punct-rihkku) IF …;
    ADDRELATION (LEFT) (&punct-rihkku) TO (-1 (&ADDED-BEFORE-BLANK)) ;

will give a suggestion that covers the space before the infinitive.


<a id="orge23663a"></a>

## Adding literal word forms, altering existing wordforms

Say you want to tag missing spaces after punctuation. You've added a
rule like

    [ ?* {"<,>"} ]:[%<NoSpaceAfterPunctMark>]

to your whitespace-analyser.regex (used by `divvun-blanktag`) and the
input to the grammarchecker CG is now

    "<3>"
            "3" Num Arab Sg Loc Attr @HNOUN
            "3" Num Arab Sg Nom @HNOUN
            "3" Num Arab Sg Ill Attr @HNOUN
    "<,>"
            "," CLB <NoSpaceAfterPunctMark>
    "<ja>"
            "ja" CC @CNP

Then you can first of all turn that blanktag tag into an error tag with

    ADD (&no-space-after-punct-mark) (<NoSpaceAfterPunctMark>);

Now, we could just suggest a wordform on the comma and call it a day:

    COPY ("<, >" &SUGGESTWF) TARGET ("," &no-space-after-punct-mark) ;

but that will

1.  only work on commas, and
2.  be a tiny underline, hard to click for users

Instead, let's extend the underline to the following word:

    ADD (&no-space-after-punct-mark &COERROR)
        TARGET (*)
        IF (-1 (<NoSpaceAfterPunctMark>))
        ;
    ADDRELATION (RIGHT) (&no-space-after-punct-mark)
        TO (1 (&COERROR) LINK 0 (&no-space-after-punct-mark))
        ;

Every error needs a "central" cohort, even if it involves several
words; this is important in order to get error messages to show
correctly. It doesn't matter which one you pick, as long as you pick
one. Here we've picked the comma to be central, while the following
word is a "link" word. In the above rules,

-   The `&COERROR` tag says that the following word is just a part of the
    error, not the central cohort.
-   The `RIGHT` relation says that this is one big error, not two
    separate ones.

Then we can add a suggestion that puts a space between the forms:

    COPY:no-space-after-punct ("<$1 $2>"v &SUGGESTWF)
        TARGET ("<(.*)>"r &no-space-after-punct-mark)
        IF (1 ("<(.*)>"r))
           (NOT 0 (&COERROR))
        ;

This uses vislcg3's [variable strings / varstrings](http://beta.visl.sdu.dk/cg3/chunked/tags.html#variable-strings) to create the
wordform suggestion from two regular expression strings matching the
wordforms of the two cohorts. Note that the `$1` and `$2` refer to the
first and second regex groups as they appear in the rule, not as they
appear in the sentence. If the rule referred to the preceding word
with `(-1 ("<(.*)>"r))`, you'd probably want the suggestion to be `<$2
$1>`.

We also make sure we don't put a suggestion-tag on the `&COERROR` cohort
(here the word `<ja>`), which would lead to some strange suggestions
since it is already part of the suggestion-tag on the comma `<,>`
cohort.

Now the output is

    "<3>"
            "3" Num Arab Sg Loc Attr @HNOUN
            "3" Num Arab Sg Nom @HNOUN
            "3" Num Arab Sg Ill Attr @HNOUN
    "<,>"
            "," CLB <NoSpaceAfterPunctMark> &no-space-after-punct-mark ID:3 R:RIGHT:4
            "," CLB <NoSpaceAfterPunctMark> "<, ja>" &no-space-after-punct-mark &SUGGESTWF ID:3 R:RIGHT:4
    "<ja>"
            "ja" CC @CNP &COERROR &no-space-after-punct-mark ID:4

or, in JSON format:

    {
      "errs": [
        [
          ",ja",
          4,
          7,
          "no-space-after-punct-mark",
          "no-space-after-punct-mark",
          [
            ", ja"
          ]
        ]
      ],
      "text": "ja 3,ja"
    }

This looks pretty good, except the error tag is listed twice. The
second entry is actually supposed to contain a human-readable error
message, but `errors.xml` contains no entry for this tag. Let's add it:

    <error id="no-space-after-punct-mark">
      <header>
        <title xml:lang="en">Missing space</title>
      </header>
      <body>
        <description xml:lang="en">There is no space after the punctuation mark "$1"</description>
      </body>
    </error>

(In Giellatekno's setup, this goes in `errors.source.xml`, which is
compiled to `errors.xml`.)

Now we get:

    {
      "errs": [
        [
          ",ja",
          4,
          7,
          "no-space-after-punct-mark",
          "Missing space",
          [
            ", ja"
          ]
        ]
      ],
      "text": "ja 3,ja"
    }

which should end up as a nice error message, suggestion and
underline in the UI.


<a id="org26182db"></a>

## Including spelling errors

To use the `divvun-cgspell` module, you need a spelling acceptor
(dictionary) FST and error model FST. These are the same format as the
files used by [hfst-ospell](https://github.com/hfst/hfst-ospell/). The speller isn't yet used to handle
real-word errors, just adding suggestions to unknowns.

The `divvun-cgspell` module should go before disambiguation in the
pipeline, so the disambiguator can pick the best suggestion in
context.

The module adds the tag `<spelled>` to any suggestions. The speller
module itself doesn't take any context into account, that's for later
steps to handle. As an example, you might have this unknown word as
input to the speller module:

    "<coffe>"
            "coffe" ?

To which the output from the speller might be

    "<coffes>"
            "coffes" ?
            "coffee" N Sg <W:37.3018> <WA:17.3018> <spelled> "<coffee>"
            "coffee" N Pl <W:37.3018> <WA:17.3018> <spelled> "<coffees>"
            "coffer" N Pl <W:39.1010> <WA:17.3018> <spelled> "<coffers>"
            "Coffey" N Prop <W:40.0000> <WA:18.1800> <spelled> "<Coffey>"

The *form* to be suggested is included as a "wordform-tag" at the very
end of each reading from the speller.

Now the later CG stages can use the context of this cohort to pick
more relevant suggestions (e.g. if the word to the left was "a", we
might want to `REMOVE` the plurals or even `SELECT` the singulars). We
could also `ADD/MAP` some relevant tags or relations.

Note that the readings added by the speller don't include any error
tags (tags with `&` in front). To turn these readings into error
underlines and actually show the suggestions, add a rule like

    ADD (&typo &SUGGESTWF) (<spelled>) ;

to the grammar checker CG. The reason we add `&SUGGESTWF` and not
`&SUGGEST` is that we're using the wordform-tag directly as the
suggestion, and not sending each analysis through the generator (as
`&SUGGEST` would do). So if, after disambiguation and grammarchecker
CG's, we had

    "<coffes>"
            "coffee" N Pl <W:37.3018> <WA:17.3018> <spelled> "<coffees>" &typo &SUGGESTWF
            "coffer" N Pl <W:39.1010> <WA:17.3018> <spelled> "<coffers>" &typo &SUGGESTWF

then the final `divvun-suggest` step would simply use the contents of
the tags

    "<coffers>"
    "<coffees>"

to create the suggestion-list, without bothering with generating from

    "coffee" N Pl
    "coffer" N Pl

This makes the system more robust in case the speller lexicon differs
from the regular suggestion generator, and saves some duplicate work.


<a id="orgb55740d"></a>

## Summary of special tags and relations

CG lets you define all kinds of new tags and relation-names and within
CG you are free to make your own conventions as to what they mean.
However, in the Divvun grammar checker system, certain CG tags and
relation-names have special meanings to the rest of the system. Below
is a summary of the special tags/relations and their uses. In
addition, note that all divvun error tags need to start with the `&`
character, but apart from that are free to name errors as long as they
don't conflict with the below special tags.


<a id="org67d1b58"></a>

### Tags

-   `&SUGGEST` on a reading means that `divvun-suggest` should try to
    generate this reading into a form for suggestions, using the
    generator FST. See [Simple grammarchecker.cg3 rules](#org0955ce1).
-   `&SUGGESTWF` on a reading means that `divvun-suggest` should use the
    reading's wordform-tag (e.g. a tag like
    
        "<Cupertino>"
    
    on a *reading*, not as the first line of a cohort) as a suggestion.
    See [Including spelling errors](#org26182db).
-   `<spelled>` is added by `divvun-cgspell` to any suggestions it
    makes. See [Including spelling errors](#org26182db).
-   `&COERROR` makes a cohort non-central in that error, see [Deleting words](#org4d2c8ef).
    (For backwards-compatibility with older grammar checker files,
    `&LINK` also has the same meaning as `&COERROR`, though this is
    deprecated and at some point `&LINK` may be fully removed)
-   `&ADDED` means this cohort was added (typically with `ADDCOHORT`)
    and should be a part of the suggestion for the error. It will appear
    after the blank of the preceding cohort, and will not be the central
    cohort of the error. See [Adding words](#orgb76a5c9).
-   `&ADDED-BEFORE-BLANK` is like `&ADDED`, except that it appears
    before the blank of the preceding cohort.
-   Any other tag starting with `&` is an error type tag,
    e.g. `&real-hallan` or `&punct-rihkku`, defined by the CG rule
    author. It should also appear in `errors.xml` (without the initial
    `&`) with a human-readable error message.


<a id="orga4550ed"></a>

### Relations

-   `LEFT` and `RIGHT` are used to extend the underline to added
    cohorts; see [Adding words](#orgb76a5c9) and
    .
-   `DELETE1` (and `DELETE2` etc.) are used to say that a word in the
    context of this error should be deleted in the suggestion. See [Deleting words](#org4d2c8ef).
-   `$2` (and `$3` etc.) are used to make wordforms in the context
    available to human-readable error messages in `errors.xml`. Note
    that `$1` is always the wordform of the *central* cohort of the
    error (so don't add `$1` as a relation). See [Simple grammarchecker.cg3 rules](#org0955ce1).


<a id="orge03c2e9"></a>

# Troubleshooting

If you get

    terminate called after throwing an instance of 'std::regex_error'
      what():  regex_error

then your C++ compiler is too old. See [Prerequisites](#org6558baf).

If you get

    configure: error: 'g++  -std=c++11 -Wall -I/usr/include/hfst/ @GLIB_CFLAGS@  -I/usr/include/ ' does not accept ISO C++11

then you may be at the receiving end of
<https://github.com/hfst/hfst/issues/366>. A workaround is to edit
`/usr/lib64/pkgconfig/hfst.pc` and simply delete the string
`@GLIB_CFLAGS@`.


<a id="org7a81616"></a>

# References / more documentation

The architecture of systems using libdivvun is described in

-   Wiechetek, L., Moshagen, S., & Unhammer, K. B. (2019, February).
    [Seeing more than whitespace—Tokenisation and disambiguation in a North Sámi grammar checker](https://aclanthology.org/W19-6007.pdf). In
    *Proceedings of the 3rd Workshop on the Use of Computational Methods in the Study of Endangered Languages Volume 1 (Papers)*
    (pp. 46-55).

