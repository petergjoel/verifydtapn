#include "DiscreteVerification/DataStructures/RealMarking.hpp"

namespace VerifyTAPN::DiscreteVerification {

void RealPlace::add(RealToken new_token)
{
    size_t index = 0;
    for(auto& token : tokens) {
        if(token.getAge() == new_token.getAge()) {
            token.add(new_token.getCount());
            return;
        }
        if(token.getAge() > new_token.getAge()) break;
        index++;
    }
    tokens.insert(tokens.begin() + index, new_token);
}

bool RealPlace::remove(RealToken to_remove)
{
    size_t index = 0;
    for(auto it = tokens.begin() ; it != tokens.end() ; it++) {
        if(it->getAge() == to_remove.getAge()) {
            it->remove(to_remove.getCount());
            if(it->getCount() == 0) {
                tokens.erase(it);
            }
            return true;
        }
    }
    return false;
}

RealTokenList RealMarking::emptyTokenList = RealTokenList();

RealMarking::RealMarking(TAPN::TimedArcPetriNet* tapn, NonStrictMarkingBase& base)
{
    size_t n_places = tapn->getNumberOfPlaces();
    auto placeList = base.getPlaceList();
    auto pit = placeList.begin();
    for(int i = 0 ; i < n_places ; i++) {
        if(pit != placeList.end() && pit->place->getIndex() == i) {
            places.push_back(RealPlace(*pit));
            pit++;
        } else {
            places.push_back(RealPlace(&tapn->getPlace(i)));
        }
    }
}

RealMarking::RealMarking(const RealMarking& other)
{
    places = other.places;
    deadlocked = other.deadlocked;
    totalAge = other.totalAge;
    _thread_id = other._thread_id;
}

uint32_t RealMarking::size() const
{
    uint32_t size = 0;
    for(const auto& place : places) {
        size += place.numberOfTokens();
    }
    return size;
}

RealPlaceList& RealMarking::getPlaceList()
{
    return places;
}

RealTokenList& RealMarking::getTokenList(int placeId)
{
    return places[placeId].tokens;
}

void RealMarking::deltaAge(double x)
{
    for(auto& place : places) {
        place.deltaAge(x);
    }
    totalAge += x;
}

NonStrictMarkingBase RealMarking::generateImage()
{
    NonStrictMarkingBase marking;
    for(const auto& place : places) {
        if(place.isEmpty()) continue;
        Place new_place = place.generateImagePlace();
        marking.addTokenInPlace(*new_place.place, new_place.tokens.front());
    }
    return marking;
}

uint32_t RealMarking::numberOfTokensInPlace(int placeId) const
{
    return places[placeId].numberOfTokens();
}

bool RealMarking::canDeadlock(const TAPN::TimedArcPetriNet &tapn, int maxDelay, bool ignoreCanDelay) const
{
    return deadlocked;
}

bool RealMarking::removeToken(int placeId, double age)
{
    RealToken token(age, 1);
    return removeToken(placeId, token);
}

bool RealMarking::removeToken(int placeId, RealToken& token)
{
    return removeToken(places[placeId], token);
}

bool RealMarking::removeToken(RealPlace &place, RealToken &token)
{
    return place.remove(token);
}

void RealMarking::addTokenInPlace(TAPN::TimedPlace &place, double age)
{
    RealToken token(age, 1);
    addTokenInPlace(place, token);
}

void RealMarking::addTokenInPlace(RealPlace &place, RealToken &token)
{
    place.add(token);
}

void RealMarking::addTokenInPlace(const TAPN::TimedPlace &place, RealToken &token)
{
    places[place.getIndex()].add(token);
}

double RealMarking::availableDelay() const
{
    double available = std::numeric_limits<double>::infinity();
    for(const auto& place : places) {
        if(place.isEmpty()) continue;
        double delay = place.availableDelay();
        if(delay < available) {
            available = delay;
        }
    }
    return available;
}

void RealMarking::setDeadlocked(const bool dead) 
{
    deadlocked = dead;
}

bool RealMarking::enables(TAPN::TimedTransition* transition) {
    for(auto input : transition->getInhibitorArcs()) {
        uint32_t weight = input->getWeight();
        RealTokenList tokens = getTokenList(input->getInputPlace().getIndex());
        for(auto& token : tokens) {
            if(token.getCount() > weight) {
                weight = 0;
            } else {
                weight -= token.getCount();
            }
            if(weight == 0) break;
        }
        if(weight == 0) return false;
    }
    for(auto input : transition->getPreset()) {
        TAPN::TimeInterval interval = input->getInterval();
        uint32_t weight = input->getWeight();
        RealTokenList tokens = getTokenList(input->getInputPlace().getIndex());
        for(auto& token : tokens) {
            if(interval.contains(token.getAge())) {
                if(token.getCount() > weight) {
                    weight = 0;
                } else {
                    weight -= token.getCount();
                }
            }
            if(weight == 0) break;
        }
        if(weight > 0) return false;
    }
    for(auto input : transition->getTransportArcs()) {
        TAPN::TimedPlace& outputPlace = input->getDestination();
        TAPN::TimeInterval interval = input->getInterval();
        if(outputPlace.getInvariant().getBound() < interval.getUpperBound())
            interval.setUpperBound(outputPlace.getInvariant().getBound(), false);
        uint32_t weight = input->getWeight();
        RealTokenList tokens = getTokenList(input->getSource().getIndex());
        for(auto& token : tokens) {
            if(interval.contains(token.getAge())) {
                if(token.getCount() > weight) {
                    weight = 0;
                } else {
                    weight -= token.getCount();
                }
            }
            if(weight == 0) break;
        }
        if(weight > 0) return false;
    }
    return true;
}

}