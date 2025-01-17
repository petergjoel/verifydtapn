#ifndef SMCVERIFICATION_HPP
#define SMCVERIFICATION_HPP

#include "DiscreteVerification/VerificationTypes/Verification.hpp"
#include "DiscreteVerification/DataStructures/NonStrictMarking.hpp"
#include "DiscreteVerification/Generators/SMCRunGenerator.h"
#include "Core/Query/SMCQuery.hpp"
#include "Core/TAPN/WatchExpression.hpp"

#include <mutex>

namespace VerifyTAPN::DiscreteVerification {

class SMCVerification : public Verification<RealMarking> {

    public:

        SMCVerification(TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query,
                        VerificationOptions options) 
            : Verification(tapn, initialMarking, query, options)
            , runGenerator(tapn, options.getSMCNumericPrecision())
            , numberOfRuns(0), maxTokensSeen(0), smcSettings(query->getSmcSettings())
            { }

        virtual bool run() override;
        virtual bool parallel_run();

        virtual void prepare() { }

        virtual bool executeRun(SMCRunGenerator* generator = nullptr);

        virtual void printStats() override;
        void printTransitionStatistics() const override;

        unsigned int maxUsedTokens() override;
        void setMaxTokensIfGreater(unsigned int i);

        virtual bool reachedRunBound(SMCRunGenerator* generator = nullptr);
        
        virtual void handleRunResult(const bool res, int steps, double delay, unsigned int thread_id = 0) = 0;
        virtual bool mustDoAnotherRun() = 0;

        virtual void printResult() = 0;

        inline bool mustSaveTrace() const { return traces.size() < options.getSmcTraces(); }
        virtual void handleTrace(const bool runRes, SMCRunGenerator* generator = nullptr);
        void saveTrace(SMCRunGenerator* generator = nullptr);

        virtual void initWatchs(unsigned int n_threads = 1);

        SMCQuery* getSmcQuery() { return (SMCQuery*) query; }

        void getTrace() override;

        void printHumanTrace(std::stack<RealMarking *> &stack, const std::string& name);

        void printXMLTrace(std::stack<RealMarking *> &stack, const std::string& name, rapidxml::xml_document<> &doc, rapidxml::xml_node<>* list_node);

        rapidxml::xml_node<> *createTransitionNode(RealMarking *old, RealMarking *current, rapidxml::xml_document<> &doc);

        void createTransitionSubNodes(RealMarking *old, RealMarking *current, rapidxml::xml_document<> &doc,
                                      rapidxml::xml_node<> *transitionNode, const TAPN::TimedPlace &place,
                                      const TAPN::TimeInterval &interval, int weight);

        rapidxml::xml_node<> *
        createTokenNode(rapidxml::xml_document<> &doc, const TAPN::TimedPlace &place, const RealToken &token);

    protected:

        SMCRunGenerator runGenerator;
        SMCSettings smcSettings;
        size_t numberOfRuns;
        unsigned int maxTokensSeen;
        double totalTime = 0;
        unsigned long totalSteps = 0;
        int64_t durationNs = 0;

        std::mutex run_res_mutex;

        std::vector<std::stack<RealMarking*>> traces;

        std::vector<std::vector<Watch>> watchs;
        std::vector<WatchAggregator> watch_aggrs;

};

}

#endif  /* SMC_VERIFICATION_HPP */