/* 
 * File:   RandomRunGenerator.h
 * Author: Tanguy Dubois
 * 
 * Created on 11 April 2024, 10.13
 */

#ifndef SMCRUNGENERATOR_H
#define SMCRUNGENERATOR_H

#include "DiscreteVerification/Generators/Generator.h"
#include "DiscreteVerification/Util/IntervalOps.hpp"
#include "Core/Query/SMCQuery.hpp"
#include "DiscreteVerification/DataStructures/RealMarking.hpp"
#include "Core/TAPN/StochasticStructure.hpp"

namespace VerifyTAPN {
    namespace DiscreteVerification {

        class SMCRunGenerator {

        public:

            SMCRunGenerator(TAPN::TimedArcPetriNet &tapn, const unsigned int numericPrecision = 0)
            : _tapn(tapn)
            , _defaultTransitionIntervals(tapn.getTransitions().size()) 
            , _transitionsStatistics(tapn.getTransitions().size(), 0)
            , _numericPrecision(numericPrecision)
            {
                std::random_device rd;
                _rng = std::ranlux48(rd());
            };

            ~SMCRunGenerator() {
                delete _origin;
                if(_trace.size() > 0) {
                    for(RealMarking* marking : _trace) {
                        if(marking != nullptr) delete marking;
                    }
                }
                else if(_parent != nullptr) delete _parent;
            }

            virtual void prepare(RealMarking *parent);
            virtual RealMarking* next();
            virtual void reset();

            SMCRunGenerator copy() const;

            RealMarking* getMarking() { return _parent; }

            void refreshTransitionsIntervals();

            void disableTransitions(RealMarking* marking);

            std::vector<Util::interval<double>> transitionFiringDates(TimedTransition* transi);
            std::vector<Util::interval<double>> arcFiringDates(TimeInterval time_interval, uint32_t weight, RealTokenList& tokens);
            
            std::vector<RealToken> removeRandom(RealTokenList& tokenlist, const TimeInterval& interval, const int weight);
            std::vector<RealToken> removeYoungest(RealTokenList& tokenlist, const TimeInterval& interval, const int weight);
            std::vector<RealToken> removeOldest(RealTokenList& tokenlist, const TimeInterval& interval, const int weight);

            std::pair<TimedTransition*, double> getWinnerTransitionAndDelay();

            RealMarking* fire(TimedTransition* transi);

            bool reachedEnd() const;

            double getRunDelay() const;
            int getRunSteps() const;

            void printTransitionStatistics(std::ostream &out) const;

            std::stack<RealMarking*> getTrace() const;

            bool recordTrace = false;

            unsigned int _thread_id = 0;
            
        protected:
        
            TimedTransition* chooseWeightedWinner(const std::vector<size_t>& winner_indexs);
            
            bool _maximal = false;
            TimedArcPetriNet& _tapn;
            std::vector<std::vector<Util::interval<double>>> _defaultTransitionIntervals; // Type not pretty, but need disjoint intervals
            std::vector<std::vector<Util::interval<double>>> _transitionIntervals; // Type not pretty, but need disjoint intervals
            std::vector<double> _dates_sampled;
            std::vector<uint32_t> _transitionsStatistics;
            RealMarking* _origin;
            RealMarking* _parent;
            double _lastDelay = 0;
            double _totalTime = 0;
            int _totalSteps = 0;

            unsigned int _numericPrecision = 0;

            std::ranlux48 _rng;

            std::vector<RealMarking*> _trace;
            
        };

    }
}

#endif /* SMCRUNGENERATOR_H */

