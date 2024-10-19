#include "Core/TAPN/TimedArcPetriNet.hpp"
#include "Core/TAPN/TimeInterval.hpp"

#include <limits>
#include <numeric>
#include <string>

namespace VerifyTAPN {
namespace TAPN {
    void TimedArcPetriNet::initialize(const VerificationOptions &options) {
        bool lowerGuardsByGCD = options.getGCDLowerGuardsEnabled();
        bool useGlobalMaxConstant = options.getGlobalMaxConstantsEnabled();

        // start by doing GCD if enabled
        if (lowerGuardsByGCD) {
            GCDLowerGuards();
        }

        for (unsigned int i = 0; i < places.size(); i++) {
            updateMaxConstant(places[i]->getInvariant());
        }

        for (auto* arc : inputArcs) {
            updateMaxConstant(arc->getInterval());
        }

        for (auto* arc : transportArcs) {
            updateMaxConstant(arc->getInterval());
        }

        findMaxConstants(options);

        if (useGlobalMaxConstant) {
            for (auto* place : places) {
                place->setMaxConstant(getMaxConstant() == 0 ? -1 : getMaxConstant());
            }
        }

        markUntimedPlaces();
    }

    bool TimedArcPetriNet::containsOrphanTransitions() {
        if (!transitions.empty()) {
            auto iter = transitions.begin();
            while (iter != transitions.end()) {
                if ((*iter)->getPresetSize() == 0 && (*iter)->getPostsetSize() == 0) {
                    return true;
                } else {
                    iter++;
                }
            }
        }
        return false;
    }

    void TimedArcPetriNet::GCDLowerGuards() {
        std::set<int> constants;

        for (auto* place : places) {
            constants.insert(place->getInvariant().getBound());
        }

        for (auto* inputArc : inputArcs) {
            constants.insert(inputArc->getInterval().getLowerBound());
            constants.insert(inputArc->getInterval().getUpperBound());
        }
        for (auto* transportArc : transportArcs) {
            constants.insert(transportArc->getInterval().getLowerBound());
            constants.insert(transportArc->getInterval().getUpperBound());
        }

        constants.erase(0);
        constants.erase(std::numeric_limits<int>::max());

        if (constants.empty()) {
            return; // no constants other than 0 or inf
        }

        int divider = *constants.begin();
        for (int constant : constants) {
            divider = std::gcd(divider, constant);
        }


        if (divider <= 1)
            return;

        gcd = divider;

        for (auto* place : places) {
            place->divideInvariantBy(gcd);
        }

        for (auto* inputArc : inputArcs) {
            inputArc->divideIntervalBy(gcd);
        }
        for (auto* transportArc : transportArcs) {
            transportArc->divideIntervalBy(gcd);
        }
    }

    void TimedArcPetriNet::markUntimedPlaces() {
        for (auto* place : places) {
            bool isUntimedPlace = place->getInvariant() == TimeInvariant::LS_INF;

            for (auto* transportArc : transportArcs) {
                isUntimedPlace = isUntimedPlace && &transportArc->getSource() != place;
            }

            if (isUntimedPlace) {
                for (auto* inputArc : inputArcs) {
                    if (&inputArc->getInputPlace() == place) {
                        isUntimedPlace = isUntimedPlace && inputArc->getInterval().isZeroInfinity();
                    }
                }
            }

            if (isUntimedPlace) place->markPlaceAsUntimed();
        }
    }

