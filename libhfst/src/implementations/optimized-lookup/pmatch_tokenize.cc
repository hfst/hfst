// Copyright (c) 2016-2019 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "pmatch_tokenize.h"

#include <unicode/brkiter.h>
#include <unicode/unistr.h>
#include <iomanip> // Required for std::fixed, std::setprecision
#include <limits>  // Required for std::numeric_limits
#include <set>     // Required for std::set
#include <sstream> // Required for std::ostringstream
#include <vector>  // Required for std::vector
#include <algorithm> // Required for std::copy, std::sort
#include <iterator> // Required for std::ostream_iterator, std::back_inserter
#include <cmath> // Required for std::isinf, std::isnan

static UErrorCode characterBoundaryStatus = U_ZERO_ERROR;
static icu::BreakIterator *characterBoundary
    = icu::BreakIterator::createCharacterInstance(NULL,
                                                  characterBoundaryStatus);

namespace hfst_ol_tokenize
{

using std::pair;
using std::string;
using std::vector;

using hfst_ol::Location;
using hfst_ol::LocationVector;
using hfst_ol::LocationVectorVector;

static const string subreading_separator = "#";
static const string wtag = "W"; // TODO: cg-conv has an argument --wtag, allow
                                // changing here as well?
static bool IS_CG_TAG_MODIFIER_WARNED
    = false; // Only warn once on skipping modifier letters

// Helper function to format weight as a CG numeric tag string (e.g., <W:0.5>)
// Uses the same logic as previously in print_cg_subreading.
static std::string
format_weight_as_CG_numeric_tag(hfst_ol::Weight const &weight)
{
    if (std::isinf(weight))
    {
        return "<" + wtag + ":inf>";
    }
    if (std::isnan(weight))
    {
        return "<" + wtag + ":nan>";
    }

    std::ostringstream w_oss;
    w_oss << std::fixed << std::setprecision(9) << weight;
    std::string rounded = w_oss.str();

    // CG-style trimming logic
    bool seendot = false;
    bool inzeroes = true;
    size_t firstzero_len = rounded.length(); // Represents the length of the string to keep

    for (size_t i = rounded.length(); i > 0; --i)
    {
        if (inzeroes && rounded[i - 1] == '0')
        {
            firstzero_len = i; // The new length would be up to, but not including, current char if it's a zero from the end
        }
        else
        {
            inzeroes = false;
        }
        if (rounded[i - 1] == '.')
        {
            seendot = true;
            break;
        }
    }

    if (seendot)
    {
        rounded = rounded.substr(0, firstzero_len);
    }

    return "<" + wtag + ":" + rounded + ">";
}

void
print_escaping_backslashes(std::string const &str, std::ostream &outstream)
{
    // TODO: inline?
    size_t i = 0, j = 0;
    while ((j = str.find("\\", i)) != std::string::npos)
    {
        outstream << str.substr(i, j - i) << "\\\\";
        i = j + 1;
    }
    outstream << str.substr(i, j - i);
}

// Helper function to escape strings for JSON output
void
print_json_escaped(std::string const &str, std::ostream &outstream)
{
    for (char const &c : str)
    {
        switch (c)
        {
        case '"':
            outstream << "\\\"";
            break;
        case '\\':
            outstream << "\\\\";
            break;
        case '\b':
            outstream << "\\b";
            break;
        case '\f':
            outstream << "\\f";
            break;
        case '\n':
            outstream << "\\n";
            break;
        case '\r':
            outstream << "\\r";
            break;
        case '\t':
            outstream << "\\t";
            break;
        default:
            // TODO: Handle other control characters if necessary
            outstream << c;
            break;
        }
    }
}

void
print_no_output(std::string const &input, std::ostream &outstream,
                const TokenizeSettings &s)
{
    if (s.output_format == tokenize || s.output_format == space_separated)
    {
        outstream << input;
    }
    else if (s.output_format == xerox)
    {
        outstream << input << "\t" << input << "+?";
    }
    else if (s.output_format == cg || s.output_format == giellacg)
    {
        outstream << "\"<";
        print_escaping_backslashes(input, outstream);
        outstream << ">\"" << std::endl << "\t\"";
        print_escaping_backslashes(input, outstream);
        outstream << "\" ?";
    }
    else if (s.output_format == jsonl)
    {
        outstream << "{\"w\":\"";
        print_json_escaped(input, outstream);
        outstream << "\"}";
    }
    //    std::cerr << "from print_no_output\n";
    outstream << "\n\n";
}

void
print_escaping_newlines(std::string const &str, std::ostream &outstream)
{
    // TODO: inline?
    size_t i = 0, j = 0;
    while ((j = str.find_first_of("\n\r", i)) != std::string::npos)
    {
        outstream << str.substr(i, j - i);
        if (str[j] == '\n')
        {
            outstream << "\\n";
        }
        else if (str[j] == '\r')
        {
            outstream << "\\r";
        }
        i = j + 1;
    }
    outstream << str.substr(i, j - i);
}

void
print_nonmatching_sequence(std::string const &str, std::ostream &outstream,
                           const TokenizeSettings &s)
{
    if (s.output_format == tokenize || s.output_format == space_separated)
    {
        outstream << str;
    }
    else if (s.output_format == xerox)
    {
        outstream << str << "\t" << str << "+?";
    }
    else if (s.output_format == cg)
    {
        outstream << "\"<";
        print_escaping_backslashes(str, outstream);
        outstream << ">\"" << std::endl << "\t\"";
        print_escaping_backslashes(str, outstream);
        outstream << "\" ?";
    }
    else if (s.output_format == giellacg)
    {
        outstream << ":";
        print_escaping_newlines(str, outstream);
    }
    else if (s.output_format == visl)
    {
        outstream << str;
    }
    else if (s.output_format == conllu)
    {
        outstream << str;
    }
    else if (s.output_format == finnpos)
    {
        outstream << str << "\t_\t_\t_\t_";
    }
    else if (s.output_format == jsonl)
    {
        outstream << "{\"t\":\"";
        print_json_escaped(str, outstream);
        outstream << "\"}";
    }
    //    std::cerr << "from print_nonmatching_sequence\n";
    outstream << "\n";
}

bool
location_compare(const Location &lhs, const Location &rhs)
{
    if (lhs.weight == rhs.weight)
    {
        if (lhs.tag == rhs.tag)
        {
            if (lhs.start == rhs.start)
            {
                if (lhs.length == rhs.length)
                {
                    return lhs.output < rhs.output;
                }
                else
                {
                    return lhs.length < rhs.length;
                }
            }
            else
            {
                return lhs.start < rhs.start;
            }
        }
        else
        {
            return lhs.tag < rhs.tag;
        }
    }
    else
    {
        return lhs.weight < rhs.weight;
    }
}

bool
location_compare_ignoring_weights(const Location &lhs, const Location &rhs)
{
    if (lhs.tag == rhs.tag)
    {
        if (lhs.start == rhs.start)
        {
            if (lhs.length == rhs.length)
            {
                return lhs.output < rhs.output;
            }
            else
            {
                return lhs.length < rhs.length;
            }
        }
        else
        {
            return lhs.start < rhs.start;
        }
    }
    else
    {
        return lhs.tag < rhs.tag;
    }
}

bool
location_compare_using_only_weights(const Location &lhs, const Location &rhs)
{
    return lhs.weight < rhs.weight;
}

const LocationVector
dedupe_locations(LocationVector const &locations, const TokenizeSettings &s)
{
    if (!s.dedupe)
    {
        return locations;
    }
    if (s.print_weights)
    {
        std::set<Location, bool (*)(const Location &lhs, const Location &rhs)>
            ls(&location_compare);
        ls.insert(locations.begin(), locations.end());
        LocationVector uniq;
        std::copy(ls.begin(), ls.end(), std::back_inserter(uniq));
        return uniq;
    }
    else
    {
        std::set<Location, bool (*)(const Location &lhs, const Location &rhs)>
            ls(&location_compare_ignoring_weights);
        ls.insert(locations.begin(), locations.end());
        LocationVector uniq;
        std::copy(ls.begin(), ls.end(), std::back_inserter(uniq));
        std::sort(uniq.begin(), uniq.end(),
                  location_compare_using_only_weights);
        return uniq;
    }
}
/**
 * Keep only the max_weight_classes best weight classes
 */
const LocationVector
keep_n_best_weight(LocationVector const &locations, const TokenizeSettings &s)
{
    if (locations.size() <= s.max_weight_classes)
    {
        // We know we won't trim anything, no need to copy the vector:
        return locations;
    }
    int classes_found = -1;
    hfst_ol::Weight last_weight_class = 0.0;
    LocationVector goodweight;
    for (LocationVector::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        if (it->output.empty())
        {
            goodweight.push_back(*it);
            continue;
        }
        hfst_ol::Weight current_weight = it->weight;
        if (classes_found == -1) // we're just starting
        {
            classes_found = 1;
            last_weight_class = current_weight;
        }
        else if (last_weight_class != current_weight)
        {
            last_weight_class = current_weight;
            ++classes_found;
        }
        if (classes_found > s.max_weight_classes)
        {
            break;
        }
        else
        {
            goodweight.push_back(*it);
        }
    }
    return goodweight;
}

/**
 * Return the size in bytes of the first complete UTF-8 codepoint in c,
 * or 0 if invalid.
 */
size_t
u8_first_codepoint_size(const unsigned char *c)
{
    if (*c <= 127)
    {
        return 1;
    }
    else if ((*c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16))
    {
        return 4;
    }
    else if ((*c & (128 + 64 + 32)) == (128 + 64 + 32))
    {
        return 3;
    }
    else if ((*c & (128 + 64)) == (128 + 64))
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

/**
 * We define tags (non-lemmas) as being exactly the Multichar_symbols.
 * Since non-Multichar_symbols may still be multi*byte*, we check that
 * the symbol is strictly longer than the size of the first
 * possibly-multi-byte codepoint.
 *
 * If we have ICU, we check that the symbol is longer than the first
 * "character" (so characters composed of multiple codepoints are
 * treated the same as their non-composed counterparts). ICU
 * doesn't treat modifier letters of as part of the same character,
 * but we sometimes have them on the same arc – e.g. 'k̓ʷ' where 'ʷ' is
 * a modifier – so we skip following modifiers too (c.f. issue 497).
 */
bool
is_cg_tag(const string &str)
{
    icu::UnicodeString us(str.c_str());
    characterBoundary->setText(us);
    const int32_t i_after = characterBoundary->following(0);
    if (u_charType(us.char32At(i_after)) == U_MODIFIER_LETTER)
    {
        const bool is_tag
            = us.length() > characterBoundary->following(i_after);
        if (!IS_CG_TAG_MODIFIER_WARNED && !is_tag)
        {
            std::cerr
                << "WARNING: Skipping modifier letter for baseform letter "
                << str
                << " (to avoid this warning, ensure Modifiers are not part of "
                   "the same Multichar_symbol as their preceding Character)"
                << std::endl;
            IS_CG_TAG_MODIFIER_WARNED = true; // warn only once
        }
        return is_tag;
    }
    else
    {
        return us.length() > i_after;
    }
}

void
print_cg_subreading(size_t const &indent,
                    hfst::StringVector::const_iterator &out_beg,
                    hfst::StringVector::const_iterator &out_end,
                    hfst_ol::Weight const &weight,
                    hfst::StringVector::const_iterator &in_beg,
                    hfst::StringVector::const_iterator &in_end,
                    std::ostream &outstream, const TokenizeSettings &s)
{
    outstream << string(indent, '\t');
    bool in_lemma = false;
    for (hfst::StringVector::const_iterator it = out_beg; it != out_end; ++it)
    {
        if (it->compare("@PMATCH_BACKTRACK@") == 0)
        {
            continue;
        }
        bool is_tag = is_cg_tag(*it);
        if (in_lemma)
        {
            if (is_tag)
            {
                in_lemma = false;
                outstream << "\"";
            }
        }
        else
        {
            if (!is_tag)
            {
                in_lemma = true;
                outstream << "\"";
            }
        }
        print_escaping_backslashes(*it, outstream);
    }
    if (in_lemma)
    {
        outstream << "\"";
    }

    if (s.print_weights)
    {
        outstream << " " << format_weight_as_CG_numeric_tag(weight);
    }
    if (in_beg != in_end)
    {
        std::ostringstream form;
        std::copy(in_beg, in_end, std::ostream_iterator<string>(form, ""));
        outstream << " \"<";
        print_escaping_backslashes(form.str(), outstream);
        outstream << ">\"";
    }
    outstream << std::endl;
}

void
print_cg_subreading_ex(size_t const &indent,
                       hfst::StringVector::const_iterator &out_beg,
                       hfst::StringVector::const_iterator &out_end,
                       hfst_ol::Weight const &weight,
                       hfst::StringVector::const_iterator &in_beg,
                       hfst::StringVector::const_iterator &in_end,
                       std::string const &middle, std::ostream &outstream,
                       const TokenizeSettings &s)
{
    outstream << string(indent, '\t');
    bool in_lemma = false;
    for (hfst::StringVector::const_iterator it = out_beg; it != out_end; ++it)
    {
        if (it->compare("@PMATCH_BACKTRACK@") == 0)
        {
            continue;
        }
        bool is_tag = is_cg_tag(*it);
        if (in_lemma)
        {
            if (is_tag)
            {
                in_lemma = false;
                outstream << "\"";
            }
        }
        else
        {
            if (!is_tag)
            {
                in_lemma = true;
                outstream << "\"";
            }
        }
        print_escaping_backslashes(*it, outstream);
    }
    if (in_lemma)
    {
        outstream << "\"";
    }
    if ((s.hack_uncompose) && (!middle.empty()))
    {
        outstream << " \"" << middle << "\"MIDTAPE";
    }
    if (s.print_weights)
    {
        outstream << " " << format_weight_as_CG_numeric_tag(weight);
    }
    if (in_beg != in_end)
    {
        std::ostringstream form;
        std::copy(in_beg, in_end, std::ostream_iterator<string>(form, ""));
        outstream << " \"<";
        print_escaping_backslashes(form.str(), outstream);
        outstream << ">\"";
    }
    outstream << std::endl;
}

typedef std::set<size_t> SplitPoints;

pair<SplitPoints, size_t>
print_reading_giellacg(const Location *loc, size_t indent,
                       const bool always_wftag, std::ostream &outstream,
                       const TokenizeSettings &s)
{
    SplitPoints bt_its;
    if (loc->output.empty())
    {
        return make_pair(bt_its, indent);
    }
    else if ((loc->output.find(" ??") != string::npos) && (indent == 1))
    {
        return make_pair(bt_its, indent);
    }
    typedef hfst::StringVector::const_iterator PartIt;
    PartIt out_beg = loc->output_symbol_strings.begin(),
           out_end = loc->output_symbol_strings.end(),
           in_beg = loc->input_symbol_strings.begin(),
           in_end = loc->input_symbol_strings.end();
    if (!always_wftag)
    {
        // don't print input wordform tag unless we've seen a subreading/input
        // mark
        in_beg = in_end;
    }
    size_t part = loc->input_parts.size();
    while (true)
    {
        bool sub_found = false;
        size_t out_part = part > 0 ? loc->output_parts.at(part - 1) : 0;
        while (out_part > 0
               && loc->output_symbol_strings.at(out_part - 1)
                      == "@PMATCH_BACKTRACK@")
        {
            bt_its.insert(loc->input_parts.at(part - 1));
            --part;
            out_part = part > 0 ? loc->output_parts.at(part - 1) : 0;
        }
        for (PartIt it = out_end - 1;
             it > loc->output_symbol_strings.begin() + out_part; --it)
        {
            if (subreading_separator.compare(*it) == 0)
            {
                // Found a sub-reading mark
                out_beg = ++it;
                sub_found = true;
                break;
            }
        }
        if (!sub_found)
        {
            if (out_part > 0)
            {
                // Found an input mark
                out_beg = loc->output_symbol_strings.begin() + out_part;
                in_beg = loc->input_symbol_strings.begin()
                         + loc->input_parts.at(part - 1);
                --part;
            }
            else
            {
                // No remaining sub-marks or input-marks to the left
                out_beg = loc->output_symbol_strings.begin();
                if (in_end != loc->input_symbol_strings.end())
                {
                    // We've seen at least one input-mark, so we need to output
                    // the remaining input as well
                    in_beg = loc->input_symbol_strings.begin();
                }
            }
        }
        print_cg_subreading_ex(indent, out_beg, out_end, loc->weight, in_beg,
                               in_end, loc->middle, outstream, s);
        if (out_beg == loc->output_symbol_strings.begin())
        {
            break;
        }
        else
        {
            ++indent;
            out_end = out_beg;
            in_end = in_beg;
            if (sub_found)
            {
                --out_end; // skip the subreading separator symboli
            }
        }
    }
    if (!bt_its.empty())
    {
        bt_its.insert(0);
        bt_its.insert(loc->input_symbol_strings.size());
    }
    return make_pair(bt_its, indent);
}

/**
 * Treat syms as "characters" to concatenate and split at indices
 * given by splitpoints to create a new string vector. Assumes
 * splitpoints includes both ends of syms.
 */
const hfst::StringVector
split_at(const hfst::StringVector &syms, const SplitPoints *splitpoints)
{
    hfst::StringVector subs;
    if (splitpoints->size() < 2)
    {
        std::cerr << "split_at called with " << std::endl;
        return subs;
    }
    // Loop to next-to-last
    for (SplitPoints::const_iterator it = splitpoints->begin();
         std::next(it) != splitpoints->end(); ++it)
    {
        std::ostringstream ss;
        // Copy the substring between this point and the next:
        std::copy(syms.begin() + *(it), syms.begin() + *(std::next(it)),
                  std::ostream_iterator<string>(ss, ""));
        subs.push_back(ss.str());
    }
    return subs;
}

/*
 * Look up form, filtering out empties and those that don't cover the
 * full string.
 */
const LocationVector
locate_fullmatch(hfst_ol::PmatchContainer &container, string &form,
                 const TokenizeSettings &s)
{
    LocationVectorVector sublocs = container.locate(form, s.time_cutoff);
    LocationVector loc_filtered;
    // TODO: Worth noticing about? Is this as safe as checking that
    // input.length != form.length? if(sublocs.size() != 1) {
    //     std::cerr << "Warning: '" << form << "' only tokenisable by further
    //     splitting."<<std::endl;
    // }
    for (LocationVectorVector::const_iterator it = sublocs.begin();
         it != sublocs.end(); ++it)
    {
        if (it->empty()
            || (it->size() == 1
                && it->at(0).output.compare("@_NONMATCHING_@") == 0)
            // keep only those that cover the full form
            || it->at(0).input.length() != form.length())
        {
            continue;
        }
        LocationVector loc = keep_n_best_weight(dedupe_locations(*it, s), s);
        for (LocationVector::iterator loc_it = loc.begin();
             loc_it != loc.end(); ++loc_it)
        {
            if (!loc_it->output.empty()
                && loc_it->weight < std::numeric_limits<float>::max()
                && (loc_it->output.find(" ??") == string::npos))
            {
                // TODO: why aren't the <W:inf> excluded earlier?
                if (s.hack_uncompose)
                {
                    container.uncompose(*loc_it);
                }
                loc_filtered.push_back(*loc_it);
            }
        }
    }
    return loc_filtered;
}

void
print_location_vector_giellacg(hfst_ol::PmatchContainer &container,
                               LocationVector const &locations,
                               std::ostream &outstream,
                               const TokenizeSettings &s)
{
    outstream << "\"<";
    print_escaping_backslashes(locations.at(0).input, outstream);
    outstream << ">\"" << std::endl;
    if (locations.size() == 1
        && (locations.at(0).output.empty()
            || locations.at(0).output.find(" ??") != string::npos))
    {
        // Treat empty analyses as unknown-but-tokenised:
        // and ??
        outstream << "\t\"";
        print_escaping_backslashes(locations.at(0).input, outstream);
        outstream << "\" ?" << std::endl;
        return;
    }
    // Output regular analyses first, making a note of backtracking points.
    std::set<SplitPoints> backtrack;
    for (LocationVector::const_iterator loc_it = locations.begin();
         loc_it != locations.end(); ++loc_it)
    {
        // Check for uncompose
        Location *hack = new Location(*loc_it);
        if (s.hack_uncompose)
        {
            container.uncompose(*hack);
        }
        SplitPoints bt_points
            = print_reading_giellacg(hack, 1, false, outstream, s).first;
        if (!bt_points.empty())
        {
            backtrack.insert(bt_points);
        }
        delete hack;
    }
    if (backtrack.empty())
    {
        return;
    }
    // The rest of the function handles possible backtracking:
    hfst::StringVector in_syms = locations.at(0).input_symbol_strings;

    for (std::set<SplitPoints>::const_iterator bt_points = backtrack.begin();
         bt_points != backtrack.end(); ++bt_points)
    {

        // First, for every set of backtrack points, we split on every
        // point in that N+1-sized set (the backtrack points include
        // start/end points), and create an N-sized vector splitlocs of
        // resulting analyses
        LocationVectorVector splitlocs;
        hfst::StringVector words = split_at(in_syms, &*(bt_points));
        for (hfst::StringVector::const_iterator it = words.begin();
             it != words.end(); ++it)
        {
            // Trim left/right spaces:
            const size_t first = find_first_not_of_def(*it, ' ', 0);
            const size_t last
                = 1 + find_last_not_of_def(*it, ' ', it->length() - 1);
            string form = it->substr(first, last - first);
            LocationVector loc = locate_fullmatch(container, form, s);
            if (loc.size() == 0 && s.verbose)
            {
                std::cerr << "Warning: The analysis of \"<"
                          << locations.at(0).input
                          << ">\" has backtracking around the substring \"<"
                          << form << ">\", but that substring has no analyses."
                          << std::endl;
                // but push it anyway, since we want exactly one subvector per
                // splitpoint
            }
            if (form.length() != it->length())
            { // Ensure the spaces we ignored when looking up are output in the
              // form:
                vector<string> lspace = vector<string>(first, " ");
                vector<string> rspace
                    = vector<string>(it->length() - last, " ");
                for (LocationVector::iterator lvit = loc.begin();
                     lvit != loc.end(); ++lvit)
                {
                    lvit->input = form;
                    vector<string> &syms = lvit->input_symbol_strings;
                    syms.insert(syms.begin(), lspace.begin(), lspace.end());
                    syms.insert(syms.end(), rspace.begin(), rspace.end());
                    for (vector<size_t>::iterator ip
                         = lvit->input_parts.begin();
                         ip != lvit->input_parts.end(); ++ip)
                    {
                        *ip += first;
                    }
                }
            }
            splitlocs.push_back(loc);
        }
        if (splitlocs.empty())
        {
            continue;
        }
        // Second, we reorder splitlocs so we can output as a
        // cohort of non-branching CG subreadings; first word as leaf
        // nodes. This means that splitlocs = [[A,B],[C,D]] should
        // end up as the sequence
        // (C,0),(A,1),(C,0),(B,1),(D,0),(A,1),(D,0),(B,1)
        // (where the number is the initial indentation).
        size_t depth = 0;
        const size_t bottom = splitlocs.size() - 1;
        vector<std::ostringstream> out(splitlocs.size());
        vector<pair<LocationVector, size_t> > stack;
        // In CG the *last* word is the least indented, so start from
        // the end of splitlocs, indentation being 1 tab:
        stack.push_back(make_pair(splitlocs.at(bottom), 0));
        while (!stack.empty() && !stack.back().first.empty())
        {
            LocationVector &locs = stack.back().first;
            const Location loc = locs.back();
            locs.pop_back();
            const size_t indent = 1 + stack.back().second;
            out.at(depth).clear();
            out.at(depth).str(string());
            // (ignore splitpoints of splitpoints)
            const size_t new_indent
                = print_reading_giellacg(&loc, indent, true, out.at(depth), s)
                      .second;
            if (depth == bottom)
            {
                for (vector<std::ostringstream>::const_iterator it
                     = out.begin();
                     it != out.end(); ++it)
                {
                    outstream << it->str();
                }
            }
            if (depth < bottom)
            {
                ++depth;
                if (depth > 0)
                {
                    stack.push_back(
                        make_pair(splitlocs.at(bottom - depth), new_indent));
                }
            }
            else if (locs.empty())
            {
                depth--;
                stack.pop_back();
            }
        }
    }
}

// Omorfi-specific at this time
std::string
fetch_and_kill_between(std::string left, std::string right,
                       std::string &analysis)
{
    size_t start = analysis.find(left);
    size_t stop = analysis.find(right, start + 1);
    if (start == std::string::npos || stop == std::string::npos)
    {
        return "";
    }
    std::string retval
        = analysis.substr(start + left.size(), stop - start - left.size());
    analysis.erase(start, stop - start + right.size());
    return retval;
}

std::string
fetch_and_kill_feats(std::string &analysis)
{
    std::string retval;
    std::string tmp;
    tmp = fetch_and_kill_between("[ANIMACY=", "]", analysis);
    retval += (tmp != "" ? ("Animacy=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[ASPECT=", "]", analysis);
    retval += (tmp != "" ? ("Aspect=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[CASE=", "]", analysis);
    retval += (tmp != "" ? ("Case=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[DEFINITE=", "]", analysis);
    retval += (tmp != "" ? ("Definite=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[CMP=", "]", analysis);
    retval += (tmp != "" ? ("Degree=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[GENDER=", "]", analysis);
    retval += (tmp != "" ? ("Gender=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[MOOD=", "]", analysis);
    retval += (tmp != "" ? ("Mood=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[NEGATIVE=", "]", analysis);
    retval += (tmp != "" ? ("Negative=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[NUMTYPE=", "]", analysis);
    retval += (tmp != "" ? ("Numtype=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[NUM=", "]", analysis);
    retval += (tmp != "" ? ("Number=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[PERS=", "]", analysis);
    retval += (tmp != "" ? ("Person=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[POSS=", "]", analysis);
    retval += (tmp != "" ? ("Poss=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[PRONTYPE=", "]", analysis);
    retval += (tmp != "" ? ("PronType=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[REFLEX=", "]", analysis);
    retval += (tmp != "" ? ("Reflex=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[TENSE=", "]", analysis);
    retval += (tmp != "" ? ("Tense=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[VERBFORM=", "]", analysis);
    retval += (tmp != "" ? ("VerbForm=" + tmp + "|") : "");
    tmp = fetch_and_kill_between("[VOICE=", "]", analysis);
    retval += (tmp != "" ? ("Voice=" + tmp + "|") : "");
    if (retval.size() != 0)
    {
        retval.erase(retval.size() - 1);
    }
    return retval;
}

std::string
empty_to_underscore(std::string to_test)
{
    if (to_test.size() == 0)
    {
        return "_";
    }
    return to_test;
}

// Helper function to process a single JSON segment and its subreadings recursively
void process_json_segment(std::pair<hfst::StringVector::const_iterator, hfst::StringVector::const_iterator> segment,
                         std::vector<std::pair<hfst::StringVector::const_iterator, hfst::StringVector::const_iterator>> &remaining_segments,
                         hfst_ol::Weight const &weight,
                         std::ostream &outstream,
                         const TokenizeSettings &s,
                         bool &first_tag)
{
    hfst::StringVector::const_iterator seg_beg = segment.first;
    hfst::StringVector::const_iterator seg_end = segment.second;

    // Find first tag in the segment
    hfst::StringVector::const_iterator first_tag_it = seg_end;
    for (auto it = seg_beg; it != seg_end; ++it) {
        if (it->compare("@PMATCH_BACKTRACK@") == 0) continue;
        if (is_cg_tag(*it)) {
            first_tag_it = it;
            break;
        }
    }

    // Build lemma
    std::ostringstream lemma_ss;
    for (auto it = seg_beg; it != first_tag_it; ++it) {
        if (it->compare("@PMATCH_BACKTRACK@") == 0) continue;
        lemma_ss << *it;
    }

    // Build tags list
    std::vector<std::string> tags;
    for (auto it = first_tag_it; it != seg_end; ++it) {
        if (it->compare("@PMATCH_BACKTRACK@") == 0) continue;
        if (is_cg_tag(*it)) {
            std::string tag = *it;
            tag.erase(0, tag.find_first_not_of(' '));
            tags.push_back(tag);
        }
    }

    // Output this segment's data
    outstream << "{\"l\":\"";
    print_json_escaped(lemma_ss.str(), outstream);
    outstream << "\",\"ts\":[";

    bool first_tag_local = true;
    for (const auto &tag : tags) {
        if (!first_tag_local) {
            outstream << ",";
        }
        outstream << "\"";
        print_json_escaped(tag, outstream);
        outstream << "\"";
        first_tag_local = false;
    }
    outstream << "]";

    // Process next subreading if available
    if (!remaining_segments.empty()) {
        outstream << ",\"s\":";
        auto next_segment = remaining_segments.front();
        remaining_segments.erase(remaining_segments.begin());
        process_json_segment(next_segment, remaining_segments, weight, outstream, s, first_tag);
    }

    outstream << "}";
}

void
print_json_reading(hfst::StringVector::const_iterator out_beg,
                   hfst::StringVector::const_iterator out_end,
                   hfst_ol::Weight const &weight,
                   hfst::StringVector::const_iterator in_beg,
                   hfst::StringVector::const_iterator in_end,
                   size_t &part,
                   const Location *loc,
                   std::ostream &outstream, const TokenizeSettings &s,
                   bool &first_reading)
{
    if (!first_reading)
    {
        outstream << ",";
    }
    first_reading = false;

    // We need to split the output into segments based on the separator and analyze them separately
    std::vector<std::pair<hfst::StringVector::const_iterator, hfst::StringVector::const_iterator>> segments;

    // Find all separators from right to left and build segments
    hfst::StringVector::const_iterator segment_end = out_end;
    size_t out_part_idx = (part > 0) ? loc->output_parts.at(part - 1) : 0;
    hfst::StringVector::const_iterator segment_start = (out_part_idx > 0) ?
        (loc->output_symbol_strings.begin() + out_part_idx) : out_beg;

    // Scan from right to left to find all separators
    bool found_separator = false;
    for (hfst::StringVector::const_iterator it = out_end; it > segment_start; --it) {
        if (it != out_end && subreading_separator.compare(*(it-1)) == 0) {
            // Found a separator - add segment from separator+1 to current segment_end
            segments.push_back(std::make_pair(it, segment_end));
            segment_end = it - 1; // Move end to before the separator
            found_separator = true;
        }
    }

    // Add the leftmost segment (or the only segment if no separators)
    segments.push_back(std::make_pair(segment_start, segment_end));

    // Process all segments recursively
    if (!segments.empty()) {
        // Start with the rightmost segment (main reading)
        auto main_segment = segments[0];

        // Create a list of remaining segments (subreadings)
        std::vector<std::pair<hfst::StringVector::const_iterator, hfst::StringVector::const_iterator>> remaining_segments;
        for (size_t i = 1; i < segments.size(); i++) {
            remaining_segments.push_back(segments[i]);
        }

        // Process main segment and recursively handle subreadings
        bool dummy_first_tag = true;
        process_json_segment(main_segment, remaining_segments, weight, outstream, s, dummy_first_tag);
    }
    else {
        // This should never happen, but just in case
        outstream << "\"l\":\"\",\"ts\":[]";
    }
}

void
print_location_vector(hfst_ol::PmatchContainer &container,
                      LocationVector const &locations, std::ostream &outstream,
                      int token_number, const TokenizeSettings &s)
{
    if (s.output_format == tokenize && locations.size() != 0)
    {
        outstream << locations.at(0).input;
        if (s.print_weights)
        {
            outstream << "\t" << locations.at(0).weight;
        }
        outstream << std::endl;
        if (locations.at(0).tag == "<Boundary=Sentence>")
        {
            outstream << std::endl;
        }
    }
    else if (s.output_format == space_separated && locations.size() != 0)
    {
        outstream << locations.at(0).input;
        if (s.print_weights)
        {
            outstream << "\t" << locations.at(0).weight;
        }
        outstream << " ";
        if (locations.at(0).tag == "<Boundary=Sentence>")
        {
            outstream << std::endl;
        }
    }
    else if (s.output_format == cg && locations.size() != 0)
    {
        // Print the cg cohort header
        outstream << "\"<";
        print_escaping_backslashes(locations.at(0).input, outstream);
        outstream << ">\"" << std::endl;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            // For the most common case, eg. analysis strings that begin with
            // the original input, we try to do what cg tools expect and
            // surround the original input with double quotes. Otherwise we
            // omit the double quotes and assume the rule writer knows what
            // he's doing.
            if (loc_it->output.find(loc_it->input) == 0)
            {
                // The nice case obtains
                outstream << "\t\"";
                print_escaping_backslashes(loc_it->input, outstream);
                outstream << "\""
                          << loc_it->output.substr(loc_it->input.size(),
                                                   std::string::npos);
            }
            else
            {
                outstream << "\t" << loc_it->output;
            }
            if (s.print_weights)
            {
                outstream << "\t" << loc_it->weight;
            }
            outstream << std::endl;
        }
        outstream << std::endl;
    }
    else if (s.output_format == giellacg && locations.size() != 0)
    {
        print_location_vector_giellacg(container, locations, outstream, s);
    }
    else if (s.output_format == visl && locations.size() != 0)
    {
        print_location_vector_giellacg(container, locations, outstream, s);
    }
    else if (s.output_format == xerox)
    {
        float best_weight = std::numeric_limits<float>::max();
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            if (best_weight > loc_it->weight)
            {
                best_weight = loc_it->weight;
            }
        }
        bool printed_something = false;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            if ((s.beam < 0.0 || loc_it->weight <= best_weight + s.beam) &&
                // We don't print "plain" tokens without any analysis
                // except if they are the only one present
                (loc_it->output.compare(loc_it->input) != 0
                 || (loc_it + 1 == locations.end() && !printed_something)))
            {
                outstream << loc_it->input << "\t" << loc_it->output;
                if (s.print_weights)
                {
                    if (loc_it + 1 == locations.end() && !printed_something)
                    {
                        outstream << "\t" << best_weight;
                    }
                    else
                    {
                        outstream << "\t" << loc_it->weight;
                    }
                }
                outstream << std::endl;
                printed_something = true;
            }
        }
        if (locations.at(0).tag == "<Boundary=Sentence>")
        {
            outstream << std::endl;
        }
        outstream << std::endl;
    }
    else if (s.output_format == conllu)
    {
        hfst_ol::Weight lowest_weight = hfst_ol::INFINITE_WEIGHT;
        hfst_ol::Location best_location;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            if (loc_it->weight < lowest_weight)
            {
                best_location = *loc_it;
                lowest_weight = loc_it->weight;
            }
            //            if (loc_it->tag == "@MULTIWORD@"
            //            outstream << loc_it->input << "\t" << loc_it->output;
        }
        outstream << token_number << "\t" << best_location.input;
        outstream << "\t"
                  << empty_to_underscore(fetch_and_kill_between(
                         "[WORD_ID=", "]", best_location.output));
        outstream << "\t"
                  << empty_to_underscore(fetch_and_kill_between(
                         "[UPOS=", "]", best_location.output));
        outstream << "\t"
                  << empty_to_underscore(fetch_and_kill_between(
                         "[XPOS=", "]", best_location.output));
        outstream << "\t"
                  << empty_to_underscore(
                         fetch_and_kill_feats(best_location.output))
                  << "\t"
                  << "_" // HEAD
                  << "\t"
                  << "_" // DEPREL
                  << "\t"
                  << "_";                                               // DEPS
        outstream << "\t" << empty_to_underscore(best_location.output); // MISC
        if (s.print_weights)
        {
            outstream << "\t" << best_location.weight;
        }
        outstream << std::endl;
    }
    else if (s.output_format == finnpos)
    {
        std::set<std::string> tags;
        std::set<std::string> lemmas;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            // Assume the last space is where the tags begin
            size_t tags_start_at = loc_it->output.find_last_of(" ");
            if (tags_start_at != std::string::npos)
            {
                std::string lemma = loc_it->output.substr(0, tags_start_at);
                if (lemma.find_first_of(" ") == std::string::npos)
                {
                    // can't have spaces in lemmas
                    lemmas.insert(lemma);
                }
                std::string tag = loc_it->output.substr(tags_start_at + 1);
                if (tag.find_first_of(" ") == std::string::npos)
                {
                    // or tags
                    tags.insert(tag);
                }
            }
        }
        outstream << locations.at(0).input << "\t_\t";
        // the input and a blank for features
        if (lemmas.empty())
        {
            outstream << "_";
        }
        else
        {
            std::string accumulator;
            for (std::set<std::string>::const_iterator it = lemmas.begin();
                 it != lemmas.end(); ++it)
            {
                accumulator.append(*it);
                accumulator.append(" ");
            }
            outstream << accumulator.substr(0, accumulator.size() - 1);
        }
        outstream << "\t";
        if (tags.empty())
        {
            outstream << "_";
        }
        else
        {
            std::string accumulator;
            for (std::set<std::string>::const_iterator it = tags.begin();
                 it != tags.end(); ++it)
            {
                accumulator.append(*it);
                accumulator.append(" ");
            }
            outstream << accumulator.substr(0, accumulator.size() - 1);
        }
        outstream << "\t_" << std::endl;
        if (locations.at(0).tag == "<Boundary=Sentence>")
        {
            outstream << std::endl;
        }
    }
    else if (s.output_format == jsonl && locations.size() != 0)
    {
        outstream << "{\"w\":\"";
        print_json_escaped(locations.at(0).input, outstream);
        outstream << "\"";
        // Determine if there are any valid readings to print.
        // A reading is invalid if its output is empty, contains " ??", is "@_NONMATCHING_@",
        // or its weight is infinite.
        std::vector<const hfst_ol::Location*> valid_locations_to_process;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it)
        {
            if (loc_it->output.empty() ||
                loc_it->output.find(" ??") != string::npos ||
                loc_it->output.compare("@_NONMATCHING_@") == 0 ||
                loc_it->weight >= std::numeric_limits<float>::max())
            {
                continue; // Skip invalid readings
            }
            valid_locations_to_process.push_back(&(*loc_it));
        }

        if (!valid_locations_to_process.empty())
        {
            outstream << ",\"rs\":[";
            bool first_reading = true;
            for (const hfst_ol::Location* original_loc_ptr : valid_locations_to_process)
            {
                // No need to re-filter; original_loc_ptr points to a valid location.
                Location *hack = new Location(*original_loc_ptr);
                if (s.hack_uncompose)
                {
                    container.uncompose(*hack);
                }

                size_t initial_part = hack->input_parts.size();
                print_json_reading(hack->output_symbol_strings.begin(),
                                   hack->output_symbol_strings.end(),
                                   hack->weight,
                                   hack->input_symbol_strings.begin(),
                                   hack->input_symbol_strings.end(),
                                   initial_part,
                                   hack,
                                   outstream, s, first_reading);
                delete hack;
            }
            outstream << "]";
        }
        outstream << "}\n";
    }
    //    std::cerr << "from print_location_vector\n";
}

void
match_and_print(hfst_ol::PmatchContainer &container, std::ostream &outstream,
                const string &input_text, const TokenizeSettings &s)
{
    LocationVectorVector locations
        = container.locate(input_text, s.time_cutoff);
    if (locations.size() == 0 && s.print_all)
    {
        print_no_output(input_text, outstream, s);
        return;
    }
    int token_number = 1;
    for (LocationVectorVector::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        if ((it->size() == 1
             && it->at(0).output.compare("@_NONMATCHING_@") == 0))
        {
            if (s.print_all)
            {
                print_nonmatching_sequence(it->at(0).input, outstream, s);
            }
            continue;
            // All nonmatching cases have been handled
        }
        print_location_vector(container,
                              keep_n_best_weight(dedupe_locations(*it, s), s),
                              outstream, token_number, s);
        ++token_number;
    }
    if (s.output_format == finnpos || s.output_format == tokenize
        || s.output_format == xerox)
    {
        outstream << std::endl;
    }
}

void
process_input(hfst_ol::PmatchContainer &container, std::istream &instream,
              std::ostream &outstream, const TokenizeSettings &s)
{
    container.set_single_codepoint_tokenization(!s.tokenize_multichar);
    const size_t bufsize = 4096;
    for (char line[bufsize]; instream.getline(line, bufsize);)
    {
        string input_text(line);
        if (!input_text.empty())
        {
            match_and_print(container, outstream, input_text, s);
        }
    }
}

}
