/* 
 * File:   PTrieMarkingStore.h
 * Author: Peter G. Jensen
 *
 * Created on 21 September 2015, 18:08
 */

#ifndef PTRIEMARKINGSTORE_H
#define    PTRIEMARKINGSTORE_H

#include "MetaData.h"
#include "NonStrictMarkingBase.hpp"
#include "MarkingStore.h"
#include "MarkingEncoder.h"
#include "ptrie.h"

namespace VerifyTAPN { namespace DiscreteVerification {
    template<typename T = MetaData *>
    class PTrieMarkingStore : public MarkingStore<T> {
    protected:
        class Iterator : public MarkingStore<T>::Iterator {
        private:
            const ptrie_t<T>& map;
            size_t id;
        public:
            Iterator(const ptrie_t<T>& map, size_t id) : map(map), id(id) {
            }
            
            virtual void next() {
                ++id;
            }
            
            virtual bool done() {
                return id >= map.size();
            }
            
            virtual typename MarkingStore<T>::Pointer* operator*() const
            {
                return reinterpret_cast<typename MarkingStore<T>::Pointer*>(id);
            }
            
        };
        
        MarkingEncoder<T, NonStrictMarkingBase> encoder;
        ptrie_t<T> store;
    public:

        /**
         * Default constructor
         */
        PTrieMarkingStore(TAPN::TimedArcPetriNet &tapn, int knumber)
                : MarkingStore<T>(),
                  encoder(tapn, knumber),
                  store() {}

        virtual ~PTrieMarkingStore() = default;

        /**
         *
         * @param m A marking to insert
         * @return PAIR (bool, Pointer). bool = true if inserted, false if not.
         * Pointer points to the newly inserted, or existing marking.
         */
        virtual
        typename
        MarkingStore<T>::result_t insert_and_dealloc(NonStrictMarkingBase *m) {

            std::pair<bool, ptriepointer_t<T> > res =
                    store.insert(encoder.encode(m));
            delete m;
            if (res.first) this->stored += 1;
            auto *
                    p = reinterpret_cast<typename MarkingStore<T>::Pointer *>(res.second.index);
            return typename MarkingStore<T>::result_t(res.first, p);
        }

        /**
         * Method for reconstructing a marking from a given pointer. The
         * template parameter decides (if needed) how the pointer is reconstructed
         * @param p
         * @return an expanded marking of type U.
         */
        virtual
        NonStrictMarkingBase *expand(typename MarkingStore<T>::Pointer *p) {
            ptriepointer_t<T> pointer = ptriepointer_t<T>(&store,
                                                          reinterpret_cast<size_t>(p));
            NonStrictMarkingBase *m = encoder.decode(pointer);
            return m;
        }

        /**
         * Method for freeing/deleting markings returned from expand.
         * @param m a marking to free
         */
        virtual
        void free(NonStrictMarkingBase *m) {
            delete m;
        };

        virtual
        T &get_meta(typename MarkingStore<T>::Pointer *p) {
            ptriepointer_t<T> pointer = ptriepointer_t<T>(&store,
                                                          reinterpret_cast<size_t>(p));
            return pointer.get_meta();
        }

        virtual
        void set_meta(typename MarkingStore<T>::Pointer *p, T &meta) {
            ptriepointer_t<T> pointer = ptriepointer_t<T>(&store,
                                                          reinterpret_cast<size_t>(p));
            pointer.set_meta(meta);
        }

        virtual typename MarkingStore<T>::Iterator* begin() const override {
            return new Iterator(store, 0);
        }
        
    };
} }

#endif    /* PTRIEMARKINGSTORE_H */

