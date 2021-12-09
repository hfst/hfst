#include "implementations/optimized-lookup/pmatch.h"
#include "implementations/optimized-lookup/pmatch_tokenize.h"
#include "auxiliary_functions.cc"

using namespace hfst_ol;

int main(int argc, char **argv)
{
  verbose_print("PmatchContainer load");
  std::ifstream instream("./tokeniser.pmhfst");
  auto * t = new hfst_ol::PmatchContainer(instream);
}

