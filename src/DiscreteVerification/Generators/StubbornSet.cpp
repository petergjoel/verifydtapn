/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StubbornSet.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 13 November 2020, 12.20
 */

#include "DiscreteVerification/Generators/StubbornSet.h"
#include "DiscreteVerification/Generators/InterestingVisitor.h"
#include "DiscreteVerification/QueryVisitor.hpp"

#include <cassert>

namespace VerifyTAPN {
    namespace DiscreteVerification {

        StubbornSet::StubbornSet(const TimedArcPetriNet& tapn, AST::Query* query)
        : _tapn(tapn), _query(query), _gen_enabled(tapn), _interesting(tapn), 
          _enabled(tapn.getTransitions().size()), _stubborn(tapn.getTransitions().size()) {
            
        }

        const TimedTransition* StubbornSet::pop_next() {
            while(!_enabled_set.empty())
            {
                auto tid = _enabled_set.front();
                _enabled_set.pop_front();
                if(!_stubborn[tid]) continue;
                return _tapn.getTransitions()[tid];
            }
            return nullptr;
        }

        void StubbornSet::preset_of(size_t i) {
            auto place = _tapn.getPlaces()[i];
            for (auto* arc : place->getOutputArcs()) {
                auto t = arc->getInputTransition().getIndex();
                assert(t < _tapn.getTransitions().size());
                set_stubborn(t);
            }
            for (auto* arc : place->getProdTransportArcs()) {
                auto t = arc->getTransition().getIndex();
                assert(t < _tapn.getTransitions().size());
                set_stubborn(t);
            }
        }

        void StubbornSet::postset_of(size_t i, const TAPN::TimeInterval &interval) {
            auto place = _tapn.getPlaces()[i];
            if (!_added_zt) {
                auto &tl = _parent->getTokenList(place->getIndex());
                if (!tl.empty()) {
                    if (place->getInvariant().getBound() == tl.back().getAge())
                        _added_zt = true;
                }
            }
            for (auto* arc : place->getInputArcs()) {
                auto t = arc->getOutputTransition().getIndex();
                assert(t < _tapn.getTransitions().size());
                if (!interval.intersects(arc->getInterval())) continue;
                set_stubborn(t);
            }

            for (auto* arc : place->getTransportArcs()) {
                if (&arc->getSource() == &arc->getDestination()) continue;
                auto t = arc->getTransition().getIndex();
                assert(t < _tapn.getTransitions().size());
                if (!interval.intersects(arc->getInterval())) continue;
                set_stubborn(t);
            }
        }

        void StubbornSet::inhib_postset_of(size_t i) {
            auto place = _tapn.getPlaces()[i];
            for (auto* arc : place->getInhibitorArcs()) {
                auto t = arc->getOutputTransition().getIndex();
                assert(t < _tapn.getTransitions().size());
                set_stubborn(t);
            }
        }

        bool StubbornSet::urgent_priority(const TimedTransition* urg_trans, const TimedTransition* trans) const {
            return urg_trans == nullptr;
        }

        const TAPN::TimedTransition* StubbornSet::compute_enabled(std::function<void(const TimedTransition*)>&& monitor) {
            assert(_enabled_set.empty());
            const TAPN::TimedTransition *urg_trans = nullptr;
            _gen_enabled.prepare(_parent);
            std::fill(_enabled.begin(), _enabled.end(), false);

            do
            {
                auto [trans, consumes] = _gen_enabled.next_transition();
                if(trans == nullptr) break;
                _enabled[trans->getIndex()] = true;
                monitor(trans);
                _enabled_set.push_back(trans->getIndex());
                if (this->urgent_priority(urg_trans, trans) && trans->isUrgent()) {
                    urg_trans = trans;
                    _urgent_enabled = true;
                }

            } while(true);
            return urg_trans;
        }

        bool StubbornSet::is_singular() {
            if (_enabled_set.size() <= 1) {
                _can_reduce = false;
                _enabled_set.clear();
                return true;
            }
            return false;
        }

        void StubbornSet::reset(NonStrictMarkingBase* p) {
            _added_zt = false;
            _parent = p;
            _can_reduce = false;
            _urgent_enabled = false;
            _unprocessed.clear();
            _enabled_set.clear();
        }

        std::pair<const TimedPlace*,uint32_t> StubbornSet::invariant_place(const TimedTransition* urg_trans) {
            int32_t max_age = -1;
            _can_reduce = !_unprocessed.empty() || urg_trans != nullptr;
            if (!_can_reduce) {
                for (auto &place : _parent->getPlaceList()) {
                    int inv = place.place->getInvariant().getBound();
                    max_age = place.maxTokenAge();
                    if (max_age == inv) {
                        _can_reduce = true;
                        return {place.place, max_age};
                    }
                }
            }
            return {nullptr, max_age};
        }

