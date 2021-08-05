/*
 * DiscreteVerification.cpp
 *
 *  Created on: 23 Feb 2012
 *      Author: jakob
 */

#include "DiscreteVerification/DiscreteVerification.hpp"
#include "DiscreteVerification/DeadlockVisitor.hpp"
#include "DiscreteVerification/VerificationTypes/SafetySynthesis.h"
#include "DiscreteVerification/Generators/ReducingGenerator.hpp"

#include <fstream>

namespace VerifyTAPN { namespace DiscreteVerification {

    template<typename T>
    void VerifyAndPrint(TAPN::TimedArcPetriNet &tapn, Verification<T> &verifier, VerificationOptions &options,
                        AST::Query *query);

    DiscreteVerification::DiscreteVerification() {
        // TODO Auto-generated constructor stub

    }

    DiscreteVerification::~DiscreteVerification() {
        // TODO Auto-generated destructor stub
    }

    int
    DiscreteVerification::run(TAPN::TimedArcPetriNet &tapn, std::vector<int> initialPlacement, AST::Query *query,
                              VerificationOptions &options) {
        if (!tapn.isNonStrict()) {
            std::cout << "The supplied net contains strict intervals." << std::endl;
            return -1;
        }

        NonStrictMarking *initialMarking = new NonStrictMarking(tapn, initialPlacement);

        std::cout << "MC: " << tapn.getMaxConstant() << std::endl;
#if DEBUG
        std::cout << "Places: " << std::endl;
        for (TAPN::TimedPlace::Vector::const_iterator iter = tapn.getPlaces().begin(); iter != tapn.getPlaces().end(); iter++) {
            std::cout << "Place " << (*iter)->getIndex() << " has category " << (*iter)->getType() << std::endl;
        }
#endif

        if (initialMarking->size() > options.getKBound()) {
            std::cout << "The specified k-bound is less than the number of tokens in the initial markings.";
            return 1;
        }

        std::cout << options;

        // Select verification method
        if (options.getWorkflowMode() != VerificationOptions::NOT_WORKFLOW) {
            if (options.getVerificationType() == VerificationOptions::TIMEDART) {
                std::cout << "Workflow analysis currently only supports discrete exploration (i.e. not TimeDarts)."
                     << std::endl;
               std::exit(1);
            }

            if (options.getWorkflowMode() == VerificationOptions::WORKFLOW_SOUNDNESS) {
                WorkflowSoundness *verifier;
                if (options.getMemoryOptimization() == VerificationOptions::NO_MEMORY_OPTIMIZATION) {
                    verifier = new WorkflowSoundness(tapn, *initialMarking, query, options,
                                                     getWaitingList<NonStrictMarking *>(query, options));
                } else {
                    verifier = new WorkflowSoundnessPTrie(tapn, *initialMarking, query, options,
                                                          getWaitingList<ptriepointer_t<MetaData *> >(query,
                                                                                                      options));
                }

                if (verifier->getModelType() == verifier->NOTTAWFN) {
                    std::cerr << "Model is not a TAWFN!" << std::endl;
                    return -1;
                } else if (verifier->getModelType() == verifier->ETAWFN) {
                    std::cout << "Model is a ETAWFN" << std::endl << std::endl;
                } else if (verifier->getModelType() == verifier->MTAWFN) {
                    std::cout << "Model is a MTAWFN" << std::endl << std::endl;
                }
                VerifyAndPrint(
                        tapn,
                        *verifier,
                        options,
                        query);
                verifier->printExecutionTime(std::cout);
                verifier->printMessages(std::cout);
#ifdef CLEANUP
                delete verifier;
#endif
            } else {
                // Assumes correct structure of net!
                WorkflowStrongSoundnessReachability *verifier;
                if (options.getMemoryOptimization() == VerificationOptions::NO_MEMORY_OPTIMIZATION) {
                    verifier = new WorkflowStrongSoundnessReachability(tapn, *initialMarking, query, options,
                                                                       getWaitingList<NonStrictMarking *>(query,
                                                                                                          options));
                } else {
                    verifier = new WorkflowStrongSoundnessPTrie(tapn, *initialMarking, query, options,
                                                                getWaitingList<ptriepointer_t<MetaData *> >(query,
                                                                                                            options));
                }
                VerifyAndPrint(
                        tapn,
                        *verifier,
                        options,
                        query);
                verifier->printExecutionTime(std::cout);
#ifdef CLEANUP
                delete verifier;
#endif
            }


            return 0;
        }


        // asume that we dont reach here except if we are doing normal verification
        AST::BoolResult containsDeadlock;
        DeadlockVisitor deadlockVisitor = DeadlockVisitor();
        deadlockVisitor.visit(*query, containsDeadlock);
        if (containsDeadlock.value && options.getGCDLowerGuardsEnabled()) {
            std::cout
                    << "Lowering constants by greatest common divisor is unsound for queries containing the deadlock proposition"
                    << std::endl;
           std::exit(1);
        } else if (containsDeadlock.value && options.getTrace() == VerificationOptions::FASTEST_TRACE) {
            std::cout << "Fastest trace is not supported for queries containing the deadlock proposition." << std::endl;
           std::exit(1);
        }

        if ((query->getQuantifier() == EG || query->getQuantifier() == AF) && options.getGCDLowerGuardsEnabled()) {
            std::cout << "Lowering constants by greatest common divisor is unsound for EG and AF queries" << std::endl;
           std::exit(1);
        }

        if (query->getQuantifier() == CG || query->getQuantifier() == CF) {
            if (options.getTrace() != VerificationOptions::NO_TRACE) {
                std::cout << "Traces are not supported for game synthesis" << std::endl;
                std::exit(1);
            }
            if (options.getVerificationType() != VerificationOptions::DISCRETE) {
                std::cout << "TimeDarts are not supported for game synthesis" << std::endl;
               std::exit(1);
            }
            if (options.getGCDLowerGuardsEnabled()) {
                std::cout << "Lowering by GCD is not supported for game synthesis" << std::endl;
                std::exit(1);
            }
            if (options.getSearchType() == VerificationOptions::MINDELAYFIRST) {
                std::cout << "Minimal delay search strategy is not supported for game synthesis" << std::endl;
                std::exit(1);
            }
            
            // Only needed if verifying normal CTL/LTL with game-algorithm.
            // Notice that violating k-bound produces different results than in normal ctl semantics
            if (query->getQuantifier() == AST::EG || query->getQuantifier() == AST::AF) {
                tapn.setAllControllable(true);
            } else if (query->getQuantifier() == AST::EF || query->getQuantifier() == AST::AG) {
                tapn.setAllControllable(false);
                query->setChild(new NotExpression(query->getChild()));
            }

            SafetySynthesis synthesis = SafetySynthesis(
                    tapn, *initialMarking, query, options
            );
            bool result = synthesis.run();
            synthesis.print_stats();
            if(!options.getStrategyFile().empty()) {
                if(options.getStrategyFile() == "_")
                    synthesis.write_strategy(std::cout);
                else
                {
                    std::ofstream of(options.getStrategyFile());
                    synthesis.write_strategy(of);
                }
            }

            if (query->getQuantifier() == AST::EF || query->getQuantifier() == AST::AF) result = !result;

            std::cout << "Query is " << (result ? "satisfied" : "NOT satisfied") << "." << std::endl;
            std::cout << "Max number of tokens found in any reachable marking: ";
            std::cout << synthesis.max_tokens() << std::endl;

        } else if (options.getVerificationType() == VerificationOptions::DISCRETE) {

            if (options.getMemoryOptimization() == VerificationOptions::PTRIE) {
                //TODO fix initialization
                WaitingList<ptriepointer_t<MetaData *> > *strategy = getWaitingList<ptriepointer_t<MetaData *> >(
                        query, options);
                if (query->getQuantifier() == EG || query->getQuantifier() == AF) {
                    LivenessSearchPTrie verifier = LivenessSearchPTrie(tapn, *initialMarking, query, options,
                                                                       strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                } else if (query->getQuantifier() == EF || query->getQuantifier() == AG) {
                    if (options.getPartialOrderReduction()) {
                        auto verifier = ReachabilitySearchPTrie<ReducingGenerator>(tapn, *initialMarking, query,
                                                                                   options, strategy);
                        VerifyAndPrint(
                                tapn,
                                verifier,
                                options,
                                query);
                    } else {
                        auto verifier = ReachabilitySearchPTrie<Generator>(tapn, *initialMarking, query, options,
                                                                           strategy);
                        VerifyAndPrint(
                                tapn,
                                verifier,
                                options,
                                query);
                    }

                }
                delete strategy;
            } else {
                WaitingList<NonStrictMarking *> *strategy = getWaitingList<NonStrictMarking *>(query, options);
                if (query->getQuantifier() == EG || query->getQuantifier() == AF) {
                    LivenessSearch verifier = LivenessSearch(tapn, *initialMarking, query, options, strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                } else if (query->getQuantifier() == EF || query->getQuantifier() == AG) {
                    if (options.getPartialOrderReduction()) {
                        auto verifier = ReachabilitySearch<ReducingGenerator>(tapn, *initialMarking, query, options,
                                                                              strategy);
                        VerifyAndPrint(
                                tapn,
                                verifier,
                                options,
                                query);
                    } else {
                        auto verifier = ReachabilitySearch<Generator>(tapn, *initialMarking, query, options,
                                                                      strategy);
                        VerifyAndPrint(
                                tapn,
                                verifier,
                                options,
                                query);
                    }
                }
                delete strategy;
            }
        } else if (options.getVerificationType() == VerificationOptions::TIMEDART) {
            if (query->getQuantifier() == EG || query->getQuantifier() == AF) {
                if (containsDeadlock.value) {
                    std::cout
                            << "The combination of TimeDarts, Deadlock proposition and EG or AF queries is currently not supported"
                            << std::endl;
                   std::exit(1);
                }
                if (options.getMemoryOptimization() == VerificationOptions::PTRIE) {
                    WaitingList<std::pair<WaitingDart *, ptriepointer_t<LivenessDart *> > > *strategy = getWaitingList<std::pair<WaitingDart *, ptriepointer_t<LivenessDart *> > >(
                            query, options);
                    TimeDartLivenessPData verifier = TimeDartLivenessPData(tapn, *initialMarking, query, options,
                                                                           strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                    delete strategy;
                } else {
                    WaitingList<WaitingDart *> *strategy = getWaitingList<WaitingDart *>(query, options);
                    TimeDartLiveness verifier = TimeDartLiveness(tapn, *initialMarking, query, options, strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                    delete strategy;
                }
            } else if (query->getQuantifier() == EF || query->getQuantifier() == AG) {

                if (options.getMemoryOptimization() == VerificationOptions::PTRIE) {
                    WaitingList<TimeDartEncodingPointer> *strategy = getWaitingList<TimeDartEncodingPointer>(query,
                                                                                                             options);
                    TimeDartReachabilitySearchPData verifier = TimeDartReachabilitySearchPData(tapn,
                                                                                               *initialMarking,
                                                                                               query, options,
                                                                                               strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                    delete strategy;
                } else {
                    WaitingList<TimeDartBase *> *strategy = getWaitingList<TimeDartBase *>(query, options);
                    TimeDartReachabilitySearch verifier = TimeDartReachabilitySearch(tapn, *initialMarking, query,
                                                                                     options, strategy);
                    VerifyAndPrint(
                            tapn,
                            verifier,
                            options,
                            query);
                    delete strategy;
                }
            }
        }

        return 0;
    }

    template<typename T>
    void VerifyAndPrint(TAPN::TimedArcPetriNet &tapn, Verification<T> &verifier, VerificationOptions &options,
                        AST::Query *query) {
        bool result = (!options.isWorkflow() && (query->getQuantifier() == AG || query->getQuantifier() == AF))
                      ? !verifier.run() : verifier.run();

        if (options.getGCDLowerGuardsEnabled()) {
            std::cout << "Lowering all guards by greatest common divisor: " << tapn.getGCD() << std::endl;
        }
        std::cout << std::endl;

        verifier.printStats();
        verifier.printTransitionStatistics();
        verifier.printPlaceStatistics();

        std::cout << "Query is " << (result ? "satisfied" : "NOT satisfied") << "." << std::endl;
        std::cout << "Max number of tokens found in any reachable marking: ";
        if (verifier.maxUsedTokens() > options.getKBound())
            std::cout << ">" << options.getKBound() << std::endl;
        else
            std::cout << verifier.maxUsedTokens() << std::endl;

        if (options.getTrace() != VerificationOptions::NO_TRACE) {
            if ((query->getQuantifier() == EF && result) || (query->getQuantifier() == AG && !result) ||
                (query->getQuantifier() == EG && result) || (query->getQuantifier() == AF && !result) ||
                (options.isWorkflow())) {
                verifier.getTrace();
            } else {
                std::cout << "A trace could not be generated due to the query result" << std::endl;
            }
        }
    }
} }
