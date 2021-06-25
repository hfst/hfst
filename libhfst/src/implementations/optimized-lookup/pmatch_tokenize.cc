// Copyright (c) 2016-2019 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "pmatch_tokenize.h"


#if USE_ICU_UNICODE
#include <unicode/unistr.h>
#include <unicode/brkiter.h>
static UErrorCode characterBoundaryStatus = U_ZERO_ERROR;
static icu::BreakIterator* characterBoundary = icu::BreakIterator::createCharacterInstance(NULL, characterBoundaryStatus);
#endif

namespace hfst_ol_tokenize {

using std::string;
using std::vector;
using std::pair;

using hfst_ol::Location;
using hfst_ol::LocationVector;
using hfst_ol::LocationVectorVector;

static const string subreading_separator = "#";
static const string wtag = "W"; // TODO: cg-conv has an argument --wtag, allow changing here as well?
static bool IS_CG_TAG_MODIFIER_WARNED = false; // Only warn once on skipping modifier letters

void print_escaping_backslashes(std::string const & str, std::ostream & outstream)
{
    // TODO: inline?
    size_t i = 0, j = 0;
    while((j = str.find("\\", i)) != std::string::npos) {
        outstream << str.substr(i, j-i) << "\\\\";
        i = j+1;
    }
    outstream << str.substr(i, j-i);
}

void print_no_output(std::string const & input, std::ostream & outstream, const TokenizeSettings& s)
{
    if (s.output_format == tokenize || s.output_format == space_separated) {
        outstream << input;
    } else if (s.output_format == xerox) {
        outstream << input << "\t" << input << "+?";
    } else if (s.output_format == cg || s.output_format == giellacg) {
	    outstream << "\"<";
        print_escaping_backslashes(input, outstream);
        outstream << ">\"" << std::endl << "\t\"";
        print_escaping_backslashes(input, outstream);
        outstream << "\" ?";
    }
//    std::cerr << "from print_no_output\n";
    outstream << "\n\n";
}

void print_escaping_newlines(std::string const & str, std::ostream & outstream)
{
    // TODO: inline?
    size_t i = 0, j = 0;
    while((j = str.find("\n", i)) != std::string::npos) {
        outstream << str.substr(i, j-i) << "\\n";
        i = j+1;
    }
    outstream << str.substr(i, j-i);
}

void print_nonmatching_sequence(std::string const & str, std::ostream & outstream, const TokenizeSettings& s)
{
    if (s.output_format == tokenize || s.output_format == space_separated) {
        outstream << str;
    } else if (s.output_format == xerox) {
        outstream << str << "\t" << str << "+?";
    } else if (s.output_format == cg) {
        outstream << "\"<";
        print_escaping_backslashes(str, outstream);
        outstream << ">\"" << std::endl << "\t\"";
        print_escaping_backslashes(str, outstream);
        outstream << "\" ?";
    } else if (s.output_format == giellacg) {
        outstream << ":";
        print_escaping_newlines(str, outstream);
    } else if (s.output_format == visl) {
        outstream << str;
    } else if (s.output_format == conllu) {
        outstream << str;
    } else if (s.output_format == finnpos) {
        outstream << str << "\t_\t_\t_\t_";
    }
//    std::cerr << "from print_nonmatching_sequence\n";
    outstream << "\n";
}

bool location_compare(const Location& lhs, const Location& rhs) {
    if (lhs.weight == rhs.weight) {
        if(lhs.tag == rhs.tag) {
            if(lhs.start == rhs.start){
                if(lhs.length == rhs.length) {
                    return lhs.output < rhs.output;
                }
                else {
                    return lhs.length < rhs.length;
                }
            }
            else {
                return lhs.start < rhs.start;
            }
        }
        else {
            return lhs.tag < rhs.tag;
        }
    }
    else {
        return lhs.weight < rhs.weight;
    }
}

bool location_compare_ignoring_weights(const Location& lhs, const Location& rhs) {
    if(lhs.tag == rhs.tag) {
        if(lhs.start == rhs.start){
            if(lhs.length == rhs.length) {
                return lhs.output < rhs.output;
            }
            else {
                return lhs.length < rhs.length;
            }
        }
        else {
            return lhs.start < rhs.start;
        }
    }
    else {
        return lhs.tag < rhs.tag;
    }
}

bool location_compare_using_only_weights(const Location& lhs, const Location& rhs) {
    return lhs.weight < rhs.weight;
}

const LocationVector dedupe_locations(LocationVector const & locations, const TokenizeSettings & s) {
    if(!s.dedupe) {
        return locations;
    }
    if(s.print_weights) {
        std::set<Location, bool(*)(const Location& lhs, const Location& rhs)> ls(&location_compare);
        ls.insert(locations.begin(), locations.end());
        LocationVector uniq;
        std::copy(ls.begin(), ls.end(), std::back_inserter(uniq));
        return uniq;
    } else {
        std::set<Location, bool(*)(const Location& lhs, const Location& rhs)> ls(&location_compare_ignoring_weights);
        ls.insert(locations.begin(), locations.end());
        LocationVector uniq;
        std::copy(ls.begin(), ls.end(), std::back_inserter(uniq));
        std::sort(uniq.begin(), uniq.end(), location_compare_using_only_weights);
        return uniq;
    }
}
/**
 * Keep only the max_weight_classes best weight classes
 */
const LocationVector keep_n_best_weight(LocationVector const & locations, const TokenizeSettings& s)
{
    if(locations.size() <= s.max_weight_classes) {
        // We know we won't trim anything, no need to copy the vector:
        return locations;
    }
    int classes_found = -1;
    hfst_ol::Weight last_weight_class = 0.0;
    LocationVector goodweight;
    for (const auto & location : locations) {
        if(location.output.empty()) {
            goodweight.push_back(location);
            continue;
        }
        hfst_ol::Weight current_weight = location.weight;
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
        else {
            goodweight.push_back(location);
        }
    }
    return goodweight;
}

/**
 * Return the size in bytes of the first complete UTF-8 codepoint in c,
 * or 0 if invalid.
 */
size_t u8_first_codepoint_size(const unsigned char* c) {
    if (*c <= 127) {
        return 1;
    }
    else if ( (*c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16) ) {
        return 4;
    }
    else if ( (*c & (128 + 64 + 32 )) == (128 + 64 + 32) ) {
        return 3;
    }
    else if ( (*c & (128 + 64 )) == (128 + 64)) {
        return 2;
    }
    else {
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
bool is_cg_tag(const string & str) {
#if USE_ICU_UNICODE
    icu::UnicodeString us(str.c_str());
    characterBoundary->setText(us);
    const int32_t i_after = characterBoundary->following(0);
    if(u_charType(us.char32At(i_after)) == U_MODIFIER_LETTER) {
        const bool is_tag = us.length() > characterBoundary->following(i_after);
        if(!IS_CG_TAG_MODIFIER_WARNED && !is_tag) {
            std::cerr << "WARNING: Skipping modifier letter for baseform letter " << str << " (to avoid this warning, ensure Modifiers are not part of the same Multichar_symbol as their preceding Character)" << std::endl;
            IS_CG_TAG_MODIFIER_WARNED = true; // warn only once
        }
        return is_tag;
    }
    else {
        return us.length() > i_after;
    }
#else
    // Note: invalid codepoints are also treated as tags;  ¯\_(ツ)_/¯
    return str.size() > u8_first_codepoint_size((const unsigned char*)str.c_str());
#endif
}

void print_cg_subreading(size_t const & indent,
                         hfst::StringVector::const_iterator & out_beg,
                         hfst::StringVector::const_iterator & out_end,
                         hfst_ol::Weight const & weight,
                         hfst::StringVector::const_iterator & in_beg,
                         hfst::StringVector::const_iterator & in_end,
                         std::ostream & outstream,
                         const TokenizeSettings& s)
{
    outstream << string(indent, '\t');
    bool in_lemma = false;
    for(hfst::StringVector::const_iterator it = out_beg;
        it != out_end; ++it) {
        if(it->compare("@PMATCH_BACKTRACK@") == 0) {
            continue;
        }
        bool is_tag = is_cg_tag(*it);
        if(in_lemma) {
            if(is_tag) {
                in_lemma = false;
                outstream << "\"";
            }
        }
        else {
            if(!is_tag) {
                in_lemma = true;
                outstream << "\"";
            }
        }
        print_escaping_backslashes(*it, outstream);
    }
    if(in_lemma) {
        outstream << "\"";
    }

    if (s.print_weights) {
        std::ostringstream w;
        w << std::fixed << std::setprecision(9) << weight;
        std::string rounded = w.str();
        bool seendot = false;
        bool inzeroes = true;
        size_t firstzero = rounded.length();
        for(size_t i = rounded.length(); i > 0; --i) {
            if(inzeroes && rounded[i-1] == '0') {
                firstzero = i;  // not i-1, keep one zero
            }
            else {
                inzeroes = false;
            }
            if(rounded[i-1] == '.') {
                seendot = true;
                break;
            }
        }
        if(seendot) {
            rounded = rounded.substr(0, firstzero);
        }
        outstream << " <" << wtag << ":" << rounded << ">";
    }
    if (in_beg != in_end) {
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
print_reading_giellacg(const Location *loc,
                       size_t indent,
                       const bool always_wftag,
                       std::ostream & outstream,
                       const TokenizeSettings& s)
{
    SplitPoints bt_its;
    if(loc->output.empty()) {
        return make_pair(bt_its, indent);
    }
    typedef hfst::StringVector::const_iterator PartIt;
    PartIt
        out_beg = loc->output_symbol_strings.begin(),
        out_end = loc->output_symbol_strings.end(),
        in_beg = loc->input_symbol_strings.begin(),
        in_end = loc->input_symbol_strings.end();
    if(!always_wftag) {
        // don't print input wordform tag unless we've seen a subreading/input mark
        in_beg = in_end;
    }
    size_t part = loc->input_parts.size();
    while(true) {
        string inpart;
        bool sub_found = false;
        size_t out_part = part > 0 ? loc->output_parts.at(part-1) : 0;
        while(out_part > 0 && loc->output_symbol_strings.at(out_part-1) == "@PMATCH_BACKTRACK@") {
            bt_its.insert(loc->input_parts.at(part-1));
            --part;
            out_part = part > 0 ? loc->output_parts.at(part-1) : 0;
        }
        for(PartIt it = out_end-1;
            it > loc->output_symbol_strings.begin() + out_part;
            --it) {
            if(subreading_separator.compare(*it) == 0) {
                // Found a sub-reading mark
                out_beg = ++it;
                sub_found = true;
                break;
            }
        }
        if(!sub_found) {
            if(out_part > 0) {
                // Found an input mark
                out_beg = loc->output_symbol_strings.begin() + out_part;
                in_beg = loc->input_symbol_strings.begin() + loc->input_parts.at(part-1);
                --part;
            }
            else {
                // No remaining sub-marks or input-marks to the left
                out_beg = loc->output_symbol_strings.begin();
                if(in_end != loc->input_symbol_strings.end()) {
                    // We've seen at least one input-mark, so we need to output the remaining input as well
                    in_beg = loc->input_symbol_strings.begin();
                }
            }
        }
        print_cg_subreading(indent,
                            out_beg,
                            out_end,
                            loc->weight,
                            in_beg,
                            in_end,
                            outstream,
                            s);
        if(out_beg == loc->output_symbol_strings.begin()) {
            break;
        }
        else {
            ++indent;
            out_end = out_beg;
            in_end = in_beg;
            if(sub_found) {
                --out_end; // skip the subreading separator symbol
            }
        }
    }
    if(!bt_its.empty()) {
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
const hfst::StringVector split_at(const hfst::StringVector & syms,
                                  const SplitPoints * splitpoints)
{
    hfst::StringVector subs;
    if(splitpoints->size() < 2) {
        std::cerr << "split_at called with " << std::endl;
        return subs;
    }
    // Loop to next-to-last
    for(SplitPoints::const_iterator it = splitpoints->begin(); std::next(it) != splitpoints->end(); ++it) {
        std::ostringstream ss;
        // Copy the substring between this point and the next:
        std::copy(syms.begin() + *(it),
                  syms.begin() + *(std::next(it)),
                  std::ostream_iterator<string>(ss, ""));
        subs.push_back(ss.str());
    }
    return subs;
}

/*
 * Look up form, filtering out empties and those that don't cover the
 * full string.
 */
const LocationVector locate_fullmatch(hfst_ol::PmatchContainer & container,
                                      string & form,
                                      const TokenizeSettings& s)
{
    LocationVectorVector sublocs = container.locate(form, s.time_cutoff);
    LocationVector loc_filtered;
    // TODO: Worth noticing about? Is this as safe as checking that input.length != form.length?
    // if(sublocs.size() != 1) {
    //     std::cerr << "Warning: '" << form << "' only tokenisable by further splitting."<<std::endl;
    // }
    for(const auto & subloc : sublocs) {
        if (subloc.empty()
            || (subloc.size() == 1 && subloc.at(0).output.compare("@_NONMATCHING_@") == 0)
            // keep only those that cover the full form
            || subloc.at(0).input.length() != form.length()) {
            continue;
        }
        LocationVector loc = keep_n_best_weight(dedupe_locations(subloc, s), s);
        for (const auto & loc_it : loc) {
            if(!loc_it.output.empty()
               && loc_it.weight < std::numeric_limits<float>::max()) {
                // TODO: why aren't the <W:inf> excluded earlier?
                loc_filtered.push_back(loc_it);
            }
        }
    }
    return loc_filtered;
}

void print_location_vector_giellacg(hfst_ol::PmatchContainer & container,
                                    LocationVector const & locations,
                                    std::ostream & outstream,
                                    const TokenizeSettings& s)
{
    outstream << "\"<";
    print_escaping_backslashes(locations.at(0).input, outstream);
    outstream << ">\"" << std::endl;
    if(locations.size() == 1 && locations.at(0).output.empty()) {
        // Treat empty analyses as unknown-but-tokenised:
        outstream << "\t\"";
        print_escaping_backslashes(locations.at(0).input, outstream);
        outstream << "\" ?" << std::endl;
        return;
    }
    // Output regular analyses first, making a note of backtracking points.
    std::set<SplitPoints> backtrack;
    for (const auto & location : locations) {
        SplitPoints bt_points = print_reading_giellacg(&location, 1, false, outstream, s).first;
        if(!bt_points.empty()) {
            backtrack.insert(bt_points);
        }
    }
    if(backtrack.empty()) {
	return;
    }
    // The rest of the function handles possible backtracking:
    hfst::StringVector in_syms = locations.at(0).input_symbol_strings;

    for(const auto & bt_points : backtrack) {

        // First, for every set of backtrack points, we split on every
        // point in that N+1-sized set (the backtrack points include
        // start/end points), and create an N-sized vector splitlocs of
        // resulting analyses
        LocationVectorVector splitlocs;
        hfst::StringVector words = split_at(in_syms, &bt_points);
        for(const auto & word : words) {
            // Trim left/right spaces:
            const size_t first = find_first_not_of_def(word, ' ', 0);
            const size_t last = 1 + find_last_not_of_def(word, ' ', word.length() - 1);
            string form = word.substr(first, last-first);
            LocationVector loc = locate_fullmatch(container, form, s);
            if(loc.size() == 0 && s.verbose) {
                std::cerr << "Warning: The analysis of \"<" << locations.at(0).input << ">\" has backtracking around the substring \"<" << form << ">\", but that substring has no analyses." << std::endl;
                // but push it anyway, since we want exactly one subvector per splitpoint
            }
            if(form.length() != word.length()) { // Ensure the spaces we ignored when looking up are output in the form:
                vector<string> lspace = vector<string>(first, " ");
                vector<string> rspace = vector<string>(word.length()-last, " ");
                for(auto & lvit : loc) {
                    lvit.input = form;
                    vector<string>& syms = lvit.input_symbol_strings;
                    syms.insert(syms.begin(), lspace.begin(), lspace.end());
                    syms.insert(syms.end(), rspace.begin(), rspace.end());
                    for(unsigned long & input_part : lvit.input_parts) {
                        input_part += first;
                    }
                }
            }
            splitlocs.push_back(loc);
        }
        if(splitlocs.empty()) {
            continue;
        }
        // Second, we reorder splitlocs so we can output as a
        // cohort of non-branching CG subreadings; first word as leaf
        // nodes. This means that splitlocs = [[A,B],[C,D]] should
        // end up as the sequence
        // (C,0),(A,1),(C,0),(B,1),(D,0),(A,1),(D,0),(B,1)
        // (where the number is the initial indentation).
        size_t depth = 0;
        const size_t bottom = splitlocs.size()-1;
        vector<std::ostringstream> out(splitlocs.size());
        vector<pair<LocationVector, size_t > > stack;
        // In CG the *last* word is the least indented, so start from
        // the end of splitlocs, indentation being 1 tab:
        stack.push_back(make_pair(splitlocs.at(bottom),
                                  0));
        while(!stack.empty() && !stack.back().first.empty()) {
            LocationVector & locs = stack.back().first;
            const Location loc = locs.back();
            locs.pop_back();
            const size_t indent = 1 + stack.back().second;
            out.at(depth).clear();
            out.at(depth).str(string());
            // (ignore splitpoints of splitpoints)
            const size_t new_indent = print_reading_giellacg(&loc, indent, true, out.at(depth), s).second;
            if(depth == bottom) {
                for(const auto & it : out) {
                    outstream << it.str();
                }
            }
            if(depth < bottom) {
                ++depth;
                if(depth > 0) {
                    stack.push_back(make_pair(splitlocs.at(bottom-depth),
                                              new_indent));
                }
            }
            else if(locs.empty()) {
                depth--;
                stack.pop_back();
            }
        }
    }
}

// Omorfi-specific at this time
std::string fetch_and_kill_between(std::string left, std::string right, std::string & analysis)
{
    size_t start = analysis.find(left);
    size_t stop = analysis.find(right, start + 1);
    if (start == std::string::npos || stop == std::string::npos) {
        return "";
    }
    std::string retval = analysis.substr(start + left.size(), stop - start - left.size());
    analysis.erase(start, stop - start + right.size());
    return retval;
}

std::string fetch_and_kill_feats(std::string & analysis)
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
    if (retval.size() != 0) {
        retval.erase(retval.size() - 1);
    }
    return retval;
}

std::string empty_to_underscore(std::string to_test)
{
    if (to_test.size() == 0) {
        return "_";
    }
    return to_test;
}


void print_location_vector(hfst_ol::PmatchContainer & container,
                           LocationVector const & locations,
                           std::ostream & outstream,
                           int token_number,
                           const TokenizeSettings& s)
{
    if (s.output_format == tokenize && locations.size() != 0) {
        outstream << locations.at(0).input;
        if (s.print_weights) {
            outstream << "\t" << locations.at(0).weight;
        }
        outstream << std::endl;
        if (locations.at(0).tag == "<Boundary=Sentence>") {
            outstream << std::endl;
        }
    } else if (s.output_format == space_separated && locations.size() != 0) {
	outstream << locations.at(0).input;
        if (s.print_weights) {
            outstream << "\t" << locations.at(0).weight;
        }
        outstream << " ";
        if (locations.at(0).tag == "<Boundary=Sentence>") {
            outstream << std::endl;
        }
    } else if (s.output_format == cg && locations.size() != 0) {
        // Print the cg cohort header
        outstream << "\"<";
        print_escaping_backslashes(locations.at(0).input, outstream);
        outstream << ">\"" << std::endl;
        for (const auto & location : locations) {
            // For the most common case, eg. analysis strings that begin with the original input,
            // we try to do what cg tools expect and surround the original input with double quotes.
            // Otherwise we omit the double quotes and assume the rule writer knows what he's doing.
            if (location.output.find(location.input) == 0) {
                // The nice case obtains
                outstream << "\t\"";
                print_escaping_backslashes(location.input, outstream);
                outstream << "\"" <<
                    location.output.substr(location.input.size(), std::string::npos);
            } else {
                outstream << "\t" << location.output;
            }
            if (s.print_weights) {
                outstream << "\t" << location.weight;
            }
            outstream << std::endl;
        }
        outstream << std::endl;
    } else if (s.output_format == giellacg && locations.size() != 0) {
        print_location_vector_giellacg(container, locations, outstream, s);
    } else if (s.output_format == visl && locations.size() != 0) {
        print_location_vector_giellacg(container, locations, outstream, s);
    } else if (s.output_format == xerox) {
        float best_weight = std::numeric_limits<float>::max();
        for (const auto & location : locations) {
            if (best_weight > location.weight) {
                best_weight = location.weight;
            }
        }
        bool printed_something = false;
        for (LocationVector::const_iterator loc_it = locations.begin();
             loc_it != locations.end(); ++loc_it) {
            if ((s.beam < 0.0 || loc_it->weight <= best_weight + s.beam) &&
                // We don't print "plain" tokens without any analysis
                // except if they are the only one present
                (loc_it->output.compare(loc_it->input) != 0 ||
                 (loc_it + 1 == locations.end() && !printed_something))) {
                outstream << loc_it->input << "\t" << loc_it->output;
                if (s.print_weights) {
                    if (loc_it + 1 == locations.end() && !printed_something) {
                        outstream << "\t" << best_weight;
                    } else {
                        outstream << "\t" << loc_it->weight;
                    }
                }
                outstream << std::endl;
                printed_something = true;
            }
        }
        if (locations.at(0).tag == "<Boundary=Sentence>") {
            outstream << std::endl;
        }
        outstream << std::endl;
    } else if (s.output_format == conllu) {
        hfst_ol::Weight lowest_weight = hfst_ol::INFINITE_WEIGHT;
        hfst_ol::Location best_location;
        for (const auto & location : locations) {
            if (location.weight < lowest_weight) {
                best_location = location;
                lowest_weight = location.weight;
            }
//            if (loc_it->tag == "@MULTIWORD@"
//            outstream << loc_it->input << "\t" << loc_it->output;
        }
        outstream << token_number
                  << "\t" << best_location.input;
        outstream << "\t" << empty_to_underscore(fetch_and_kill_between("[WORD_ID=", "]", best_location.output));
        outstream << "\t" << empty_to_underscore(fetch_and_kill_between("[UPOS=", "]", best_location.output));
        outstream << "\t" << empty_to_underscore(fetch_and_kill_between("[XPOS=", "]", best_location.output));
        outstream << "\t" << empty_to_underscore(fetch_and_kill_feats(best_location.output))
                  << "\t" << "_" // HEAD
                  << "\t" << "_" // DEPREL
                  << "\t" << "_"; // DEPS
        outstream << "\t" << empty_to_underscore(best_location.output); // MISC
                    if (s.print_weights) {
                outstream << "\t" << best_location.weight;
            }
        outstream << std::endl;
    } else if (s.output_format == finnpos) {
        std::set<std::string> tags;
        std::set<std::string> lemmas;
            for (const auto & location : locations) {
                // Assume the last space is where the tags begin
                size_t tags_start_at = location.output.find_last_of(" ");
                if (tags_start_at != std::string::npos) {
                    std::string lemma = location.output.substr(0, tags_start_at);
                    if (lemma.find_first_of(" ") == std::string::npos) {
                        // can't have spaces in lemmas
                        lemmas.insert(lemma);
                    }
                    std::string tag = location.output.substr(tags_start_at + 1);
                    if (tag.find_first_of(" ") == std::string::npos) {
                        // or tags
                        tags.insert(tag);
                    }
                }
            }
        outstream << locations.at(0).input << "\t_\t";
        // the input and a blank for features
        if (lemmas.empty()) {
            outstream << "_";
        } else {
            std::string accumulator;
            for (const auto & lemma : lemmas) {
                accumulator.append(lemma);
                accumulator.append(" ");
            }
            outstream << accumulator.substr(0, accumulator.size() - 1);
        }
        outstream << "\t";
        if (tags.empty()) {
            outstream << "_";
        } else {
            std::string accumulator;
            for (const auto & tag : tags) {
                accumulator.append(tag);
                accumulator.append(" ");
            }
            outstream << accumulator.substr(0, accumulator.size() - 1);
        }
        outstream << "\t_" << std::endl;
        if (locations.at(0).tag == "<Boundary=Sentence>") {
            outstream << std::endl;
        }
    }
//    std::cerr << "from print_location_vector\n";
}

void match_and_print(hfst_ol::PmatchContainer & container,
                     std::ostream & outstream,
                     const string & input_text,
                     const TokenizeSettings& s)
{
    LocationVectorVector locations = container.locate(input_text, s.time_cutoff);
    if (locations.size() == 0 && s.print_all) {
        print_no_output(input_text, outstream, s);
        return;
    }
    int token_number = 1;
    for(const auto & location : locations) {
        if ((location.size() == 1 && location.at(0).output.compare("@_NONMATCHING_@") == 0)) {
            if (s.print_all) {
                print_nonmatching_sequence(location.at(0).input, outstream, s);
            }
            continue;
            // All nonmatching cases have been handled
        }
        print_location_vector(container,
                              keep_n_best_weight(dedupe_locations(location, s), s),
                              outstream,
                              token_number,
                              s);
        ++token_number;
    }
    if (s.output_format == finnpos || s.output_format == tokenize || s.output_format == xerox) {
        outstream << std::endl;
    }
}

void process_input(hfst_ol::PmatchContainer & container,
                   std::istream& instream,
                   std::ostream& outstream,
                   const TokenizeSettings& s)
{
    container.set_single_codepoint_tokenization(!s.tokenize_multichar);
    const size_t bufsize = 4096;
    for(char line[bufsize]; instream.getline(line, bufsize); ) {
        string input_text(line);
        if(!input_text.empty()) {
            match_and_print(container, outstream, input_text, s);
        }
    }
}

}
