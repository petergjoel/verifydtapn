/* 
 * File:   GameStubbornSet.h
 * Author: Peter G. Jensen
 *
 * Created on 14 November 2020, 15.36
 */

#ifndef GAMESTUBBORNSET_H
#define GAMESTUBBORNSET_H

#include "StubbornSet.h"


namespace VerifyTAPN {
    namespace DiscreteVerification {

        class GameStubbornSet : private StubbornSet {
        public:
            bool urgent() const { return StubbornSet::urgent(); }
            bool irreducable() const { return StubbornSet::irreducable(); }

            GameStubbornSet(const TimedArcPetriNet& tapn, AST::Query* query);
            void prepare(NonStrictMarkingBase *parent);
            const TimedTransition* pop_next(bool controllable);
        protected:
            bool is_safe();
            void compute_bounds();
            bool reach();
            virtual bool urgent_priority(const TimedTransition* urg_trans, const TimedTransition* trans) const;
            virtual bool zt_priority(const TimedTransition*, const TimedPlace* place) const;
            virtual bool stubborn_filter(size_t) const;
            void compute_future_enabled();
            virtual bool ok_zt(const TimedTransition* trans) override {
                return true;
                /*if(trans->isEnvironment()) return true;
                else if(trans->isControllable() && !_has_env) return true;
                return false;*/
            }
            virtual bool is_enabled(size_t t) const override { 
                if (!StubbornSet::is_enabled(t)) return false; 
                return true;
            }
            
        private:
            light_deque<uint32_t> _ctrl_trans;
            light_deque<uint32_t> _env_trans;
            std::unique_ptr<uint32_t[]> _fireing_bounds;
            std::unique_ptr<std::pair<uint32_t,uint32_t>[]> _place_bounds;
            std::unique_ptr<uint8_t[]> _places_seen;
            std::unique_ptr<bool[]> _future_enabled;
            bool _has_env, _has_ctrl;

            // for places seen
            static constexpr uint8_t INCR = 16;
            static constexpr uint8_t DECR = 32;
            static constexpr uint8_t WAITING = 64;

        };
    }
}

#endif /* GAMESTUBBORNSET_H */