    void TimedArcPetriNet::findMaxConstants(const VerificationOptions &options) {
        for (auto* place : places) {
            int maxConstant = -1;

            // If using fastest trace the value is initialized to 0 to avoid issues with wrong delays
            // See https://bugs.launchpad.net/tapaal/+bug/2035819
            maxConstant = options.getTrace() == VerificationOptions::FASTEST_TRACE ? 0 : -1;

            if (place->getInvariant() != TimeInvariant::LS_INF) {
                maxConstant = place->getInvariant().getBound();
                place->setMaxConstant(maxConstant);
                place->setType(Inv);
            } else {
                place->setType(Dead);
                for (auto* inputArc : inputArcs) {
                    if (&inputArc->getInputPlace() == place) {
                        TimedInputArc *ia = inputArc;
                        const TAPN::TimeInterval &interval = ia->getInterval();

                        const int lowerBound = interval.getLowerBound();
                        const int upperBound = interval.getUpperBound();

                        if (upperBound != std::numeric_limits<int>::max() || lowerBound != 0) {
                            if (upperBound == std::numeric_limits<int>::max()) {
                                maxConstant = (maxConstant < lowerBound ? lowerBound : maxConstant);
                                place->setType(Std);
                            } else {
                                maxConstant = (maxConstant < upperBound ? upperBound : maxConstant);
                            }
                        } else {
                            place->setType(Std);
                        }
                    }
                }
                for (auto* transportArc : transportArcs) {
                    if (&transportArc->getSource() == place) {
                        int maxArc = -1;
                        TransportArc *ta = transportArc;
                        const TAPN::TimeInterval &interval = ta->getInterval();
                        const int lowerBound = interval.getLowerBound();
                        const int upperBound = interval.getUpperBound();

                        if (upperBound != std::numeric_limits<int>::max() || lowerBound != 0) {
                            if (upperBound == std::numeric_limits<int>::max()) {
                                maxArc = lowerBound;
                                place->setType(Std);
                            } else {
                                maxArc = upperBound;
                            }
                        } else {
                            place->setType(Std);
                        }
                        int destinationInvariant = ta->getDestination().getInvariant().getBound();
                        if (destinationInvariant != std::numeric_limits<int>::max()) {
                            maxArc = maxArc < destinationInvariant ? maxArc : destinationInvariant;
                        }
                        maxConstant = maxConstant < maxArc ? maxArc : maxConstant;
                    }
                }
        
                place->setMaxConstant(maxConstant);

                for (auto* inhibitorArc : inhibitorArcs) {
                    if (inhibitorArc->getInputPlace().getIndex() == place->getIndex() && place->getType() == Dead) {
                        place->setType(Std);
                    }
                }
            }
        }

        for (auto* place : places) {
            std::vector<TimedPlace *> causalitySet;
            calculateCausality(*place, &causalitySet);
            for (auto cau_iter : causalitySet) {
                if (cau_iter->getMaxConstant() > place->getMaxConstant()) {
                    place->setMaxConstant(cau_iter->getMaxConstant());
                }
            }
        }

        for (auto* transition : transitions) {
            for (auto* place_iter : transition->getPostset()) {
                if (place_iter->getOutputPlace().getMaxConstant() > -1) {
                    transition->setUntimedPostset(false);
                    break;
                }
            }
        }
    }

    void TimedArcPetriNet::calculateCausality(TimedPlace &p, std::vector<TimedPlace *> *result) const {
        for (auto* iter : *result) {
            if (iter == &p) return;
        }
        result->push_back(&p);
        for (auto* iter : this->getTransportArcs()) {
            if (&iter->getSource() == &p) {
                if (iter->getInterval().getUpperBound() == std::numeric_limits<int>::max()) {
                    calculateCausality(iter->getDestination(), result);
                }
            }
        }
    }

    void TimedArcPetriNet::updatePlaceTypes(AST::Query *query, const VerificationOptions &options) {

        AST::IntVectorResult placeNumbers;
        PlaceVisitor visitor;
        query->accept(visitor, placeNumbers);

        for (auto* place : places) {
            if (options.getKeepDeadTokens() && place->getType() == Dead) {
                place->setType(Std);
                continue;
            }
            for (int id_iter : placeNumbers.value) {
                if (id_iter == place->getIndex() && place->getType() == Dead) {
                    place->setType(Std);
                    break;
                }
            }
        }
    }

    void TimedArcPetriNet::setAllControllable(bool value) {
        for (auto* transition : transitions) {
            transition->setControllable(value);
        }
    }

    void TimedArcPetriNet::updateMaxConstant(const TimeInterval &interval) {
        int lowerBound = interval.getLowerBound();
        int upperBound = interval.getUpperBound();
        if (lowerBound < std::numeric_limits<int>::max() && lowerBound > maxConstant) {
            maxConstant = lowerBound;
        }
        if (upperBound < std::numeric_limits<int>::max() && upperBound > maxConstant) {
            maxConstant = upperBound;
        }
    }

    void TimedArcPetriNet::updateMaxConstant(const TimeInvariant &invariant) {
        int bound = invariant.getBound();
        if (bound < std::numeric_limits<int>::max() && bound > maxConstant) {
            maxConstant = bound;
        }
    }

    int TimedArcPetriNet::getPlaceIndex(const std::string &placeName) const {
        int idx = TimedPlace::BottomIndex();
        for (unsigned int i = 0; i < places.size(); ++i) {
            if (places[i]->getName() == placeName) {
                idx = i;
                break;
            }
        }

        return idx;
    }

