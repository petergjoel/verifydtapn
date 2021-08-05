#include "Core/TAPNParser/TAPNXmlParser.hpp"
#include "Core/TAPNParser/util.hpp"

#include <string>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace VerifyTAPN {
    using namespace rapidxml;

    TimedArcPetriNet *TAPNXmlParser::parse(const std::string &filename) const {
        const std::string contents = VerifyTAPN::readFile(filename);
        std::vector<char> charArray(contents.begin(), contents.end());
        charArray.push_back('\0');

        xml_document<> xmldoc;
        xmldoc.parse<0>(&charArray[0]); // we need a char* to the string, and contents.c_str() returns const char*

        xml_node<> *pnml = xmldoc.first_node("pnml");
        if (pnml == 0) throw std::string("invalid file.");

        xml_node<> *root = pnml->first_node("net");
        if (root == 0) throw std::string("invalid file.");

        return parseTAPN(*root);
    }

    std::vector<int> TAPNXmlParser::parseMarking(const std::string &filename, const TimedArcPetriNet &tapn) const {
        const std::string contents = VerifyTAPN::readFile(
                filename); // not sure if this is a good idea, because it copies to string? Maybe the compiler is smart enough not to make a copy
        std::vector<char> charArray(contents.begin(), contents.end());
        charArray.push_back('\0');

        xml_document<> xmldoc;
        xmldoc.parse<0>(&charArray[0]); // we need a char* to the string, and contents.c_str() returns const char*

        xml_node<> *pnml = xmldoc.first_node(
                "pnml"); // we don't need error handling here, since we are parsing the same file as above
        xml_node<> *root = pnml->first_node("net");
        return parseInitialMarking(*root, tapn);
    }

    TimedArcPetriNet *TAPNXmlParser::parseTAPN(const xml_node<> &root) const {
        TimedPlace::Vector places = parsePlaces(root);
        TimedTransition::Vector transitions = parseTransitions(root);

        TAPNXmlParser::ArcCollections arcs = parseArcs(root, places, transitions);

        auto *tapn = new TimedArcPetriNet(places, transitions, arcs.inputArcs, arcs.outputArcs, arcs.transportArcs,
                                          arcs.inhibitorArcs);

        return tapn;
    }

    TimedPlace::Vector TAPNXmlParser::parsePlaces(const xml_node<> &root) const {
        TimedPlace::Vector places;

        xml_node<> *placeNode = root.first_node("place");
        while (placeNode != nullptr) {
            TimedPlace *place = parsePlace(*placeNode);
            places.push_back(place);
            placeNode = placeNode->next_sibling("place");
        }

        return places;
    }

    TimedPlace *TAPNXmlParser::parsePlace(const xml_node<> &placeNode) const {
        std::string id(placeNode.first_attribute("id")->value());
        std::string name(placeNode.first_attribute("name")->value());

        std::string invariantNode = placeNode.first_attribute("invariant")->value();

        TimeInvariant timeInvariant = TimeInvariant::createFor(invariantNode, replace);
        return new TimedPlace(name, id, timeInvariant);
    }

    TimedTransition::Vector TAPNXmlParser::parseTransitions(const xml_node<> &root) const {
        TimedTransition::Vector transitions;

        xml_node<> *transitionNode = root.first_node("transition");
        while (transitionNode != nullptr) {
            TimedTransition *transition = parseTransition(*transitionNode);
            transitions.push_back(transition);
            transitionNode = transitionNode->next_sibling("transition");
        }

        return transitions;
    }

    TimedTransition *TAPNXmlParser::parseTransition(const xml_node<> &transitionNode) const {
        std::string id(transitionNode.first_attribute("id")->value());
        std::string name(transitionNode.first_attribute("name")->value());
        xml_attribute<char> *urgenatt = transitionNode.first_attribute("urgent");
        bool urgent = false;
        if (urgenatt != nullptr) {
            std::string urgentStr = urgenatt->value();
            if (urgentStr == "true") {
                urgent = true;
            }
        }

        bool controllable = true;
        xml_attribute<char> *player = transitionNode.first_attribute("player");
        if (player != nullptr) {
            std::string playerid = player->value();
            controllable = playerid == "0";
        }
        return new TimedTransition(name, id, urgent, controllable);
    }

    TAPNXmlParser::ArcCollections TAPNXmlParser::parseArcs(const xml_node<> &root, const TimedPlace::Vector &places,
                                                           const TimedTransition::Vector &transitions) const {

        TimedInputArc::Vector inputArcs = parseInputArcs(root, places, transitions);
        TransportArc::Vector transportArcs = parseTransportArcs(root, places, transitions);
        InhibitorArc::Vector inhibitorArcs = parseInhibitorArcs(root, places, transitions);
        OutputArc::Vector outputArcs = parseOutputArcs(root, places, transitions);

        return ArcCollections(inputArcs, outputArcs, transportArcs, inhibitorArcs);
    }

    TransportArc::Vector
    TAPNXmlParser::parseTransportArcs(const rapidxml::xml_node<> &root, const TimedPlace::Vector &places,
                                      const TimedTransition::Vector &transitions) const {
        TransportArc::Vector transportArcs;
        xml_node<> *arcNode = root.first_node("transportArc");
        while (arcNode != nullptr) {
            transportArcs.push_back(parseTransportArc(*arcNode, places, transitions));
            arcNode = arcNode->next_sibling("transportArc");
        }
        return transportArcs;
    }

    InhibitorArc::Vector
    TAPNXmlParser::parseInhibitorArcs(const rapidxml::xml_node<> &root, const TimedPlace::Vector &places,
                                      const TimedTransition::Vector &transitions) const {
        InhibitorArc::Vector inhibitorArcs;
        xml_node<> *arcNode = root.first_node("inhibitorArc");
        while (arcNode != nullptr) {
            inhibitorArcs.push_back(parseInhibitorArc(*arcNode, places, transitions));
            arcNode = arcNode->next_sibling("inhibitorArc");
        }
        return inhibitorArcs;
    }

    TimedInputArc::Vector TAPNXmlParser::parseInputArcs(const xml_node<> &root, const TimedPlace::Vector &places,
                                                        const TimedTransition::Vector &transitions) const {
        TimedInputArc::Vector inputArcs;
        xml_node<> *arcNode = root.first_node("inputArc");
        while (arcNode != nullptr) {
            inputArcs.push_back(parseInputArc(*arcNode, places, transitions));
            arcNode = arcNode->next_sibling("inputArc");
        }

        return inputArcs;
    }

    OutputArc::Vector TAPNXmlParser::parseOutputArcs(const xml_node<> &root, const TimedPlace::Vector &places,
                                                     const TimedTransition::Vector &transitions) const {
        OutputArc::Vector outputArcs;
        xml_node<> *arcNode = root.first_node("outputArc");
        while (arcNode != nullptr) {
            outputArcs.push_back(parseOutputArc(*arcNode, places, transitions));
            arcNode = arcNode->next_sibling("outputArc");
        }

        return outputArcs;
    }

    TimedPlace* find_place(const TimedPlace::Vector &places, const char* pid) {
        for(auto* p : places)
            if(p->getName().compare(pid) == 0)
                return p;
        std::cerr << "Could not find place \"" << pid << "\". It must be defined before use." << std::endl;
        exit(-1);
        return nullptr;
    }

    TimedTransition* find_transition(const TimedTransition::Vector& transitions, const char* tid) {
        for(auto* t : transitions)
            if(t->getName().compare(tid) == 0)
                return t;
        std::cerr << "Could not find transition \"" << tid << "\". It must be defined before use." << std::endl;
        exit(-1);
        return nullptr;
    }

    TimedInputArc *TAPNXmlParser::parseInputArc(const rapidxml::xml_node<> &arcNode, const TimedPlace::Vector &places,
                                                const TimedTransition::Vector &transitions) const {
        auto* source = arcNode.first_attribute("source")->value();
        auto* target = arcNode.first_attribute("target")->value();
        TimeInterval tint;
        if(auto ins = arcNode.first_attribute("inscription"))
        {
            std::string interval;
            interval = ins->value();
            tint = TimeInterval::createFor(interval, replace);
        }

        int weight = getWeight(arcNode.first_attribute("weight"));

        auto* place = find_place(places, source);
        auto* transition = find_transition(transitions, target);

        return new TimedInputArc(*place, *transition, weight, tint);
    }

    TransportArc *
    TAPNXmlParser::parseTransportArc(const rapidxml::xml_node<> &arcNode, const TimedPlace::Vector &places,
                                     const TimedTransition::Vector &transitions) const {
        auto* sourceName = arcNode.first_attribute("source")->value();
        auto* transitionName = arcNode.first_attribute("transition")->value();
        auto* targetName = arcNode.first_attribute("target")->value();
        std::string interval;
        TimeInterval tint;
        if(auto ins = arcNode.first_attribute("inscription"))
        {
            interval = ins->value();
            tint = TimeInterval::createFor(interval, replace);
        }

        int weight = getWeight(arcNode.first_attribute("weight"));

        auto* source = find_place(places, sourceName);
        auto* transition = find_transition(transitions, transitionName);
        auto* target = find_place(places, targetName);

        if (!tint.setUpperBound(target->getInvariant().getBound(),
                                target->getInvariant().isBoundStrict())) {
            std::cout << "Invariant on " << target->getName() <<
                      " makes the guard " << interval <<
                      " unsatisfiable for transport arc of transition "
                      << transition->getName() << std::endl;
        }
        return new TransportArc(*source, *transition, *target, tint, weight);
    }

    InhibitorArc *
    TAPNXmlParser::parseInhibitorArc(const rapidxml::xml_node<> &arcNode, const TimedPlace::Vector &places,
                                     const TimedTransition::Vector &transitions) const {
        auto* source = arcNode.first_attribute("source")->value();
        auto* target = arcNode.first_attribute("target")->value();

        int weight = getWeight(arcNode.first_attribute("weight"));

        auto* place = find_place(places, source);
        auto* transition = find_transition(transitions, target);

        return new InhibitorArc(*place, *transition, weight);
    }

    OutputArc *TAPNXmlParser::parseOutputArc(const rapidxml::xml_node<> &arcNode, const TimedPlace::Vector &places,
                                             const TimedTransition::Vector &transitions) const {
        auto* source = arcNode.first_attribute("source")->value();
        auto* target = arcNode.first_attribute("target")->value();

        int weight = getWeight(arcNode.first_attribute("weight"));

        auto* transition = find_transition(transitions, source);
        auto* place = find_place(places, target);

        return new OutputArc(*transition, *place, weight);

    }

    std::vector<int>
    TAPNXmlParser::parseInitialMarking(const rapidxml::xml_node<> &root, const TimedArcPetriNet &tapn) const {
        std::vector<int> markedPlaces;
        xml_node<> *placeNode = root.first_node("place");
        int totalInitTokens = 0;
        while (placeNode != nullptr) {
            std::string initialMarkingValue = "0";
            if(auto init = placeNode->first_attribute("initialMarking"))
                initialMarkingValue = init->value();
            
            std::string placeName(placeNode->first_attribute("name")->value());

            boost::algorithm::trim(initialMarkingValue);

            int nTokens;
            if (replace.count(initialMarkingValue))
                nTokens = replace.at(initialMarkingValue);
            else
                nTokens = boost::lexical_cast<int>(initialMarkingValue);
            totalInitTokens += nTokens;

            if (nTokens > 0) {
                for (int i = 0; i < nTokens; i++) {
                    markedPlaces.push_back(tapn.getPlaceIndex(placeName));
                }
            }

            placeNode = placeNode->next_sibling("place");
        }

        return markedPlaces;
    }

    int TAPNXmlParser::getWeight(xml_attribute<char> *attribute) const {
        int weight = 1;
        if (attribute != nullptr) {
            if (replace.count(attribute->value()))
                weight = replace.at(attribute->value());
            else
                weight = std::atoi(attribute->value());
        }
        return weight;
    }


}
