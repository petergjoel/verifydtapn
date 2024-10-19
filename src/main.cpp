
#include "verifydtapn.h"
#include "Core/ArgsParser.hpp"
#include "DiscreteVerification/DiscreteVerification.hpp"

#include <iostream>
#include <fstream>
#include <memory>

using namespace VerifyTAPN;
using namespace VerifyTAPN::TAPN;



int main(int argc, char *argv[]) {
    srand(time(nullptr));

    ArgsParser parser;
    VerificationOptions options = parser.parse(argc, argv);

    std::unique_ptr<std::stringstream> output_stream = std::make_unique<std::stringstream>();
    unfoldtacpn::ColoredPetriNetBuilder builder(options.getPrintBindings() ? output_stream.get() : nullptr);

    auto [initialPlacement, tapn] = parse_net_file(builder, options.getInputFile());
    if(!options.getOutputModelFile().empty())
    {
        std::fstream of(options.getOutputModelFile(), std::ios::out);
        tapn->toTAPNXML(of, initialPlacement);
        of.close();
    }

    tapn->initialize(options);

    if (options.getCalculateCmax()) {
        std::cout << options << std::endl;
        std::cout << "C-MAX" << std::endl;
        for (auto* iter : tapn->getPlaces()) {
            std::cout << '<' << iter->getName() << '#' << iter->getMaxConstant() << "> ";
        }
        std::cout << std::endl;
        return 0;
    }

    if (options.getPrintBindings()) {
        std::cout << output_stream.get()->str();
    }
    
    std::unique_ptr<AST::Query> query = make_query(builder, options, *tapn);
    assert(query);


    if(options.getSearchType() == VerificationOptions::OverApprox)
    {
        std::cout << "Verification-mode is OverApprox, terminating, no more to do!" << std::endl;
        return 0;
    }

    if (tapn->containsOrphanTransitions()) {
        std::cout << "The model contains orphan transitions. This is not supported by the engine." << std::endl;
        return 1;
    }

    if (query->getQuantifier() == AST::CF || query->getQuantifier() == AST::CG) {
        options.setKeepDeadTokens(true);
    }

    tapn->updatePlaceTypes(query.get(), options);

    int result = DiscreteVerification::DiscreteVerification::run(*tapn, initialPlacement, query.get(), options);

    return result;
}


