//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "metadata.h"
#include "call_faust.h"
#include "messages.h"
#include "thirdparty/pugixml.hpp"
#include "thirdparty/gsl-lite.hpp"
#include <string>
#include <vector>
#include <iostream>

struct Cmd_Args {
    std::string dspfile;
    Faust_Args faustargs;
};

static void display_usage();
static int do_cmdline(Cmd_Args &cmd, int argc, char *argv[]);
static int process_document(std::ostream &out, const pugi::xml_document &doc);

int main(int argc, char *argv[])
{
    Cmd_Args cmd;
    if (do_cmdline(cmd, argc, argv) == -1) {
        display_usage();
        return 1;
    }

    pugi::xml_document doc;
    if (call_faust(cmd.dspfile, doc, cmd.faustargs) == -1) {
        errs() << "The faust command has failed.\n";
        return 1;
    }

    if (process_document(std::cout, doc) == -1) {
        errs() << "The document could not be processed.\n";
        return 1;
    }

    return 0;
}

static void display_usage()
{
    std::cerr << "Usage: faustmd [-I path]* [-cn name] [-pn name] <file.dsp>\n";
}

static int do_cmdline(Cmd_Args &cmd, int argc, char *argv[])
{
    bool moreflags = true;
    unsigned extraindex = 0;

    for (int i = 1; i < argc; ++i) {
        gsl::string_span arg = argv[i];

        if (moreflags && arg == "--")
            moreflags = false;
        else if (moreflags && arg == "-I") {
            if (++i == argc) {
                errs() << "The flag `-I` requires an argument.\n";
                return -1;
            }
            cmd.faustargs.incdirs.push_back(argv[i]);
        }
        else if (moreflags && arg == "-cn") {
            if (++i == argc) {
                errs() << "The flag `-cn` requires an argument.\n";
                return -1;
            }
            cmd.faustargs.classname = argv[i];
        }
        else if (moreflags && arg == "-pn") {
            if (++i == argc) {
                errs() << "The flag `-pn` requires an argument.\n";
                return -1;
            }
            cmd.faustargs.processname = argv[i];
        }
        else if (moreflags && !arg.empty() && arg[0] == '-') {
            errs() << "Unrecognized flag `" << arg << "`\n";
            return -1;
        }
        else {
            switch (extraindex) {
            case 0:
                cmd.dspfile = gsl::to_string(arg);
                break;
            default:
                errs() << "Unrecognized positional argument `" << arg << "`\n";
                return -1;
            }
            ++extraindex;
        }
    }

    if (extraindex != 1) {
        errs() << "There must be exactly one positional argument.\n";
        return -1;
    }

    return 0;
}

static int process_document(std::ostream &out, const pugi::xml_document &doc)
{
    if (false)
        doc.save(std::cerr);

    Metadata md;
    if (extract_metadata(doc, md) == -1) {
        errs() << "Could not extract the faust metadata.\n";
        return -1;
    }

    dump_metadata(out, md);
    return 0;
}
