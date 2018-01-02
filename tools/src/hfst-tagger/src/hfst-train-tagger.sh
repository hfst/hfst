#! /bin/sh

BINDIR/hfst-open-input-file-for-tagger "$@" |
BINDIR/hfst_tagger_compute_data_statistics.py "$@" |
BINDIR/hfst-build-tagger "$@"