        bool StubbornSet::stubborn_filter(size_t trans) const {
            return true;
        }

        void StubbornSet::set_stubborn(size_t t, bool add_to_working) {
            if (!_stubborn[t] && this->stubborn_filter(t)) {
                _stubborn[t] = true;
                //if(add_to_working)
                {
                    _unprocessed.push_back(t);
                    if (is_enabled(t) && _tapn.getTransitions()[t]->isUrgent() && this->ok_zt(_tapn.getTransitions()[t]))
                            _added_zt = true;
                }
            }
        }

        void StubbornSet::clear_stubborn() {
            std::fill(_stubborn.begin(), _stubborn.end(), false);
        }

        void StubbornSet::prepare(NonStrictMarkingBase *p) {
            return _prepare(p, [](auto a) {}, []{return true; });
        }

        void StubbornSet::_prepare(NonStrictMarkingBase *p, std::function<void(const TimedTransition*)>&& enabled_monitor, std::function<bool(void)>&& extra_conditions) {
            reset(p);
            auto* urg_trans = compute_enabled(std::move(enabled_monitor));

            if(is_singular())
                return;

            auto [inv_place, max_age] = invariant_place(urg_trans);

            if (!_can_reduce) return;
            clear_stubborn();

            if (!extra_conditions())
            {
                _can_reduce = false;
                return;
            }


            ample_set(inv_place, urg_trans);
            compute_closure();
            if (!_added_zt) {
                if(!zero_time_set(max_age, inv_place, urg_trans))
                {
                    _can_reduce = false;
                    _unprocessed.clear();
                }
                compute_closure();
            }
        }

        bool StubbornSet::zt_priority(const TimedTransition* trans, const TimedPlace* inv_place) const {
            return trans != nullptr;
        }
        
        bool StubbornSet::zero_time_set(int32_t max_age, const TAPN::TimedPlace *inv_place,
                const TAPN::TimedTransition *trans) {
            trans = nullptr;
            for(auto e : _enabled_set) {
                auto* tr = _tapn.getTransitions()[e];
                if(is_enabled(e) && tr->isUrgent() && this->ok_zt(tr))
                {
                    trans = tr;
                    break;
                }

            }
            if(trans == nullptr && inv_place == nullptr) return false;
            if (trans && this->zt_priority(trans, inv_place)) {
                // reason for urgency is an urgent edge
                set_stubborn(trans);
                for (auto* a : trans->getInhibitorArcs())
                    preset_of(a->getInputPlace().getIndex());
            } else {
                // reason for urgency is an invariant
                assert(inv_place);
                for (auto* a : inv_place->getInputArcs()) {
                    auto &interval = a->getInterval();
                    if (interval.contains(max_age)) {
                        set_stubborn(a->getOutputTransition());
                    }
                }
                for (auto* a : inv_place->getTransportArcs()) {
                    if (&a->getDestination() == &a->getSource()) continue;
                    auto &interval = a->getInterval();
                    if (interval.contains(max_age)) {
                        set_stubborn(a->getTransition());
                    }
                }
            }
            return true;
        }

        void StubbornSet::ample_set(const TAPN::TimedPlace *inv_place, const TAPN::TimedTransition *trans) {
            
            QueryVisitor<NonStrictMarkingBase> visitor(*_parent, _tapn);
            BoolResult context;
            _query->accept(visitor, context);
            _interesting.clear();
            _query->accept(_interesting, context);

            // compute the set of unprocessed
            for (size_t i = 0; i < _tapn.getPlaces().size(); ++i) {
                if (_interesting.increments(i))
                    preset_of(i);
                if (_interesting.decrements(i))
                    postset_of(i);
            }

            if (_interesting.deadlock() && !trans && !_added_zt) {
                // for now, just pick a single enabled,
                // verifypn has a good heuristic for this
                size_t min = 0;
                size_t max = _enabled.size();
                if (inv_place) {
                    for (auto* a : inv_place->getInputArcs()) {
                        if (this->is_enabled(a->getOutputTransition().getIndex())) {
                            min = a->getOutputTransition().getIndex();
                            max = min + 1;
                            break;
                        }
                    }
                    if (max != min + 1) {
                        for (auto* a : inv_place->getTransportArcs()) {
                            if (this->is_enabled(a->getTransition().getIndex())) {
                                min = a->getTransition().getIndex();
                                max = min + 1;
                                break;
                            }
                        }
                    }
                }
                for (size_t i = min; i < max; ++i) {
                    if (this->is_enabled(i)) {
                        auto trans = _tapn.getTransitions()[i];
                        for (auto* a : trans->getPreset())
                            postset_of(a->getInputPlace().getIndex());
                        for (auto* a : trans->getTransportArcs())
                            postset_of(a->getSource().getIndex());
                        for (auto* a : trans->getInhibitorArcs()) {
                            auto &place = a->getInputPlace();
                            for (auto* arc : place.getInhibitorArcs()) {
                                preset_of(arc->getInputPlace().getIndex());
                            }
                        }
                        break;
                    }
                }
            }
        }

