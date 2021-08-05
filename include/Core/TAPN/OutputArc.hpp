#ifndef VERIFYTAPN_TAPN_OUTPUTARC_HPP_
#define VERIFYTAPN_TAPN_OUTPUTARC_HPP_

#include <vector>
#include <iostream>

namespace VerifyTAPN { namespace TAPN {
    class TimedPlace;

    class TimedTransition;

    class OutputArc {
    public: // typedefs
        typedef std::vector<OutputArc *> Vector;
    public:
        OutputArc(TimedTransition &transition, TimedPlace &place, const int weight)
                : transition(transition), place(place), weight(weight) {};

        virtual ~OutputArc() = default;

    public: // modifiers
        TimedPlace &getOutputPlace() const;

        TimedTransition &getInputTransition() const;

    public: // inspectors
        void print(std::ostream &out) const;

        inline uint32_t getWeight() const { return weight; }

    private:
        TimedTransition &transition;
        TimedPlace &place;
        const uint32_t weight;
    };

    inline std::ostream &operator<<(std::ostream &out, const OutputArc &arc) {
        arc.print(out);
        return out;
    }
} }

#endif /* VERIFYTAPN_TAPN_OUTPUTARC_HPP_ */
