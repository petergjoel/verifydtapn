#include "Core/TAPN/TimedTransition.hpp"
#include "Core/TAPN/TimedPlace.hpp"

#include <cstdlib>

namespace VerifyTAPN {
namespace TAPN {

    void TimedTransition::print(std::ostream &out) const {
        out << getName();
        if (this->urgent)
            out << " urgent ";
        out << "(" << index << ")";
    }

    void TimedTransition::addToPreset(TimedInputArc* arc) {

        if (this->urgent) { // all urgency in discrete time must have untimed
            //inputarcs to not break semantics
            if (!arc->getInterval().isZeroInfinity()) {
                std::cout << "Urgent transitions must have untimed input arcs" << std::endl;
               std::exit(1);
            }
        }

        auto lb = std::lower_bound(preset.begin(), preset.end(), arc, [](auto* a, auto* b)
        {
            return a->getInputPlace().getIndex() < b->getInputPlace().getIndex();
        });
        preset.insert(lb, arc);
    }

    void TimedTransition::addTransportArcGoingThrough(TransportArc* arc) {
        if (this->urgent) { // all urgency in discrete time must have untimed
            //inputarcs to not break semantics
            if (!arc->getInterval().isZeroInfinity()) {
                std::cout << "Urgent transitions must have untimed transportarcs" << std::endl;
               std::exit(1);
            } else if (arc->getDestination().getInvariant() != TimeInvariant::LS_INF) {
                // urgency breaks if we have invariant at destination
                std::cout
                        << "Transportarcs going through an urgent transition cannot have invariants at destination-places."
                        << std::endl;
               std::exit(1);
            }
        }


        auto lb = std::lower_bound(transportArcs.begin(), transportArcs.end(), arc, [](auto* a, auto* b)
        {
            return a->getSource().getIndex() < b->getSource().getIndex();
        });
        transportArcs.insert(lb, arc);
    }

    void TimedTransition::addIncomingInhibitorArc(InhibitorArc* arc) {
        auto lb = std::lower_bound(inhibitorArcs.begin(), inhibitorArcs.end(), arc, [](auto* a, auto* b)
        {
            return a->getInputPlace().getIndex() < b->getInputPlace().getIndex();
        });
        inhibitorArcs.insert(lb, arc);
    }

    void TimedTransition::addToPostset(OutputArc* arc) {
        auto lb = std::lower_bound(postset.begin(), postset.end(), arc, [](auto* a, auto* b)
        {
            return a->getOutputPlace().getIndex() < b->getOutputPlace().getIndex();
        });
        postset.insert(lb, arc);
    }
}
}
