//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "thirdparty/pugixml.hpp"
#include <string>
#include <vector>

struct Faust_Args {
    std::string classname;
    std::string processname;
    std::vector<std::string> incdirs;
};

int call_faust(const std::string &dspfile, pugi::xml_document &docmd, const Faust_Args &faustargs);
