#ifndef PROBABILITYESTIMATION_HPP
#define PROBABILITYESTIMATION_HPP

#include "DiscreteVerification/VerificationTypes/SMCVerification.hpp"
#include "Core/TAPN/WatchExpression.hpp"

namespace VerifyTAPN::DiscreteVerification {

class ProbabilityEstimation : public SMCVerification {

    public:

        ProbabilityEstimation(
            TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
        );

        ProbabilityEstimation(
            TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options, unsigned int runs
        )
        : SMCVerification(tapn, initialMarking, query, options), validRuns(0), runsNeeded(runs)
        { }

        bool handleSuccessor(RealMarking* marking) override;
        void handleRunResult(const bool res, int steps, double delay, unsigned int thread_id = 0) override;
        bool mustDoAnotherRun() override;

        void prepare() override;

        float getEstimation();

        void computeChernoffHoeffdingBound(const float intervalWidth, const float confidence);

        void printStats() override;

        void printValidRunsStats();
        void printViolatingRunsStats();
        void printGlobalRunsStats();

        static void printRunsStats(const std::string category, unsigned long n, unsigned long totalSteps, double totalDelay, std::vector<int> perStep, std::vector<float> perDelay);

        void printCumulativeStats();

        void printWatchStats();

        void printResult() override;

    protected:

        unsigned int runsNeeded;
        unsigned int validRuns;
        double validRunsTime = 0;
        unsigned long validRunsSteps = 0;
        double violatingRunTime = 0;
        unsigned long violatingRunSteps = 0;

        std::vector<int> validPerStep;
        std::vector<float> validPerDelay;
        std::vector<int> violatingPerStep;
        std::vector<float> violatingPerDelay;
        float maxValidDuration = 0.0f;

};

}

#endif /*PROBABILITYESTIMATION_HPP*/