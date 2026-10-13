# -*- coding: utf-8 -*-
"""Smoke test for a built hfst wheel.

Used as the cibuildwheel ``test-command``: it verifies that the compiled
extension imports and that core functionality works on the target platform
(all three back-ends linked in, regex/lookup, tokenizer, ICU/UTF-8). It is
deliberately small and API-current; the full functional suite (test.sh) is run
by the C++ project's ``make check``.
"""
import sys

import hfst

print("hfst version:", hfst.__version__)

# Desktop wheels include all three back-ends. Pyodide follows the wasm build and
# disables SFST, but still exercises OpenFst and foma.
missing = []
required_backends = ["TROPICAL_OPENFST_TYPE", "FOMA_TYPE"]
if sys.platform != "emscripten":
    required_backends.insert(0, "SFST_TYPE")
for name in required_backends:
    impl = getattr(hfst.ImplementationType, name)
    if not hfst.HfstTransducer.is_implementation_type_available(impl):
        missing.append(name)
if missing:
    sys.exit("Missing back-ends: " + ", ".join(missing))

# regex + lookup
tr = hfst.regex('foo:bar')
assert tr.lookup('foo') == (('bar', 0.0),), tr.lookup('foo')

# tokenizer (current API: split_symbols is a required bool)
tok = hfst.HfstTokenizer()
tok.add_multichar_symbol('foo')
assert tok.tokenize('foobar', False) == \
    (('foo', 'foo'), ('b', 'b'), ('a', 'a'), ('r', 'r'))

# ICU / UTF-8 handling
hfst.HfstTokenizer.check_utf8_correctness('föö')

# Exception translation: C++ HfstExceptions must surface as catchable,
# correctly-typed Python exceptions (regression guard for the binding fix).
import hfst.exceptions
assert issubclass(hfst.exceptions.HfstException, Exception)
if (hfst.HfstTransducer.is_implementation_type_available(hfst.ImplementationType.FOMA_TYPE)
        and hfst.HfstTransducer.is_implementation_type_available(hfst.ImplementationType.TROPICAL_OPENFST_TYPE)):
    a = hfst.regex('foo'); a.convert(hfst.ImplementationType.FOMA_TYPE)
    b = hfst.regex('bar'); b.convert(hfst.ImplementationType.TROPICAL_OPENFST_TYPE)
    try:
        a.concatenate(b)
        raise SystemExit("expected TransducerTypeMismatchException")
    except hfst.exceptions.TransducerTypeMismatchException:
        pass

print("wheel smoke test OK")
