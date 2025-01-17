#ifndef RealMarking_HPP
#define RealMarking_HPP

#include <vector>
#include "Core/TAPN/TAPN.hpp"
#include "DiscreteVerification/DataStructures/NonStrictMarkingBase.hpp"

namespace VerifyTAPN::DiscreteVerification {

    class RealToken {

        private:

            double age;
            int count;

        public:

            RealToken(double age, int count) : age(age), count(count) { }
            RealToken(const RealToken&) = default;
            RealToken(const Token& t) : count(t.getCount()) {
                age = (double) t.getAge();
            }

            inline int cmp(const RealToken &t) const {
                if (count != t.count) return count - t.count;
                return age - t.age > 0 ? 1 : -1;
            }

            inline bool equals(const RealToken &t) const { return (this->age == t.age && this->count == t.count); };

            inline void add(int num) { count = count + num; };

            inline int getCount() const { return count; };

            inline double getAge() const { return age; };

            inline void setAge(double i) { age = i; };

            inline void setCount(int i) { count = i; };

            inline void remove(int num) { count = count - num; };

            inline void deltaAge(double x) {
                age += x;
            }

    };

    typedef std::vector<RealToken> RealTokenList;

    class RealPlace {

        public:

            const TAPN::TimedPlace* place;
            RealTokenList tokens;

            explicit RealPlace(const TAPN::TimedPlace* place) : place(place) { }
            
            RealPlace(const RealPlace& p) : place(p.place) {
                tokens = p.tokens;
            };

            RealPlace(const Place& p) : place(p.place) {
                for(const auto& token : p.tokens) {
                    tokens.push_back(RealToken(token));
                }
            }

            inline Place generateImagePlace() const {
                Place img(place);
                img.tokens.push_back(Token(0, numberOfTokens()));
                return img;
            }

            inline int numberOfTokens() const {
                int count = 0;
                for(const auto& token : tokens) {
                    count += token.getCount();
                }
                return count;
            }

            inline void deltaAge(double x) {
                for(auto& token : tokens) {
                    token.deltaAge(x);
                }
            }

            inline double maxTokenAge() const {
                if(tokens.size() == 0) {
                    return -std::numeric_limits<double>::infinity();
                }
                return tokens.back().getAge();
            }

            void add(RealToken new_token);

            void add(double age = 0.0f) {
                add(RealToken(age, 1));
            }

            bool remove(RealToken to_remove);

            double availableDelay() const {
                if(tokens.size() == 0) return std::numeric_limits<double>::infinity();
                double delay = ((double) place->getInvariant().getBound()) - maxTokenAge();
                return delay <= 0.0f ? 0.0f : delay;
            }

            inline int placeId() const {
                return place->getIndex();
            }

            inline bool isEmpty() const {
                return tokens.size() == 0;
            }

    };

    typedef std::vector<RealPlace> RealPlaceList;

    class RealMarking {

        public:

            explicit RealMarking(TAPN::TimedArcPetriNet* net, NonStrictMarkingBase& base);
            RealMarking(const RealMarking& other);

            uint32_t size() const;
    
            RealPlaceList& getPlaceList();
            RealTokenList& getTokenList(int placeId);

            void deltaAge(double x);

            NonStrictMarkingBase generateImage();

            uint32_t numberOfTokensInPlace(int placeId) const;

            bool canDeadlock(const TAPN::TimedArcPetriNet &tapn, int maxDelay, bool ignoreCanDelay) const;

            inline bool canDeadlock(const TAPN::TimedArcPetriNet &tapn, const int maxDelay) const {
                return canDeadlock(tapn, maxDelay, false);
            };

            bool removeToken(int placeId, double age);

            bool removeToken(int placeId, RealToken &token);

            bool removeToken(RealPlace &place, RealToken &token);

            void addTokenInPlace(TAPN::TimedPlace &place, double age = 0.0f);

            void addTokenInPlace(RealPlace &place, RealToken &token);

            void addTokenInPlace(const TAPN::TimedPlace &place, RealToken &token);

            double availableDelay() const;

            void setDeadlocked(const bool dead);

            inline const TAPN::TimedTransition *getGeneratedBy() const { return generatedBy; }

            inline void setGeneratedBy(const TAPN::TimedTransition *generatedBy) { this->generatedBy = generatedBy; }

            inline double getPreviousDelay() const { return fromDelay; }

            inline double getTotalAge() const { return totalAge; }

            inline void setPreviousDelay(const double delay) { this->fromDelay = delay; }

            bool enables(TAPN::TimedTransition* transition);

            unsigned int _thread_id = 0;

        private:

            RealPlaceList places;
            bool deadlocked;

            const TAPN::TimedTransition *generatedBy = nullptr;
            double fromDelay = 0.0;
            double totalAge = 0.0;

            static RealTokenList emptyTokenList;

    };

}

#endif