#ifndef MAIN_H
#define MAIN_H

#define EXTERN

#ifdef __cplusplus
#undef EXTERN
#define EXTERN extern "C"
#include <set>
#include <string>
#include <vector>

namespace hfst
{
class HfstTransducer;
class HfstInputStream;
typedef std::vector<std::string> StringVector;
typedef std::pair<float, StringVector> HfstOneLevelPath;
typedef std::set<HfstOneLevelPath> HfstOneLevelPaths;
}

#endif /* __cplusplus */

#include <stdlib.h>

// typedef struct HfstValue {
//     float weight;
//     char *s;
// } HfstValue;

// maybe these

/**
 * Create an empty transducer. Returns pointer
 */
EXTERN void *hfst_empty_transducer();
typedef void *hfst_empty_transducer_t();

/**
 * Release transducers memory.
 */
EXTERN void *hfst_transducer_free(const void *t);
typedef void *hfst_transducer_free_t(void *);

/**
 * Open a file containing a binary hfst file as an HfstInputStream.
 * Returns NULL on failure.
 */
EXTERN void *hfst_input_stream(const char *path);
typedef void *hfst_input_stream_t(const char *filename);

/**
 * Close the stream. This corresponds to the close method of the underlying
 * C++ HfstInputStream.
 */
EXTERN void hfst_input_stream_close(const void *input_stream);
typedef void *hfst_input_stream_close_t(void *);

/**
 * Free the stream. This corresponds roughly to a destructor of the underlying
 * C++ HfstInputStream.
 */
EXTERN void hfst_input_stream_free(const void *input_stream);
typedef void *hfst_input_stream_free_t(void *);

/**
 * Check if the HfstInputStream is at EOF.
 */
EXTERN bool hfst_input_stream_is_eof(const void *);
typedef bool hfst_input_stream_is_eof_t(void *);

/**
 * Check if the HfstInputStream is at "bad" (this is really a check on the
 * underlying c++ "stream" having gone "bad", right?)
 */
EXTERN bool hfst_input_stream_is_bad(const void *);
typedef bool hfst_input_stream_is_bad_t(void *);

/**
 * Extract a transducer from the stream.
 */
EXTERN void *hfst_transducer_from_stream(const void *input_stream);
typedef void *hfst_transducer_from_stream_t(void *);

// anders: These two functions are problematic in how they iterate over
// the results, when it comes to memory management. I am not sure about the
// other ones either...
EXTERN void *hfst_lookup_begin(const void *);
typedef void *hfst_lookup_begin_t(const void *);

EXTERN size_t hfst_lookup_results(const void *, char **, float *);
typedef size_t hfst_lookup_results_t(void *, char **, float *);

/**
 * Look up a string in the transducer.
 */
EXTERN void *hfst_lookup(void *transducer, const char *input);
typedef void *hfst_lookup_t(void *, const char *);

/**
 * An iterator over the results of a lookup.
 */
typedef struct ResultIterator
{
    void *begin;
    void *end;
} ResultIterator;

/**
 * Retrieve an iterator from the lookup handle.
 */
EXTERN ResultIterator *hfst_lookup_iterator(void *lookup);
typedef ResultIterator *hfst_lookup_iterator_t(void *);

/**
 * Get the current value of the iterator. The string is placed in argument
 * 's', while the weight goes in 'w'.
 */
EXTERN void hfst_lookup_iterator_value(ResultIterator *it, char **s, float *w);
typedef void hfst_lookup_iterator_value_t(ResultIterator *it, char **s,
                                          float *weight);

/**
 * Advance the lookup iterator to the next value.
 */
EXTERN void hfst_lookup_iterator_next(ResultIterator *it);
typedef void hfst_lookup_iterator_next_t(ResultIterator *);

/**
 * Free the resources used by the iterator.
 */
EXTERN void hfst_lookup_iterator_free(ResultIterator *it);
typedef void hfst_lookup_iterator_free_t(ResultIterator *it);

/**
 * Check if the iterator is done.
 */
EXTERN bool hfst_lookup_iterator_done(struct ResultIterator *it);
typedef bool hfst_lookup_iterator_done_t(struct ResultIterator *it);

#endif /* MAIN_H */
