/* 
 * File:   SafetySynthesis.h
 * Author: Peter G. Jensen
 *
 * Created on 14 September 2015, 15:59
 */

#ifndef SAFETYSYNTHESIS_H
#define    SAFETYSYNTHESIS_H

#include "DiscreteVerification/DataStructures/MarkingStore.h"
#include "Core/TAPN/TAPN.hpp"
#include "DiscreteVerification/QueryVisitor.hpp"
#include "DiscreteVerification/DataStructures/NonStrictMarkingBase.hpp"
#include "DiscreteVerification/Generators/GameGenerator.h"
#include "DiscreteVerification/DataStructures/Waiting.h"

#include <forward_list>

namespace VerifyTAPN { namespace DiscreteVerification {


    class SafetySynthesis {
    private:
        struct SafetyMeta;
        typedef MarkingStore<SafetyMeta> store_t;
        typedef weightedqueue_t<store_t::Pointer *> waiting_t;
        typedef std::stack<store_t::Pointer *> backstack_t;

        typedef std::pair<bool, store_t::Pointer *> depender_t;
        typedef std::forward_list<depender_t> depends_t;

        enum MarkingState {
            UNKNOWN = 0,            // no successors generated yet
            PROCESSED = 1,          // Generated successors
            MAYBE_WINNING = 2,      // If no env strategy, then winning
            MAYBE_LOSING = 3,       // If no ctrl strategy, then losing 
            LOOSING = 4,            // env wins
            WINNING = 5
        };           // ctrl surely wins

        struct SafetyMeta {
            uint8_t state;
            bool waiting;                       // We only need stuff on waiting once
            bool printed = false;
            size_t ctrl_children;                // Usefull.
            size_t env_children;
            depends_t dependers;                // A punch of parents
        };


        store_t *store;
        waiting_t *waiting;
        TAPN::TimedArcPetriNet &tapn;
        NonStrictMarking &initial_marking;
        AST::Query *query;
        VerificationOptions options;
        std::vector<int> placeStats;
        std::unique_ptr<GameGenerator> generator;
        size_t discovered;
        size_t explored;
        unsigned int largest;
    public:
        SafetySynthesis(
                TAPN::TimedArcPetriNet &tapn,
                NonStrictMarking &initialMarking,
                AST::Query *query,
                VerificationOptions &options);

        bool run();

        virtual ~SafetySynthesis();

        unsigned int max_tokens() { return largest; };

        void print_stats();
        
        void write_strategy(std::ostream& out);

    private:
        bool satisfies_query(NonStrictMarkingBase *m);

        std::vector<store_t::Pointer*> successors(MarkingStore<SafetyMeta>::Pointer *, SafetyMeta &, bool controller, const Query* query);

        void dependers_to_waiting(SafetyMeta &next_meta, backstack_t &waiting);
        void add_successors(store_t::Pointer *parent, SafetyMeta &meta, const std::vector<store_t::Pointer*>& successors, bool is_controller);
        
        bool done() const {
            return waiting->empty();
        }
        
        store_t::Pointer* pop_waiting();
    };
} }

#endif    /* SAFETYSYNTHESIS_H */

