#include "hfst.h"
#include <iterator>
#include <sstream>

// Here we declare the C interface.
//
// Remember, these functions are still C++ functions (we are in a C++ file),
// so internally, they can use C++ code to do their thing.
//
// What's important is that the arguments to the functions, and the
// return type, is types that C can deal with.
// Usually this means that all C++ objects become void pointers, which
// are then casted to the C++ object they are when the function begins.
//
// I am not totally sure this is the best approach, but it seems to work.

extern "C"
{
#include <string.h>

    typedef struct ResultIterator
    {
        void *begin;
        void *end;
    } ResultIterator;

    typedef struct HfstValue
    {
        float weight;
        char *s;
    } HfstValue;

    void *
    hfst_empty_transducer()
    {
        hfst::HfstTransducer *fsa = new hfst::HfstTransducer();
        return (void *)fsa;
    }

    void
    hfst_transducer_free(void *t)
    {
        assert(t != nullptr);
        delete static_cast<hfst::HfstTransducer *>(t);
    }

    void *
    hfst_input_stream(const char *filename)
    {
        hfst::HfstInputStream *is = nullptr;
        try
        {
            is = new hfst::HfstInputStream(filename);
        }
        // Rust cannot handle C++ exceptions (can even C?), so make very sure
        // we never throw one. This is C++-speak for "catch all exceptions".
        catch (...)
        {
        }
        return (void *)(is);
    }

    void
    hfst_input_stream_free(void *input_stream)
    {
        assert(input_stream != nullptr);
        delete static_cast<hfst::HfstInputStream *>(input_stream);
        input_stream = nullptr;
    }

    void
    hfst_input_stream_close(void *his)
    {
        hfst::HfstInputStream *inp = static_cast<hfst::HfstInputStream *>(his);
        inp->close();
    }

    bool
    hfst_input_stream_is_eof(void *his)
    {
        hfst::HfstInputStream *inp = static_cast<hfst::HfstInputStream *>(his);
        return inp->is_eof();
    }

    bool
    hfst_input_stream_is_bad(void *his)
    {
        hfst::HfstInputStream *inp = static_cast<hfst::HfstInputStream *>(his);
        return inp->is_bad();
    }

    void *
    hfst_transducer_from_stream(void *his)
    {
        hfst::HfstInputStream *inp = static_cast<hfst::HfstInputStream *>(his);
        hfst::HfstTransducer *fsa = new hfst::HfstTransducer(*inp);
        return static_cast<void *>(fsa);
    }

    void *
    hfst_lookup(void *self, const char *s)
    {
        hfst::HfstTransducer *fsa = static_cast<hfst::HfstTransducer *>(self);
        hfst::HfstOneLevelPaths *rv = fsa->lookup(s);
        return static_cast<void *>(rv);
    }

    size_t
    hfst_lookup_results(void *holps, void **results, float *weights)
    {
        hfst::HfstOneLevelPaths *v
            = static_cast<hfst::HfstOneLevelPaths *>(holps);
        size_t i = 0;
        for (auto it : *v)
        {
            hfst::StringVector sv = it.second;
            // anders: where will this malloc be free()'d?
            char *result = static_cast<char *>(malloc(sizeof(char) * 256));
            result[0] = 0;
            for (auto s : sv)
            {
                // XXX:
                result = strcat(result, s.c_str());
            }
            results[i] = result;
            weights[i] = it.first;
            i++;
        }
        return i;
    }

    // Create an iterator over the paths in holps, put it on the heap,
    // and return the pointer to it
    struct ResultIterator *
    hfst_lookup_iterator(void *holps)
    {
        // hfst::HfstOneLevelPaths is really:
        // std::set<
        //     std::pair<
        //         float,
        //         std::vector<std::__cxx11::basic_string<char>>
        //      >
        //  >::iterator
        // hfst::HfstOneLevelPaths::iterator b(v->begin());
        // hfst::HfstOneLevelPaths::iterator e(v->begin());
        // auto b_heap = new hfst::HfstOneLevelPaths::iterator(b);
        // auto e_heap = new hfst::HfstOneLevelPaths::iterator(e);
        hfst::HfstOneLevelPaths *v
            = static_cast<hfst::HfstOneLevelPaths *>(holps);
        ResultIterator *s = (ResultIterator *)malloc(sizeof(ResultIterator));
        s->begin = static_cast<void *>(
            new hfst::HfstOneLevelPaths::iterator(v->begin()));
        s->end = static_cast<void *>(
            new hfst::HfstOneLevelPaths::iterator(v->end()));
        return s;
    }

    bool
    hfst_lookup_iterator_done(struct ResultIterator *it)
    {
        auto begin
            = static_cast<hfst::HfstOneLevelPaths::iterator *>(it->begin);
        auto end = static_cast<hfst::HfstOneLevelPaths::iterator *>(it->end);
        return *begin == *end;
    }

    void
    hfst_lookup_iterator_free(struct ResultIterator *it)
    {
        delete static_cast<hfst::HfstOneLevelPaths::iterator *>(it->begin);
        delete static_cast<hfst::HfstOneLevelPaths::iterator *>(it->end);
        free(it);
    }

    void
    hfst_lookup_iterator_value(ResultIterator *it, char **s, float *weight)
    {
        // extract the iterator
        auto begin
            = static_cast<hfst::HfstOneLevelPaths::iterator *>(it->begin);
        auto pair = **begin;

        // the float can just be copied in, easy
        *weight = pair.first;

        // but for the string, we do more work:
        // (1) build a long, temporary string by concatenating all the
        // substrings in the vec<string>
        // according to the internet, this is how to do that:
        std::ostringstream os;
        std::copy(pair.second.begin(), pair.second.end(),
                  std::ostream_iterator<std::string>(os));
        // anders: returning this directy (as in: *s = os.str().c_str()),
        // doesn't work. Maybe C++ deallocates the temporary before the
        // function returns
        std::string full = os.str();

        // (2) copy the temporary string to some memory that we control.
        // the caller is responsible for free()ing this returned string
        size_t len = full.size();
        char *our = (char *)malloc(len + 1);
        strncpy(our, full.c_str(), len);
        our[len] = 0;
        *s = our;
    }

    void
    hfst_lookup_iterator_next(ResultIterator *it)
    {
        auto begin
            = static_cast<hfst::HfstOneLevelPaths::iterator *>(it->begin);
        (*begin)++;
    }
}
