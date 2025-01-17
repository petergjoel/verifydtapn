#include "DiscreteVerification/VerificationTypes/SMCTracesGenerator.hpp"
#include "DiscreteVerification/QueryVisitor.hpp"

#include <math.h>

namespace VerifyTAPN::DiscreteVerification {

SMCTracesGenerator::SMCTracesGenerator(
    TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
)
: SMCVerification(tapn, initialMarking, query, options)
{
}

bool SMCTracesGenerator::mustDoAnotherRun() {
    return mustSaveTrace();
}

void SMCTracesGenerator::handleRunResult(const bool res, int steps, double delay, unsigned int thread_id)
{
    
}

bool SMCTracesGenerator::handleSuccessor(RealMarking* marking) {
    QueryVisitor<RealMarking> checker(*marking, tapn);
    AST::BoolResult context;
    query->accept(checker, context);
    delete marking;
    return context.value;
}

void SMCTracesGenerator::printResult() {
    std::cout << "Generated " << traces.size() << " random traces" << std::endl;
}

void SMCTracesGenerator::printWatchStats() {
    std::vector<Observable>& obs = getSmcQuery()->getObservables();
    for(int run_i = 0 ; run_i < savedWatchs.size() ; run_i++) {
        std::vector<Watch>& runWatchs = savedWatchs[run_i];
        std::cout << "observations of run " << run_i << ":" << std::endl;
        for(int i = 0 ; i < obs.size() ; i++) {
            std::string plot = runWatchs[i].get_plots(std::get<0>(obs[i]));
            std::cout << plot << std::endl;
        }
    }
}

void SMCTracesGenerator::handleTrace(const bool runRes, SMCRunGenerator* generator) {
    if(generator == nullptr) generator = &runGenerator; 
    bool valid = query->getQuantifier() == PG ? !runRes : runRes;
    VerificationOptions::SMCTracesType type = options.getSMCTracesType();
    if(
        (type == VerificationOptions::ANY_TRACE) || 
        (type == VerificationOptions::SATISFYING_TRACES && valid) ||
        (type == VerificationOptions::UNSATISFYING_TRACES && !valid)
    ) {
        saveTrace(generator);
        saveWatchs(generator->_thread_id);
    } else {
        clearWatchs(generator->_thread_id);
    }
}

void SMCTracesGenerator::saveWatchs(const unsigned int thread_id) {
    std::vector<Watch> toSave;
    for(int i = 0 ; i < watchs.size() ; i++) {
        Watch& w = watchs[i][thread_id];
        toSave.push_back(w);
        w.reset();
    }
    savedWatchs.push_back(toSave);
}

void SMCTracesGenerator::clearWatchs(const unsigned int thread_id) {
    for(int i = 0 ; i < watchs.size() ; i++) {
        Watch& w = watchs[i][thread_id];
        w.reset();
    }
}

}
