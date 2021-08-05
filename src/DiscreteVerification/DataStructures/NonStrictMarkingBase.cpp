/*
 * NonStrictMarkingBase.cpp
 *
 *  Created on: 29/02/2012
 *      Author: MathiasGS
 */

#include "DiscreteVerification/DataStructures/NonStrictMarkingBase.hpp"
#include <limits>

namespace VerifyTAPN { namespace DiscreteVerification {

    TokenList NonStrictMarkingBase::emptyTokenList = TokenList();

    NonStrictMarkingBase::NonStrictMarkingBase() : children(0), parent(nullptr), generatedBy(nullptr) {
        // empty constructor
    }

    NonStrictMarkingBase::NonStrictMarkingBase(const TAPN::TimedArcPetriNet &tapn, const std::vector<int> &v)
            : children(0), parent(nullptr), generatedBy(nullptr) {
        int prevPlaceId = -1;
        for (int iter : v) {
            if (iter == prevPlaceId) {
                Place &p = places.back();
                if (p.tokens.empty()) {
                    Token t(0, 1);
                    p.tokens.push_back(t);
                } else {
                    p.tokens.begin()->add(1);
                }
            } else {

                Place p(&tapn.getPlace(iter));
                Token t(0, 1);
                p.tokens.push_back(t);
                places.push_back(p);
            }
            prevPlaceId = iter;
        }
    }

    NonStrictMarkingBase::NonStrictMarkingBase(const NonStrictMarkingBase &nsm) : children(0) {

        places = nsm.places;
        parent = nsm.parent;
        generatedBy = nsm.generatedBy;
    }

    unsigned int NonStrictMarkingBase::size() {
        int count = 0;
        for (const auto &place : places) {
            for (auto& t : place.tokens) {
                count += t.getCount();
            }
        }
        return count;
    }

    uint32_t NonStrictMarkingBase::numberOfTokensInPlace(int placeId) const {
        uint32_t count = 0;
        for (const auto &place : places) {
            if (place.place->getIndex() == placeId) {
                for (auto& token : place.tokens) {
                    count = count + token.getCount();
                }
            }
        }
        return count;
    }

    const TokenList &NonStrictMarkingBase::getTokenList(int placeId) const {
        for (const auto &place : places) {
            if (place.place->getIndex() == placeId) return place.tokens;
        }
        return emptyTokenList;
    }

    bool NonStrictMarkingBase::removeToken(int placeId, int age) {
        for (auto &place : places) {
            if (place.place->getIndex() == placeId) {
                for (auto tit = place.tokens.begin(); tit != place.tokens.end(); tit++) {
                    if (tit->getAge() == age) {
                        return removeToken(place, *tit);
                    }
                }
            }
        }
        return false;
    }

    void
    NonStrictMarkingBase::removeRangeOfTokens(Place &place, TokenList::iterator begin, TokenList::iterator end) {
        place.tokens.erase(begin, end);
    }

