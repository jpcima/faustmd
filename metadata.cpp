//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "metadata.h"
#include "messages.h"
#include <iostream>
#include <cstdlib>
#include <cassert>

static const std::string cstrlit(gsl::cstring_span text);
static const std::string mangle(gsl::cstring_span name);

static bool is_decint_string(gsl::cstring_span str)
{
    if (!str.empty() && str[0] == '-')
        str = str.subspan(1);

    if (str.empty())
        return false;

    for (char c : str) {
        if (c < '0' || c > '9')
            return false;
    }

    return true;
}

static int extract_widget(pugi::xml_node node, bool is_active, Metadata &md);

int extract_metadata(const pugi::xml_document &doc, Metadata &md)
{
    pugi::xml_node root = doc.child("faust");

    md.name = root.child_value("name");
    md.author = root.child_value("author");
    md.copyright = root.child_value("copyright");
    md.license = root.child_value("license");
    md.version = root.child_value("version");
    md.classname = root.child_value("classname");
    md.inputs = std::stoi(root.child_value("inputs"));
    md.outputs = std::stoi(root.child_value("outputs"));

    for (pugi::xml_node meta : root.children("meta")) {
        std::string key = meta.attribute("key").value();
        std::string value = meta.child_value();
        md.metadata.emplace_back(key, value);
    }

    for (pugi::xml_node node : root.child("ui").child("activewidgets").children("widget")) {
        if (extract_widget(node, true, md) == -1)
            return -1;
    }

    for (pugi::xml_node node : root.child("ui").child("passivewidgets").children("widget")) {
        if (extract_widget(node, false, md) == -1)
            return -1;
    }

    return 0;
}

static int extract_widget(pugi::xml_node node, bool is_active, Metadata &md)
{
    Metadata::Widget w;
    w.type = Metadata::Widget::type_from_name(node.attribute("type").value());
    if (w.type == (Metadata::Widget::Type)-1)
        return -1;

    w.id = std::stoi(node.attribute("id").value());
    w.label = node.child_value("label");
    w.var = node.child_value("varname");

    if (is_active && (w.type == Metadata::Widget::Type::HSlider ||
                      w.type == Metadata::Widget::Type::VSlider ||
                      w.type == Metadata::Widget::Type::NEntry)) {
        w.init = std::stof(node.child_value("init"));
        w.min = std::stof(node.child_value("min"));
        w.max = std::stof(node.child_value("max"));
        w.step = std::stof(node.child_value("step"));
    }
    else if (is_active && (w.type == Metadata::Widget::Type::Button ||
                           w.type == Metadata::Widget::Type::CheckBox)) {
        w.init = 0;
        w.min = 0;
        w.max = 1;
        w.step = 1;
    }
    else if (!is_active && (w.type == Metadata::Widget::Type::VBarGraph ||
                            w.type == Metadata::Widget::Type::HBarGraph)) {
        w.min = std::stof(node.child_value("min"));
        w.max = std::stof(node.child_value("max"));
    }
    else
        return -1;

    for (pugi::xml_node meta : node.children("meta")) {
        std::string key = meta.attribute("key").value();
        std::string value = meta.child_value();
        if (is_decint_string(key) && value.empty())
            continue;
        w.metadata.emplace_back(key, value);

        if (key == "unit")
            w.unit = value;
        else if (key == "scale") {
            w.scale = Metadata::Widget::scale_from_name(value);
            if (w.scale == (Metadata::Widget::Scale)-1) {
                warns() << "Unrecognized scale type `" << value << "`\n";
                w.scale = Metadata::Widget::Scale::Linear;
            }
        }
        else if (key == "tooltip")
            w.tooltip = value;
    }

    (is_active ? md.active : md.passive).push_back(std::move(w));
    return 0;
}

static void dump_widgets(std::ostream &o, const std::vector<Metadata::Widget> &widgets, bool is_active);

