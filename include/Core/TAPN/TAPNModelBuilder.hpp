#ifndef VERIFYTAPN_TAPNXMLPARSER_HPP_
#define VERIFYTAPN_TAPNXMLPARSER_HPP_

#include "Core/TAPN/TAPN.hpp"
#include <TAPNBuilderInterface.h>

#include <string>
#include <vector>



namespace VerifyTAPN {
    using namespace VerifyTAPN::TAPN;


    class TAPNModelBuilder : public unfoldtacpn::TAPNBuilderInterface {
    public:

        explicit TAPNModelBuilder() = default;

        virtual ~TAPNModelBuilder() = default;

        virtual void addPlace(const std::string& name,
                int tokens,
                bool strict,
                int bound,
                double x = 0,
                double y = 0) override;

        virtual void addTransition(const std::string &name, int player, bool urgent,
                                            double, double, int distrib_id = 0, std::vector<double> params = std::vector<double>(), double weight = 1.0, int firingMode = 0) override;

        virtual void addInputArc(const std::string &place,
                const std::string &transition,
                bool inhibitor,
                int weight,
                bool lstrict, bool ustrict, int lower, int upper) override;

        virtual void addOutputArc(const std::string& transition,
                const std::string& place,
                int weight) override;

        virtual void addTransportArc(const std::string& source,
                const std::string& transition,
                const std::string& target, int weight,
                bool lstrict, bool ustrict, int lower, int upper) override;

        const std::vector<int>& initialMarking() const {
            return _initialMarking;
        }

        TAPN::TimedArcPetriNet* make_tapn();

    private:
        TimedPlace* find_place(const std::string& pid);
        TimedTransition* find_transition(const std::string& tid);

        std::vector<TimedPlace*> _places;
        std::vector<TimedTransition*> _transitions;
        TimedInputArc::Vector _inputArcs;
        OutputArc::Vector _outputArcs;
        TransportArc::Vector _transportArcs;
        InhibitorArc::Vector _inhibitorArcs;
        std::vector<int> _initialMarking;
    };
}

#endif /* VERIFYTAPN_TAPNXMLPARSER_HPP_ */
