#ifndef INHIBITORARC_HPP_
#define INHIBITORARC_HPP_

#include "TimeInterval.hpp"

#include <vector>

namespace VerifyTAPN { namespace TAPN {
    class TimedTransition;

    class TimedPlace;

    class InhibitorArc {
    public: // typedefs
        typedef std::vector<InhibitorArc *> Vector;
    public:
        InhibitorArc(TimedPlace &place, TimedTransition &transition, const int weight) : place(place),
                                                                                         transition(transition),
                                                                                         weight(weight) {};

        virtual ~InhibitorArc() = default;

    public: // modifiers
        inline TimedPlace &getInputPlace() const { return place; }

        inline TimedTransition &getOutputTransition() const { return transition; }

    public: // Inspectors
        void print(std::ostream &out) const;

        inline uint32_t getWeight() const { return weight; }

    private:
        TimedPlace &place;
        TimedTransition &transition;
        const uint32_t weight;
    };

    inline std::ostream &operator<<(std::ostream &out, const InhibitorArc &arc) {
        arc.print(out);
        return out;
    }
} }

#endif /* INHIBITORARC_HPP_ */
