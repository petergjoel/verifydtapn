#include "DiscreteVerification/VerificationTypes/ProbabilityFloatComparison.hpp"
#include "DiscreteVerification/QueryVisitor.hpp"

#include <iostream>

namespace VerifyTAPN::DiscreteVerification {

ProbabilityFloatComparison::ProbabilityFloatComparison(
    TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
)
: SMCVerification(tapn, initialMarking, query, options), validRuns(0), ratio(0)
{ 
    computeAcceptingBounds(smcSettings.falsePositives, smcSettings.falsePositives);
    computeIndifferenceRegion(smcSettings.geqThan, smcSettings.indifferenceRegionUp, smcSettings.indifferenceRegionDown);
}

void ProbabilityFloatComparison::handleRunResult(const bool res, int steps, double delay, unsigned int thread_id) {
    bool valid = query->getQuantifier() == PG ? !res : res;
    if(p0 >= 1.0f && !valid) {
        ratio = std::numeric_limits<float>::infinity();
    } else {
        ratio += valid ? log(p1 / p0) : log((1 - p1) / (1 - p0));
    }
    validRuns += (int) valid;
}

bool ProbabilityFloatComparison::handleSuccessor(RealMarking* marking) {
    QueryVisitor<RealMarking> checker(*marking, tapn);
    AST::BoolResult context;
    query->accept(checker, context);

    delete marking;

    return context.value;
}

bool ProbabilityFloatComparison::mustDoAnotherRun() {
    if(ratio <= boundH0) {
        result = true;
        return false;
    } else if(ratio >= boundH1) {
        result = false;
        return false;
    }
    return true;
}

bool ProbabilityFloatComparison::getResult() {
    return result;
}

void ProbabilityFloatComparison::computeAcceptingBounds(const float alpha, const float beta) {
    boundH0 = log(beta / (1 - alpha));
    boundH1 = log((1 - beta) / alpha);
} 

void ProbabilityFloatComparison::computeIndifferenceRegion(const float p, const float sigma0, const float sigma1) {
    p0 = std::clamp(0.0f, p + sigma0, 1.0f);
    p1 = std::clamp(0.0f, p - sigma1, 1.0f);
} 

void ProbabilityFloatComparison::printStats() {
    SMCVerification::printStats();
    std::cout << "  valid runs:\t" << validRuns << std::endl;
}

void ProbabilityFloatComparison::printResult() {
    /*if (options.getXmlTrace()) {
        printXMLTrace(m, printStack, query, tapn);
    } else {
        printHumanTrace(m, printStack, query->getQuantifier());
    }*/
    bool result = getResult();
    std::cout << "Probability comparison:" << std::endl;
    std::cout << "\tQuery: P >= " << smcSettings.geqThan << std::endl;
    std::cout << "\tIndifference region: [" << p1 << "," << p0 << "]" << std::endl;
    std::cout << "\tFalse positives: " << smcSettings.falsePositives << std::endl;
    std::cout << "\tFalse negatives: " << smcSettings.falseNegatives << std::endl;
	std::cout << (result ? "\tHypothesis is satisfied" : "\tHypothesis is NOT satisfied") << std::endl;
}

}
