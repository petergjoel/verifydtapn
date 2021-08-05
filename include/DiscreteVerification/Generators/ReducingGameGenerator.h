/* 
 * File:   ReducingGameGenerator.h
 * Author: Peter G. Jensen
 *
 * Created on 14 November 2020, 15.40
 */

#ifndef REDUCINGGAMEGENERATOR_H
#define REDUCINGGAMEGENERATOR_H

#include "Generator.h"
#include "GameStubbornSet.h"
#include "GameGenerator.h"

namespace VerifyTAPN {
    namespace DiscreteVerification {

        class ReducingGameGenerator : public GameGenerator {
        protected:
            GameStubbornSet _stubborn;
        public:

            ReducingGameGenerator(TAPN::TimedArcPetriNet &tapn, AST::Query *query)
            : GameGenerator(tapn, query), _stubborn(tapn, query) {
            };
            
            virtual void prepare(NonStrictMarkingBase *parent) override;
            virtual NonStrictMarkingBase* next(bool controllable) override;
            virtual void reset() override;
        };
    }
}
#endif /* REDUCINGGAMEGENERATOR_H */

