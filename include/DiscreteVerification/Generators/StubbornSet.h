/* 
 * File:   StubbornSet.h
 * Author: Peter G. Jensen
 *
 * Created on 13 November 2020, 12.20
 */

#ifndef STUBBORNSET_H
#define STUBBORNSET_H

#include "DiscreteVerification/DataStructures/light_deque.h"

#include "InterestingVisitor.h"
#include "NextEnabledGenerator.h"

namespace VerifyTAPN {
    namespace DiscreteVerification {
        using namespace TAPN;
        class StubbornSet {
        public:
            StubbornSet(const TimedArcPetriNet& tapn, AST::Query* query);
            void prepare(NonStrictMarkingBase *parent);
            const TimedTransition* pop_next();
            bool urgent() const { return _urgent_enabled; }
            bool irreducable() const { return !_can_reduce; }
        protected:
            const TimedArcPetriNet& _tapn;
            AST::Query* _query;
            light_deque<uint32_t> _enabled_set;
            NonStrictMarkingBase* _parent;
            bool _can_reduce = false;
        private:
            NextEnabledGenerator _gen_enabled;
            InterestingVisitor _interesting;
            std::vector<bool> _enabled, _stubborn;
            light_deque<uint32_t> _unprocessed;
            bool _urgent_enabled = false;
            bool _added_zt;
            
        protected:
            virtual bool urgent_priority(const TimedTransition* urg_trans, const TimedTransition* trans) const;
            virtual bool zt_priority(const TimedTransition* trans, const TimedPlace* inv_place) const;
            virtual bool stubborn_filter(size_t) const;
            virtual bool ok_zt(const TimedTransition* trans) { return true; }
            void _prepare(NonStrictMarkingBase *parent, std::function<void(const TimedTransition*)>&& enabled_monitor, std::function<bool(void)>&& extra_conditions);
            bool is_singular();
            void reset(NonStrictMarkingBase*);
            void clear_stubborn();
            const TimedTransition* compute_enabled(std::function<void(const TimedTransition*)>&& monitor = [](auto){});
            std::pair<const TimedPlace*,uint32_t> invariant_place(const TimedTransition* t = nullptr);
            
            void preset_of(size_t i);

            void postset_of(size_t i, const TAPN::TimeInterval &interval = TAPN::TimeInterval());

            void inhib_postset_of(size_t i);

            bool zero_time_set(int32_t max_age, const TAPN::TimedPlace *, const TAPN::TimedTransition *);

            void ample_set(const TAPN::TimedPlace *inv_place, const TAPN::TimedTransition *trans);

            void compute_closure();
            
            void set_stubborn(const TAPN::TimedTransition& trans) { set_stubborn(trans.getIndex()); }
            void set_stubborn(const TAPN::TimedTransition *trans) { set_stubborn(trans->getIndex()); }
            void set_stubborn(size_t t, bool add_to_working = true);            
            bool is_stubborn(size_t t) const { return _stubborn[t]; }
            virtual bool is_enabled(size_t t) const { return _enabled[t]; }
        };
    }
}
#endif /* STUBBORNSET_H */

