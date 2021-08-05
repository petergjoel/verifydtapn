/* 
 * File:   MarkingStore.h
 * Author: Peter G. Jensen
 *
 * Created on 14 September 2015, 11:14
 */

#ifndef MARKINGSTORE_H
#define    MARKINGSTORE_H

#include "MetaData.h"
#include "NonStrictMarkingBase.hpp"
#include <cinttypes>

/**
 * Base class for storing markings
 */
namespace VerifyTAPN { namespace DiscreteVerification {
    template<typename T = MetaData *>
    class MarkingStore {
    protected:
        size_t stored;
        size_t m_tokens;
    public:

        /**
         * Generic wrapper for a pointer to a marking
         */
        class Pointer {
        public:
            virtual ~Pointer() = default;
        };

        /*
         * inheritance model makes it dificult to follow std pointer types
         * so we build our own simple version
         */
        class Iterator {
        public:
            virtual ~Iterator() = default;
            virtual void next() = 0;
            virtual bool done() = 0;
            virtual Pointer* operator*() const = 0;
        };

        typedef std::pair<bool, Pointer *> result_t;

        /**
         * Detault constructor
         */
        MarkingStore() : stored(0), m_tokens(0) {};

        virtual ~MarkingStore() = default;

        /**
         *
         * @param m A marking to insert
         * @return PAIR (bool, Pointer). bool = true if inserted, false if not.
         * Pointer points to the newly inserted, or existing marking.
         */
        virtual
        result_t insert_and_dealloc(NonStrictMarkingBase *m) = 0;

        /**
         * Method for reconstructing a marking from a given pointer. The
         * template parameter decides (if needed) how the pointer is reconstructed
         * @param p
         * @return an expanded marking of type U.
         */
        virtual
        NonStrictMarkingBase *expand(Pointer *p) = 0;

        /**
         * Method for freeing/deleting markings returned from expand.
         * @param m a marking to free
         */
        virtual
        void free(NonStrictMarkingBase *m) {};

        /**
         * Number of markings currently in the store
         * @return
         */
        virtual
        size_t size() {
            return stored;
        }

        /**
         * Size of the largest marking stored
         * @return
         */
        virtual
        size_t max_tokens() {
            return m_tokens;
        }

        virtual
        T &get_meta(Pointer *p) = 0;

        virtual
        void set_meta(Pointer *p, T &meta) = 0;
        
        virtual Iterator* begin() const = 0;
    };
} }
#endif    /* MARKINGSTORE_H */