    bool NonStrictMarkingBase::removeToken(Place &place, Token &token) {
        if (token.getCount() > 1) {
            token.remove(1);
            return true;
        } else {
            for (auto iter = place.tokens.begin(); iter != place.tokens.end(); iter++) {
                if (iter->getAge() == token.getAge()) {
                    place.tokens.erase(iter);
                    if (place.tokens.empty()) {
                        for (auto it = places.begin(); it != places.end(); it++) {
                            if (it->place->getIndex() == place.place->getIndex()) {
                                places.erase(it);
                                return true;
                            }
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void NonStrictMarkingBase::addTokenInPlace(TAPN::TimedPlace &place, int age) {
        for (auto &pit : places) {
            if (pit.place->getIndex() == place.getIndex()) {
                for (auto tit = pit.tokens.begin(); tit != pit.tokens.end(); tit++) {
                    if (tit->getAge() == age) {
                        Token t(age, 1);
                        addTokenInPlace(pit, t);
                        return;
                    }
                }
                Token t(age, 1);
                addTokenInPlace(pit, t);
                return;
            }
        }
        Token t(age, 1);
        Place p(&place);
        addTokenInPlace(p, t);

        // Insert place
        bool inserted = false;
        for (auto it = places.begin(); it != places.end(); it++) {
            if (it->place->getIndex() > place.getIndex()) {
                places.insert(it, p);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            places.push_back(p);
        }
    }

    void NonStrictMarkingBase::addTokenInPlace(const TAPN::TimedPlace &place, Token &token) {
        for (auto &pit : places) {
            if (pit.place->getIndex() == place.getIndex()) {
                addTokenInPlace(pit, token);
                return;
            }
        }

        Place p(&place);
        addTokenInPlace(p, token);

        // Insert place
        bool inserted = false;
        for (auto it = places.begin(); it != places.end(); it++) {
            if (it->place->getIndex() > place.getIndex()) {
                places.insert(it, p);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            places.push_back(p);
        }
    }

    void NonStrictMarkingBase::addTokenInPlace(Place &place, Token &token) {
        if (token.getCount() == 0) return;
        for (auto &iter : place.tokens) {
            if (iter.getAge() == token.getAge()) {
                iter.add(token.getCount());
                return;
            }
        }
        // Insert token
        bool inserted = false;
        for (auto it = place.tokens.begin(); it != place.tokens.end(); it++) {
            if (it->getAge() > token.getAge()) {
                place.tokens.insert(it, token);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            place.tokens.push_back(token);
        }
    }

    NonStrictMarkingBase::~NonStrictMarkingBase() = default;

    int NonStrictMarkingBase::cmp(const NonStrictMarkingBase &m1) const {
        if (m1.places.size() != places.size())
            return (int) places.size() - (int) m1.places.size();

        auto p_iter = m1.places.begin();
        auto iter = places.begin();

        for (; iter != places.end(); iter++, p_iter++) {

            if (iter->place->getIndex() != p_iter->place->getIndex())
                return (int) iter->place->getIndex() - (int) p_iter->place->getIndex();

            if (iter->tokens.size() != p_iter->tokens.size())
                return (int) iter->tokens.size() - (int) p_iter->tokens.size();

            auto pt_iter = p_iter->tokens.begin();
            auto t_iter = iter->tokens.begin();
            for (; t_iter != iter->tokens.end(); t_iter++, pt_iter++) {
                int res = t_iter->cmp(*pt_iter);
                if (res != 0) return res;
            }
        }
        return 0;
    }

    bool NonStrictMarkingBase::equals(const NonStrictMarkingBase &m1) const {
        return cmp(m1) == 0;
    }

    std::ostream &operator<<(std::ostream &out, NonStrictMarkingBase &x) {
        for (auto iter = x.places.begin(); iter != x.places.end(); iter++) {
            for (auto it = iter->tokens.begin(); it != iter->tokens.end(); it++) {
                out << "(" << iter->place->getName() << ", " << it->getAge() << ", " << it->getCount() << ")";
            }
            if (iter != x.places.end() - 1) {
                out << ", ";
            }
        }

        return out;
    }

    bool NonStrictMarkingBase::canDeadlock(const TAPN::TimedArcPetriNet &tapn, const int maxDelay,
                                           bool ignoreCanDelay) const {
        bool canDelay = true;
        bool allMC = true;
        bool hasOutArc = false;

        // this should be allocated only once!
        int count = tapn.getTransitions().size();
        int *status = new int[count];

        for (auto& trans : tapn.getTransitions()) {
            int presetSize = trans->getPresetSize();
            int index = trans->getIndex();
            if (presetSize == 0 && trans->getInhibitorArcs().empty()) {
                delete[] status;
                return false; // if empty preset, we can always fire a transition
            }
            // set to preset-size so we know when all arcs have been enabled
            // we decrement it in the next three loops
            status[index] = presetSize;
        }

        for (const auto &place : this->places) {
            int numtokens = place.numberOfTokens();
            // for regular input arcs
            for (auto* arc : place.place->getInputArcs()) {
                hasOutArc = true;
                auto id = arc->getOutputTransition().getIndex();
                int weight = arc->getWeight();
                if (numtokens < weight) {
                    status[id] = -1;        // -1 for impossible to enable
                } else if (status[id] != -1) {
                    auto lb = arc->getInterval().getLowerBound();
                    auto ub = arc->getInterval().getUpperBound();
                    // decrement if token can satisfy the bounds
                    for (auto& token : place.tokens) {
                        int age = token.getAge() + maxDelay;
                        if (age >= lb) {
                            if (age <= ub) {
                                weight -= token.getCount(); // decrement weight
                            } else {
                                break; // sorted array, if > ub then no following will satisfy it
                            }
                        }
                    }
                    if (weight <= 0) {
                        --status[id];    // decrement counter, arc satisfied
                    } else {
                        status[id] = -1; // unless we can satisfy the weight, transition not enabled
                    }
                }
            }

            // for transport arcs
            for (auto* arc : place.place->getTransportArcs()) {
                hasOutArc = true;
                auto id = arc->getTransition().getIndex();
                int weight = arc->getWeight();
                if (numtokens < (int)arc->getWeight())
                    status[id] = -1; // impossible to enable
                else if (status[id] != -1) { // if enable able so far
                    auto lb = arc->getInterval().getLowerBound();
                    auto destination_invariant = arc->getDestination().getInvariant().getBound();
                    auto ub = std::min(arc->getInterval().getUpperBound(), destination_invariant);
                    // decrement if token can satisfy the bounds
                    for (auto& token : place.tokens) {
                        auto age = token.getAge() + maxDelay;
                        if (age >= lb) {
                            if (age <= ub) {
                                weight -= token.getCount();
                            } else {
                                break; // sorted array, if > ub then no following will satisfy it
                            }
                        }
                    }
                    if (weight <= 0) {
                        --status[id];       // arc can be satisfied
                    } else {
                        status[id] = -1; // unless we can satisfy the weight, transition not enabled
                    }
                }
            }

            // for inhibitor arcs
            for (auto arc : place.place->getInhibitorArcs()) {
                auto id = arc->getOutputTransition().getIndex();
                // no precheck if it was disabled, actual calculation is just as fast
                if (numtokens >= arc->getWeight())        // we satisfy the inhibitor
                    status[id] = -1;                                        // transition cannot fire
            }

            if (canDelay) { // if delay is still possible
                int invariant = place.place->getInvariant().getBound();
                if (invariant <= place.maxTokenAge() + maxDelay) {       // if we have reached the invariant
                    canDelay = false;                               // we cannot delay
                } else {
                    if (allMC) {                                      // if all up till now have been equal MC
                        allMC = place.allMaximumConstant(maxDelay);   // check if this one is to
                    }
                }
            }
        }

        if (!hasOutArc) {
            delete[] status;
            return true; // if no outgoing arcs, we have a deadlock!
        }


        // if any transition is enabled there is no deadlock
        for (int i = 0; i < count; ++i) {
            if (status[i] == 0) {
                delete[] status;
                return false;
            }
        }
        delete[] status;
        // if we can delay there is no deadlock (needs something with check for possible delay)
        if (canDelay &&
            !ignoreCanDelay) {       // for sure we have a deadlock if all tokens are at MC +1 and we can delay
            // If the tokens are not aged yet, they will eventually reach it but no deadlock reported
            return allMC;
        } else {
            return true;    // we cannot delay, deadlock
        }
    }


    int NonStrictMarkingBase::cut(std::vector<int> &placeCount) {
#ifdef DEBUG
        std::cout << "Before cut: " << *this << std::endl;
#endif
        int maxDelay = std::numeric_limits<int>::max();
        for (auto& place_list : this->places) {
            // calculate maximum possible delay - used for deadlock query
            int invariant = place_list.place->getInvariant().getBound();
            // this handles case where invariant = inf
            if (invariant != std::numeric_limits<int>::max()) {
                // it should not be possible for a pre-cut marking to acheive an age above the invariant
                int candidate = invariant - place_list.maxTokenAge();
                // maximum possible delay for tokens seen so far
                maxDelay = std::min(maxDelay, candidate);
            }
            //set age of too old tokens to max age
            int count = 0;
            int total = 0;
            for (auto token_iter = place_list.tokens.begin();
                 token_iter != place_list.tokens.end(); token_iter++) {
                if (token_iter->getAge() >
                    place_list.place->getMaxConstant()) { // this will also removed dead tokens
                    auto beginDelete = token_iter;
                    if (place_list.place->getType() == TAPN::Std) {
                        for (; token_iter != place_list.tokens.end(); token_iter++) {
                            count += token_iter->getCount();
                        }
                    } else if (place_list.place->getType() == TAPN::Dead) {
                        // if we remove some dead tokens update place statistics
                        if (!place_list.tokens.empty()) {
                            placeCount[place_list.place->getIndex()] = std::numeric_limits<int>::max();
                        }
                    }
                    this->removeRangeOfTokens(place_list, beginDelete, place_list.tokens.end());
                    break;
                } else {
                    total += token_iter->getCount();
                }
            }

            if (count) {
                Token t(place_list.place->getMaxConstant() + 1, count);
                this->addTokenInPlace(place_list, t);
            }

            // update place statistics
            total += count;
            placeCount[place_list.place->getIndex()] = std::max(total, placeCount[place_list.place->getIndex()]);
        }
        this->cleanUp();
        return maxDelay;
#ifdef DEBUG
        std::cout << "After cut: " << *this << std::endl;
#endif
    }

    int NonStrictMarkingBase::getYoungest() {
        int youngest = std::numeric_limits<int32_t>::max();
        for (const auto &place_iter : getPlaceList()) {
            if (youngest > place_iter.tokens.front().getAge() &&
                place_iter.tokens.front().getAge() <= place_iter.place->getMaxConstant()) {
                youngest = place_iter.tokens.front().getAge();
            }
        }

        if (youngest == std::numeric_limits<int32_t>::max()) {
            youngest = 0;
        }
        return youngest;
    }

    int NonStrictMarkingBase::makeBase() {
#ifdef DEBUG
        std::cout << "Before makeBase: " << *this << std::endl;
#endif
        int youngest = getYoungest();

        for (auto &place : places) {
            for (auto& token : place.tokens) {
                if (token.getAge() != place.place->getMaxConstant() + 1) {
                    token.setAge(token.getAge() - youngest);
                }
#ifdef DEBUG
                else if (token_iter->getAge() > place_iter->place->getMaxConstant() + 1) {
                    assert(false);
                }
#endif
            }
        }

#ifdef DEBUG
        std::cout << "After makeBase: " << *this << std::endl;
        std::cout << "Youngest: " << youngest << std::endl;
#endif

        return youngest;
    }

} } /* namespace VerifyTAPN */