void dump_metadata(std::ostream &o, Metadata &md)
{
    std::string ident_classname = md.classname;
    std::string ident_meta = ident_classname + "_meta";

    o << "#ifndef __" << ident_meta << "_H__" "\n";
    o << "#define __" << ident_meta << "_H__" "\n";

    o << "\n";

    o << "#include <cstddef>" "\n";

    o << "\n";

    o << "#ifndef FAUSTMETA" << "\n";
    o << "#define FAUSTMETA " << ident_meta << "\n";
    o << "#endif" "\n";

    o << "\n";

    o << "#ifdef __GNUC__" "\n";
    o << "#define FMSTATIC __attribute__((unused)) static" "\n";
    o << "#else" "\n";
    o << "#define FMSTATIC static" "\n";
    o << "#endif" "\n";

    o << "\n";

    o << "namespace " << ident_meta << " {" "\n";

    o << "\t" "struct metadata_t { const char *key; const char *value; };" "\n";
    o << "\t" "enum class active_type_t { button, checkbox, vslider, hslider, nentry };\n";
    o << "\t" "enum class passive_type_t { vbargraph, hbargraph };\n";
    o << "\t" "enum class scale_t { linear, log, exp };\n";

    o << "\n";

    o << "\t" "FMSTATIC constexpr char name[] = " << cstrlit(md.name) << ";" "\n";
    o << "\t" "FMSTATIC constexpr char author[] = " << cstrlit(md.author) << ";" "\n";
    o << "\t" "FMSTATIC constexpr char copyright[] = " << cstrlit(md.copyright) << ";" "\n";
    o << "\t" "FMSTATIC constexpr char license[] = " << cstrlit(md.license) << ";" "\n";
    o << "\t" "FMSTATIC constexpr char version[] = " << cstrlit(md.version) << ";" "\n";
    o << "\t" "FMSTATIC constexpr char classname[] = " << cstrlit(md.classname) << ";" "\n";
    o << "\t" "FMSTATIC constexpr unsigned inputs = " << md.inputs << ";" "\n";
    o << "\t" "FMSTATIC constexpr unsigned outputs = " << md.outputs << ";" "\n";
    o << "\t" "FMSTATIC constexpr unsigned actives = " << md.active.size() << ";" "\n";
    o << "\t" "FMSTATIC constexpr unsigned passives = " << md.passive.size() << ";" "\n";

    o << "\n";

    const char *separator;

    o << "\t" "FMSTATIC const metadata_t metadata[] = {";
    separator = "";
    for (const std::pair<std::string, std::string> &md : md.metadata) {
        o << separator << "{" << cstrlit(md.first) << ", " << cstrlit(md.second) << "}";
        separator = ", ";
    }
    o << "};" "\n";

    o << "\n";

    dump_widgets(o, md.active, true);

    o << "\n";

    dump_widgets(o, md.passive, false);

    o << "}\n";

    o << "\n";

    o << "#undef FMSTATIC";

    o << "\n";

    o << "#endif // __" << ident_meta << "_H__" "\n";
}

static void dump_widgets(std::ostream &o, const std::vector<Metadata::Widget> &widgets, bool is_active)
{
    const char *separator;
    const char *prefix = is_active ? "active" : "passive";

    o << "\t" "FMSTATIC constexpr " << prefix << "_type_t " << prefix << "_type[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << "" << prefix << "_type_t::" << w.type; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr int " << prefix << "_id[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.id; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC const char *const " << prefix << "_label[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << cstrlit(w.label); separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC const char *const " << prefix << "_symbol[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << cstrlit(mangle(w.label)); separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC const std::size_t " << prefix << "_offsets[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << "(size_t)&((FAUSTCLASS *)0)->" << w.var; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr FAUSTFLOAT " << prefix << "_init[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.init; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr FAUSTFLOAT " << prefix << "_min[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.min; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr FAUSTFLOAT " << prefix << "_max[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.max; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr FAUSTFLOAT " << prefix << "_step[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.step; separator = ", "; }
    o << "};" "\n";

    o << "\n";

    o << "\t" "FMSTATIC const char *const " << prefix << "_unit[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << cstrlit(w.unit); separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr scale_t " << prefix << "_scale[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << "scale_t::" << w.scale; separator = ", "; }
    o << "};" "\n";

    o << "\t" "FMSTATIC const char *const " << prefix << "_tooltip[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << cstrlit(w.tooltip); separator = ", "; }
    o << "};" "\n";

    o << "\n";

    o << "\t" "FMSTATIC const metadata_t *const " << prefix << "_metadata[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets) {
        o << separator << "(metadata_t[]){";
        const char *separator2 = "";
        for (const std::pair<std::string, std::string> &md : w.metadata) {
            o << separator2 << "{" << cstrlit(md.first) << ", " << cstrlit(md.second) << "}";
            separator2 = ", ";
        }
        o << "}";
        separator = ", ";
    }
    o << "};" "\n";

    o << "\t" "FMSTATIC constexpr std::size_t " << prefix << "_metadata_size[] = {";
    separator = "";
    for (const Metadata::Widget &w : widgets)
        { o << separator << w.metadata.size(); separator = ", "; }
    o << "};" "\n";

    o << "\n";

    if (is_active) {
        o << "\t" "FMSTATIC inline void " << prefix << "_set(FAUSTCLASS &x, unsigned idx, FAUSTFLOAT v) {"
          << " *(FAUSTFLOAT *)((char *)&x + " << prefix << "_offsets[idx]) = v; "
          << "}" "\n";
    }
    o << "\t" "FMSTATIC inline FAUSTFLOAT " << prefix << "_get(const FAUSTCLASS &x, unsigned idx) {"
      << " return *(const FAUSTFLOAT *)((const char *)&x + " << prefix << "_offsets[idx]); "
      << "}" "\n";

    o << "\n";

    if (is_active) {
        for (const Metadata::Widget &w : widgets) {
            o << "\t" << "FMSTATIC inline void " << mangle("set_" + w.label) << "(FAUSTCLASS &x, FAUSTFLOAT v) {"
              << " x." << w.var << " = v; "
              << "}" "\n";
        }
    }
    for (const Metadata::Widget &w : widgets) {
        o << "\t" << "FMSTATIC inline FAUSTFLOAT " << mangle("get_" + w.label) << "(const FAUSTCLASS &x) {"
          << " return x." << w.var << "; "
          << "}" "\n";
    }
}