        void StubbornSet::compute_closure() {
            // Closure computation time!
            while (!_unprocessed.empty()) {
                uint32_t tr = _unprocessed.front();
                _unprocessed.pop_front();
                auto trans = _tapn.getTransitions()[tr];
                assert(tr < _tapn.getTransitions().size());
                assert(trans);
                if (this->is_enabled(tr)) {
                    // add everything we might disable in future by fireing
                    for (auto* a : trans->getPreset())
                        postset_of(a->getInputPlace().getIndex(), a->getInterval());
                    for (auto* a : trans->getPostset())
                        inhib_postset_of(a->getOutputPlace().getIndex());
                    for (auto* a : trans->getTransportArcs()) {
                        postset_of(a->getSource().getIndex(), a->getInterval());
                        inhib_postset_of(a->getDestination().getIndex());
                    }
                } else {
                    // find reason for being disabled!
                    if (auto inhib = _gen_enabled.is_inhibited(trans)) {
                        auto &p = inhib->getInputPlace();
                        auto &tl = _parent->getTokenList(p.getIndex());
                        for (auto* arc : p.getInputArcs()) {
                            uint32_t trans = arc->getOutputTransition().getIndex();
                            auto cons = arc->getOutputTransition().getConsumed(&p);
                            if(cons < arc->getWeight()) continue; // produces more than it consumes
                            if (!_stubborn[trans]) {
                                for (const auto& t : tl)
                                {
                                    if (arc->getInterval().contains(t.getAge())) {
                                        set_stubborn(trans);
                                        break;
                                    }
                                }
                            }
                        }
                        for (auto* arc : p.getTransportArcs()) {
                            uint32_t trans = arc->getTransition().getIndex();
                            auto cons = arc->getTransition().getConsumed(&p);
                            if(cons < arc->getWeight()) continue; // produces more than it consumes
                            if (!_stubborn[trans]) {
                                for (const auto& t : tl)
                                {
                                    if (arc->getInterval().contains(t.getAge())) {
                                        set_stubborn(trans);
                                        break;
                                    }
                                }
                            }
                        }
                    } else {
                        const auto best = [this](std::pair<const TimedPlace*, TimeInterval> el, const TimedPlace* p2, TimeInterval interval) -> std::pair<const TimedPlace*, TimeInterval>
                        {
                            if(el.first == nullptr) return {p2, interval};
                            if(p2 == nullptr) return el;
                            auto c1 = el.first->getInputArcs().size() +
                                      el.first->getProdTransportArcs().size();
                            auto c2 = p2->getInputArcs().size() +
                                      p2->getProdTransportArcs().size();
                            if(el.second.contains(0) && !interval.contains(0))
                                return {p2, interval};
                            else if(!el.second.contains(0) && interval.contains(0))
                                return el;
                            if(c1 == c2)
                            {
                                if(el.first < p2)
                                    return el;
                                else
                                    return {p2, interval};
                            }
                            if(c1 < c2)
                                return el;
                            else
                                return {p2, interval};
                        };
                        // first we need to find the non-enabler
                        std::pair<const TimedPlace*, TimeInterval> cand{nullptr,TimeInterval(false,0,0,false)};
                        for (auto* a : trans->getPreset()) {
                            auto& tl = _parent->getTokenList(a->getInputPlace().getIndex());
                            size_t cnt = 0;
                            for(auto& token : tl)
                            {
                                if(a->getInterval().contains(token.getAge()))
                                    cnt += token.getCount();
                            }
                            if(cnt >= a->getWeight()) continue;
                            cand = best(cand, &a->getInputPlace(), a->getInterval());
                        }

                        for (auto* a : trans->getTransportArcs()) {
                            auto& tl = _parent->getTokenList(a->getSource().getIndex());
                            size_t cnt = 0;
                            for(auto& token : tl)
                            {
                                if(a->getInterval().contains(token.getAge()))
                                    cnt += token.getCount();
                            }
                            if(cnt >= a->getWeight()) continue;
                            cand = best(cand, &a->getSource(), a->getInterval());
                        }
                        assert(cand.first);
                        if(cand.second.contains(0))
                            preset_of(cand.first->getIndex());
                        else
                        {
                            for (auto* a : cand.first->getProdTransportArcs()) {
                                auto t = &a->getTransition();
                                if (a->getInterval().intersects(cand.second)) {
                                    set_stubborn(t);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}