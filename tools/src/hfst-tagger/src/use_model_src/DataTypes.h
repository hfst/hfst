#ifndef HEADER_DataTypes_h
#define HEADER_DataTypes_h

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unordered_map>
using std::unordered_map;

#include <string>
#include <vector>

typedef int State;
typedef int Symbol;
typedef float Weight;

struct TransitionData
{
  Weight weight;
  State  target;
};


typedef unordered_map<std::string,Symbol> Symbol2NumberMap;
typedef std::vector<std::string>               Number2SymbolMap;
typedef std::vector<Weight> StateFinalWeightMap;

typedef unordered_map<Symbol,TransitionData> Symbol2TransitionDataMap;
typedef std::vector<Symbol2TransitionDataMap>     TransitionMap;

typedef std::pair<size_t,Weight> IdWeightPair;

typedef unordered_map<std::string,IdWeightPair> StringWeightMap;
typedef unordered_map<std::string,float> ProbabilityMap;

#define DEFAULT_SYMBOL "<NONE>"
#define DEFAULT 1

class InvalidState
{};

class InvalidKey
{};

class BrokenLexicalModel
{};

typedef std::pair<std::string,std::string> StringPair;
typedef std::pair<std::string,std::string> SymbolPair;
typedef std::vector<StringPair> StringPairVector;
typedef std::vector<Symbol> SymbolVector;

typedef std::pair<Weight,StringPairVector> WeightedStringPairVector;
typedef std::pair<Weight,SymbolVector> WeightedSymbolVector;

typedef std::pair<Weight,std::string> WeightedString;
typedef std::vector<WeightedString> WeightedStringVector;

#endif // HEADER_DataTypes_h
