#include "implementations/optimized-lookup/pmatch.h"
#include "implementations/optimized-lookup/pmatch_tokenize.h"

int main(int argc, char **argv)
{
	std::cerr << "loading " << std::endl;
	std::ifstream s = std::ifstream("tokeniser.pmhfst");
	std::unique_ptr<hfst_ol::PmatchContainer> tok(new hfst_ol::PmatchContainer(s));
	std::cerr << "loaded " << std::endl;
        return EXIT_SUCCESS;
}