static const std::string cstrlit(gsl::cstring_span text)
{
    std::string lit;
    lit.push_back('u');
    lit.push_back('8');
    lit.push_back('"');

    for (char c : text) {
        switch (c) {
        case '\a': lit.push_back('\\'); lit.push_back('a'); break;
        case '\b': lit.push_back('\\'); lit.push_back('b'); break;
        case '\t': lit.push_back('\\'); lit.push_back('t'); break;
        case '\n': lit.push_back('\\'); lit.push_back('n'); break;
        case '\v': lit.push_back('\\'); lit.push_back('v'); break;
        case '\f': lit.push_back('\\'); lit.push_back('f'); break;
        case '\r': lit.push_back('\\'); lit.push_back('r'); break;
        case '"': case '\\': lit.push_back('\\'); lit.push_back(c); break;
        default: lit.push_back(c); break;
        }
    }

    lit.push_back('"');
    return lit;
}

static const std::string mangle(gsl::cstring_span name)
{
    std::string id;
    size_t n = name.size();
    id.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        char c = name[i];
        bool al = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool num = c >= '0' && c <= '9';
        c = (!al && (!num || i == 0)) ? '_' : c;
        id.push_back(c);
    }

    return id;
}

Metadata::Widget::Type Metadata::Widget::type_from_name(gsl::cstring_span name)
{
    Metadata::Widget::Type t = (Metadata::Widget::Type)-1;

    if (name == "button")
        t = Type::Button;
    else if (name == "checkbox")
        t = Type::CheckBox;
    else if (name == "vslider")
        t = Type::VSlider;
    else if (name == "hslider")
        t = Type::HSlider;
    else if (name == "nentry")
        t = Type::NEntry;
    else if (name == "vbargraph")
        t = Type::VBarGraph;
    else if (name == "hbargraph")
        t = Type::HBarGraph;

    return t;
}

Metadata::Widget::Scale Metadata::Widget::scale_from_name(gsl::cstring_span name)
{
    Metadata::Widget::Scale s = (Metadata::Widget::Scale)-1;

    if (name == "log")
        s = Scale::Log;
    else if (name == "exp")
        s = Scale::Exp;

    return s;
}

std::ostream &operator<<(std::ostream &o, Metadata::Widget::Type t)
{
    switch (t) {
    case Metadata::Widget::Type::Button: return o << "button";
    case Metadata::Widget::Type::CheckBox: return o << "checkbox";
    case Metadata::Widget::Type::VSlider: return o << "vslider";
    case Metadata::Widget::Type::HSlider: return o << "hslider";
    case Metadata::Widget::Type::NEntry: return o << "nentry";
    case Metadata::Widget::Type::VBarGraph: return o << "vbargraph";
    case Metadata::Widget::Type::HBarGraph: return o << "hbargraph";
    }

    assert(false);
    return o;
}

std::ostream &operator<<(std::ostream &o, Metadata::Widget::Scale s)
{
    switch (s) {
    case Metadata::Widget::Scale::Linear: return o << "linear";
    case Metadata::Widget::Scale::Log: return o << "log";
    case Metadata::Widget::Scale::Exp: return o << "exp";
    }

    assert(false);
    return o;
}
