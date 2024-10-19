#ifndef VERIFYTAPN_TAPN_TimedArcPetriNet_HPP_
#define VERIFYTAPN_TAPN_TimedArcPetriNet_HPP_

#include "TimedPlace.hpp"
#include "TimedTransition.hpp"
#include "TimedInputArc.hpp"
#include "TransportArc.hpp"
#include "InhibitorArc.hpp"
#include "OutputArc.hpp"

#include "Core/Query/AST.hpp"
#include "Core/PlaceVisitor.hpp"
#include "Core/VerificationOptions.hpp"

#include <iostream>
#include <set>
#include "google/sparse_hash_map"
#include <boost/functional/hash.hpp>

namespace VerifyTAPN {
namespace TAPN {

    class TimedArcPetriNet {

    public:// construction
        TimedArcPetriNet(const TimedPlace::Vector &places,
                         const TimedTransition::Vector &transitions,
                         const TimedInputArc::Vector &inputArcs,
                         const OutputArc::Vector &outputArcs,
                         const TransportArc::Vector &transportArcs,
                         const InhibitorArc::Vector &inhibitorArcs)
                : places(places), transitions(transitions), inputArcs(inputArcs), outputArcs(outputArcs),
                  transportArcs(transportArcs), inhibitorArcs(inhibitorArcs) {
                };

        ~TimedArcPetriNet() {
            // call delete on all data
            for (auto &place : places) delete place;
            for (auto &transition : transitions) delete transition;
            for (auto &inputArc : inputArcs) delete inputArc;
            for (auto &outputArc : outputArcs) delete outputArc;
            for (auto &transportArc : transportArcs) delete transportArc;
            for (auto &inhibitorArc : inhibitorArcs) delete inhibitorArc;
        }

    public: // inspectors
        void print(std::ostream &out) const;

        inline int getPlaceIndex(const TimedPlace &p) const { return p.getIndex(); };

        int getPlaceIndex(const std::string &placeName) const;

        const TimedPlace &getPlace(const int placeIndex) const { return *places[placeIndex]; }

        const TimedTransition::Vector &getTransitions() const { return transitions; }

        const TimedInputArc::Vector &getInputArcs() const { return inputArcs; }

        const TransportArc::Vector &getTransportArcs() const { return transportArcs; }

        const InhibitorArc::Vector &getInhibitorArcs() const { return inhibitorArcs; }

        const TimedPlace::Vector &getPlaces() const { return places; };

        int getNumberOfConsumingArcs() const { return inputArcs.size() + transportArcs.size(); }

        const OutputArc::Vector &getOutputArcs() const { return outputArcs; }

        int getNumberOfOutputArcs() const { return outputArcs.size(); }

        int getNumberOfPlaces() const { return places.size(); };

        inline int getMaxConstant() const { return maxConstant; };

        inline int getGCD() const { return gcd; };

        inline bool isPlaceAtIndexUntimed(int index) const { return places[index]->isUntimed(); }

        bool isNonStrict() const;

        void calculateCausality(TimedPlace &p, std::vector<TimedPlace *> *result) const;

        void updatePlaceTypes(AST::Query *query, const VerificationOptions &options);

        void setAllControllable(bool value);

    public: // modifiers
        void initialize(const VerificationOptions &options);

        bool containsOrphanTransitions();

        void GCDLowerGuards();

        void toTAPNXML(std::ostream& out, const std::vector<int>& initial) const;



    private: // modifiers

        void updateMaxConstant(const TimeInterval &interval);

        void updateMaxConstant(const TimeInvariant &invariant);

        void markUntimedPlaces();

        void findMaxConstants(const VerificationOptions &options);

    private: // data
        TimedPlace::Vector places;
        TimedTransition::Vector transitions;
        TimedInputArc::Vector inputArcs;
        OutputArc::Vector outputArcs;
        TransportArc::Vector transportArcs;
        InhibitorArc::Vector inhibitorArcs;
        int maxConstant = 0;
        int gcd = 1;
    };


    inline std::ostream &operator<<(std::ostream &out, const VerifyTAPN::TAPN::TimedArcPetriNet &tapn) {
        tapn.print(out);
        return out;
    }

    inline std::size_t getHashValue(const TimedTransition &transition) {
        boost::hash<std::string> hasher;
        return hasher(transition.getName());
    }
}
}

#endif /* VERIFYTAPN_TAPN_TimedArcPetriNet_HPP_ */
