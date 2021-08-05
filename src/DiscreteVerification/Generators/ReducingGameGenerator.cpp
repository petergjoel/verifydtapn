/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ReducingGameGenerator.cpp
 * Author: pgj
 * 
 * Created on 14 November 2020, 15.40
 */


#include "DiscreteVerification/Generators/ReducingGameGenerator.h"
#include "DiscreteVerification/Generators/GameStubbornSet.h"
#include "DiscreteVerification/QueryVisitor.hpp"


namespace VerifyTAPN {
    namespace DiscreteVerification {

        void ReducingGameGenerator::prepare(NonStrictMarkingBase *parent) {
            GameGenerator::prepare(parent);
            _stubborn.prepare(parent);
        }

        void ReducingGameGenerator::reset() {
            GameGenerator::reset();
        }

        NonStrictMarkingBase *ReducingGameGenerator::next(bool controllable) {
            if(_stubborn.irreducable())
                return GameGenerator::next(controllable);

            if (_done) return nullptr;
            // we need more permutations
            if (_current)
                return fire(_current);

            // move to next transition
            auto transition = _stubborn.pop_next(controllable);
            if(transition != nullptr)
            {
#ifndef NDEBUG
                bool en =
#endif
                Generator::only_transition(transition);
#ifndef NDEBUG
                assert(en);
#endif
                return fire(transition);
            }
            // done, only delay waiting
            _done = true;
            if (controllable && !_seen_urgent)
                return from_delay();
            return nullptr;
        }
    }
}
