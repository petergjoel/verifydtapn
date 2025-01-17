#ifndef SMC_TRACES_GENERATOR
#define SMC_TRACES_GENERATOR

#include "DiscreteVerification/VerificationTypes/SMCVerification.hpp"

namespace VerifyTAPN::DiscreteVerification {

class SMCTracesGenerator : public SMCVerification {

    public:

        SMCTracesGenerator(
            TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
        );

        SMCTracesGenerator(
            TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options, unsigned int runs
        )
        : SMCVerification(tapn, initialMarking, query, options)
        { }

        bool handleSuccessor(RealMarking* marking) override;
        void handleRunResult(const bool res, int steps, double delay, unsigned int thread_id = 0) override;
        bool mustDoAnotherRun() override;

        void printResult() override;

        void printWatchStats();

        void handleTrace(const bool runRes, SMCRunGenerator* generator = nullptr) override;

    private:

        void saveWatchs(const unsigned int thread_id);
        void clearWatchs(const unsigned int thread_id);

        std::vector<std::vector<Watch>> savedWatchs;

};

}

#endif