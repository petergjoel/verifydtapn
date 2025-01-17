#ifndef PROBABILITYFLOATCOMPARISON_HPP
#define PROBABILITYFLOATCOMPARISON_HPP

#include "DiscreteVerification/VerificationTypes/SMCVerification.hpp"

namespace VerifyTAPN::DiscreteVerification {

class ProbabilityFloatComparison : public SMCVerification {

    public:

        ProbabilityFloatComparison(
            TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
        );

        bool handleSuccessor(RealMarking* marking) override;
        void handleRunResult(const bool res, int steps, double delay, unsigned int thread_id = 0) override;
        bool mustDoAnotherRun() override;

        bool getResult();

        void computeAcceptingBounds(const float alpha, const float beta);
        void computeIndifferenceRegion(const float p, const float sigma0, const float sigma1);

        void printStats() override;

        void printResult() override;

    protected:

        float ratio;
        float p0;
        float p1;
        float boundH0;
        float boundH1;
        bool result;
        unsigned int validRuns;

};

}

#endif /*PROBABILITYFLOATCOMPARISON_HPP*/