@echo OFF

rem SET debug_compile=
rem SET debug_link=
rem if "%1%"=="/DEBUG" SET debug_compile=/Zi /O2 /bigobj /MDd
rem if "%1%"=="/DEBUG" SET debug_link=/DEBUG /release
rem copy ..\..\back-ends\foma\libfoma.* .
rem copy ..\..\back-ends\openfstwin\src\lib\openfst.* .

if not exist libhfst.i echo Error: missing file "libhfst.i" && ^
exit /B

if not exist C:\python33\libs\python33.lib echo Error: missing file "C:\python33\libs\python33.lib" && ^
exit /B

if not exist C:\swigwin-3.0.5\swig.exe echo Error: missing executable "C:\swigwin-3.0.5\swig.exe" && ^
exit /B

if not exist "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include" echo Error: missing include directory "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include" && ^
exit /B

if exist "..\..\back-ends\foma\flags.c" echo Renaming foma's flags.c into _flags.c as there is a file with the same name in openfst ^
move ..\..\back-ends\foma\flags.c ..\..\back-ends\foma\_flags.c

@echo ON

copy C:\python33\libs\python33.lib .

C:\swigwin-3.0.5\swig.exe -python -c++ -I"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include" -Wall -o _libhfst.cpp libhfst.i

cl /EHsc /LD /Fe_libhfst.pyd ^
/D HAVE_FOMA /D HAVE_OPENFST /D HFSTEXPORT /D OPENFSTEXPORT /D_MSC_VER /DWINDOWS /DWIN32 ^
/I..\..\libhfst\src /I..\..\back-ends\foma /I..\..\back-ends /I..\..\back-ends\openfstwin\src\include /I ..\.. ^
/Iparsers /I..\.. /I..\..\libhfst\src /I..\..\libhfst\src\parsers /I..\..\back-ends\foma ^
/I..\..\back-ends /I..\..\back-ends\openfstwin\src\include /IC:\Python33\include\ ^
HfstApply.cpp ^
HfstInputStream.cpp ^
HfstTransducer.cpp ^
HfstOutputStream.cpp ^
HfstRules.cpp ^
HfstXeroxRules.cpp ^
HfstDataTypes.cpp ^
HfstSymbolDefs.cpp ^
HfstTokenizer.cpp ^
HfstFlagDiacritics.cpp ^
HfstExceptionDefs.cpp ^
HarmonizeUnknownAndIdentitySymbols.cpp ^
HfstLookupFlagDiacritics.cpp ^
HfstEpsilonHandler.cpp ^
HfstStrings2FstTokenizer.cpp ^
HfstPrintDot.cpp ^
HfstPrintPCKimmo.cpp ^
hfst-string-conversions.cpp ^
string-utils.cpp ^
implementations\HfstBasicTransducer.cpp ^
implementations\HfstBasicTransition.cpp ^
implementations\ConvertTransducerFormat.cpp ^
implementations\HfstTropicalTransducerTransitionData.cpp ^
implementations\ConvertTropicalWeightTransducer.cpp ^
implementations\ConvertLogWeightTransducer.cpp ^
implementations\ConvertFomaTransducer.cpp ^
implementations\ConvertOlTransducer.cpp ^
implementations\TropicalWeightTransducer.cpp ^
implementations\LogWeightTransducer.cpp ^
implementations\FomaTransducer.cpp ^
implementations\HfstOlTransducer.cpp ^
implementations\compose_intersect\ComposeIntersectRulePair.cpp ^
implementations\compose_intersect\ComposeIntersectLexicon.cpp ^
implementations\compose_intersect\ComposeIntersectRule.cpp ^
implementations\compose_intersect\ComposeIntersectFst.cpp ^
implementations\compose_intersect\ComposeIntersectUtilities.cpp ^
implementations\optimized-lookup\transducer.cpp ^
implementations\optimized-lookup\convert.cpp ^
implementations\optimized-lookup\ospell.cpp ^
implementations\optimized-lookup\pmatch.cpp ^
implementations\optimized-lookup\find_epsilon_loops.cpp ^
parsers\xre_lex.cpp ^
parsers\xre_parse.cpp ^
parsers\pmatch_parse.cpp ^
parsers\pmatch_lex.cpp ^
parsers\lexc-parser.cpp ^
parsers\lexc-lexer.cpp ^
parsers\xfst-parser.cpp ^
parsers\xfst-lexer.cpp ^
parsers\LexcCompiler.cpp ^
parsers\PmatchCompiler.cpp ^
parsers\XreCompiler.cpp ^
parsers\XfstCompiler.cpp ^
parsers\lexc-utils.cpp ^
parsers\pmatch_utils.cpp ^
parsers\xre_utils.cpp ^
parsers\xfst-utils.cpp ^
parsers\xfst_help_message.cpp ^
..\..\back-ends\openfstwin\src\lib\compat.cpp ^
..\..\back-ends\openfstwin\src\lib\flags.cpp ^
..\..\back-ends\openfstwin\src\lib\fst.cpp ^
..\..\back-ends\openfstwin\src\lib\properties.cpp ^
..\..\back-ends\openfstwin\src\lib\symbol-table.cpp ^
..\..\back-ends\openfstwin\src\lib\symbol-table-ops.cpp ^
..\..\back-ends\openfstwin\src\lib\util.cpp ^
..\..\back-ends\foma\*.c ^
_libhfst.cpp