    void TimedArcPetriNet::print(std::ostream &out) const {
        out << "TAPN:" << std::endl << "  Places: ";
        for (auto* place : places) {
            out << *place;
            out << std::endl;
        }

        out << std::endl << "  Transitions: ";
        for (auto* transition : transitions) {
            out << *transition;
            out << std::endl;
        }

        out << std::endl << "  Input Arcs: ";
        for (auto* inputArc : inputArcs) {
            out << *inputArc;
            out << ", ";
        }

        if (!transportArcs.empty()) {
            out << std::endl << "  Transport Arcs: ";
            for (auto* transportArc : transportArcs) {
                out << *transportArc;
                out << ", ";
            }
        }

        if (!inhibitorArcs.empty()) {
            out << std::endl << "  Inhibitor Arcs: ";
            for (auto* inhibitorArc : inhibitorArcs) {
                out << *inhibitorArc;
                out << ", ";
            }
        }


        out << std::endl << "  Output Arcs: ";
        for (auto* outputArc : outputArcs) {
            out << *outputArc;
            out << ", ";
        }

        out << std::endl;
    }

    void TimedArcPetriNet::toTAPNXML(std::ostream& out, const std::vector<int>& initial) const
    {
        out << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<pnml xmlns="http://www.informatik.hu-berlin.de/top/pnml/ptNetb">
<net id="ComposedModel" type="P/T net">
)";
        for (auto* place : places) {
            auto& inv = place->getInvariant();
            auto [x, y] = place->getPosition();
            out << "<place id=\"" << place->getName() << "\" name=\"" << place->getName()
                << "\" invariant=\"";
            if(inv.isBoundStrict())
            {
                if(inv.getBound() == std::numeric_limits<int>::max())
                    out << "&lt; inf";
                else
                    out << "&lt;" << inv.getBound();
            }
            else
            {
                out << "&lt;=" << inv.getBound();
            }
            out << "\" initialMarking=\"" << initial[place->getIndex()] << "\">\n";
            out << "\t<graphics><position x=\"" << x << "\" y=\"" << y << "\" /></graphics>\n";
            out << "</place>\n";
        }

        for (auto* transition : transitions) {
            out << "<transition player=\"" << (transition->isControllable() ? 0 : 1)
                << "\" id=\"" << transition->getName() << "\" name=\"" << transition->getName()
                << "\" urgent=\"" << std::boolalpha << transition->isUrgent() 
                << "\" weight=\"" << (
                    transition->getWeight() == std::numeric_limits<double>::infinity() ?
                    "inf" : std::to_string(transition->getWeight())
                )
                << "\" firingMode=\"" << SMC::firingModeName(transition->getFiringMode())
                << "\" " << transition->getDistribution().toXML()
                << ">\n";
            auto [x, y] = transition->getPosition();
            out << "\t<graphics><position x=\"" << x << "\" y=\"" << y << "\" /></graphics>\n";
            out << "</transition>\n";
        }

        for(auto* ia : inputArcs)
        {
            out << "<inputArc inscription=\"" << ia->getInterval() << "\" weight=\"" << ia->getWeight()
                << "\" source=\"" << ia->getInputPlace().getName() << "\" target=\"" << ia->getOutputTransition().getName() << "\" />\n";
        }

        for(auto* oa : outputArcs)
        {
            out << "<outputArc weight=\"" << oa->getWeight()
                << "\" target=\"" << oa->getOutputPlace().getName()
                << "\" source=\"" << oa->getInputTransition().getName() << "\" />\n";
        }

        for(auto* ia : inhibitorArcs)
        {
            out << "<inhibitorArc inscription=\"[0,inf)\" weight=\"" << ia->getWeight()
                << "\" source=\"" << ia->getInputPlace().getName()
                << "\" target=\"" << ia->getOutputTransition().getName() << "\" />\n";
        }

        for (auto* ta : transportArcs) {
            out << "<transportArc inscription=\"" << ta->getInterval() << "\" "
                << "source=\"" << ta->getSource().getName() << "\" "
                << "transition=\"" << ta->getTransition().getName() << "\" "
                << "target=\"" << ta->getDestination().getName() << "\" weight=\"" << ta->getWeight() << "\" />\n";
        }

        out << "</net></pnml>\n";
    }

    bool TimedArcPetriNet::isNonStrict() const {

        for (auto* inputArc : inputArcs) {
            TimedInputArc &ia = *inputArc;
            if (ia.getInterval().isLowerBoundStrict() || (ia.getInterval().isUpperBoundStrict() &&
                                                          ia.getInterval().getUpperBound() !=
                                                          std::numeric_limits<int>::max())) {
                return false;
            }
        }


        for (auto* transportArc : transportArcs) {
            TransportArc &ta = *transportArc;
            if (ta.getInterval().isLowerBoundStrict() || (ta.getInterval().isUpperBoundStrict() &&
                                                          ta.getInterval().getUpperBound() !=
                                                          std::numeric_limits<int>::max())) {
                return false;
            }
        }

        for (auto* place : places) {
            const TimedPlace &p = *place;
            if (p.getInvariant().isBoundStrict() && p.getInvariant().getBound() != std::numeric_limits<int>::max()) {
                return false;
            }
        }

        return true;
    }
}
}


