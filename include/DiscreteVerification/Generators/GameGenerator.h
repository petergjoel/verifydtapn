/* 
 * File:   GameGenerator.h
 * Author: Peter G. Jensen
 *
 * Created on 12 November 2020, 21.39
 */

#ifndef GAMEGENERATOR_H
#define GAMEGENERATOR_H
#include "DiscreteVerification/Generators/Generator.h"
#include "Generator.h"

namespace VerifyTAPN {
    namespace DiscreteVerification {

        class GameGenerator : protected Generator {
        protected:
            bool _had_urgent = false;
        public:
            GameGenerator(TAPN::TimedArcPetriNet &tapn, AST::Query *query)
            : Generator(tapn, query) {
            };
            virtual void prepare(NonStrictMarkingBase *parent);
            virtual NonStrictMarkingBase* next(bool controllable);
            virtual void reset();
            const TimedTransition* last_fired() const { return Generator::last_fired(); }
        };

    }
}

#endif /* GAMEGENERATOR_H */

