#ifndef VERIFICATIONOPTIONS_HPP_
#define VERIFICATIONOPTIONS_HPP_

#include <string>
#include <iosfwd>
#include <utility>
#include <utility>
#include <vector>
#include <map>
#include <set>

namespace VerifyTAPN {

    class VerificationOptions {
    public:

        enum Trace {
            NO_TRACE, SOME_TRACE, FASTEST_TRACE
        };

        enum SearchType {
            BREADTHFIRST, DEPTHFIRST, RANDOM, COVERMOST, DEFAULT, MINDELAYFIRST
        };

        enum VerificationType {
            DISCRETE, TIMEDART
        };

        enum MemoryOptimization {
            NO_MEMORY_OPTIMIZATION, PTRIE
        };

        enum WorkflowMode {
            NOT_WORKFLOW, WORKFLOW_SOUNDNESS, WORKFLOW_STRONG_SOUNDNESS
        };

        VerificationOptions() = default;

        VerificationOptions(
                SearchType searchType,
                VerificationType verificationType,
                MemoryOptimization memOptimization,
                unsigned int k_bound,
                Trace trace,
                bool xml_trace,
                bool useGlobalMaxConstants,
                bool keepDeadTokens,
                bool enableGCDLowerGuards,
                WorkflowMode workflow,
                long long workflowBound,
                bool calculateCmax,
                std::map<std::string, int> replace,
                bool order,
                std::string outputFile,
                std::string outputQuery,
                std::string outputXMLQuery,
                std::set<size_t> querynumbers
        ) : inputFile(""),
            queryFile(""),
            searchType(searchType),
            verificationType(verificationType),
            memOptimization(memOptimization),
            k_bound(k_bound),
            trace(trace),
            xml_trace(xml_trace),
            useGlobalMaxConstants(useGlobalMaxConstants),
            keepDeadTokens(keepDeadTokens),
            enableGCDLowerGuards(enableGCDLowerGuards),
            workflow(workflow),
            workflowBound(workflowBound),
            calculateCmax(calculateCmax),
            replace(std::move(std::move(replace))),
            partialOrder(order),
            outputFile(outputFile),
            outputQuery(outputQuery),
            outputXMLQuery(outputXMLQuery),
            querynumbers(querynumbers) {
        };

    public: // inspectors

        std::string getInputFile() const {
            return inputFile;
        }

        void setInputFile(std::string input) {
            inputFile = std::move(input);
        }

        std::string getQueryFile() const {
            return queryFile;
        }

        void setQueryFile(std::string input) {
            queryFile = std::move(input);
        }

        std::string getOutputModelFile() const {
            return outputFile;
        }

        void setOutputModelFile(std::string input) {
            outputFile = std::move(input);
        }

        std::string getOutputQueryFile() const {
            return outputQuery;
        }

        std::set<size_t> getQueryNumbers() const {
            return querynumbers;
        }

        void setOutputQueryFile(std::string input) {
            outputQuery = std::move(input);
        }

        std::string getOutputXMLQueryFile() const {
            return outputXMLQuery;
        }

        void setOutputXMLQueryFile(std::string input) {
            outputXMLQuery = std::move(input);
        }

        inline unsigned int getKBound() const {
            return k_bound;
        }

        inline Trace getTrace() const {
            return trace;
        };

        inline bool getXmlTrace() const {
            return xml_trace;
        };

        inline bool getGlobalMaxConstantsEnabled() const {
            return useGlobalMaxConstants;
        }

        inline SearchType getSearchType() const {
            return searchType;
        }

        inline void setSearchType(SearchType type) {
            searchType = type;
        }

        inline VerificationType getVerificationType() const {
            return verificationType;
        }

        inline MemoryOptimization getMemoryOptimization() const {
            return memOptimization;
        }

        inline void setKeepDeadTokens(bool val) {
            keepDeadTokens = val;
        }

        inline bool getKeepDeadTokens() const {
            return keepDeadTokens;
        };

        inline bool getGCDLowerGuardsEnabled() const {
            return enableGCDLowerGuards;
        }

        inline WorkflowMode getWorkflowMode() const {
            return workflow;
        };

        inline long long getWorkflowBound() const {
            return workflowBound;
        };

        inline bool isWorkflow() const {
            return workflow != NOT_WORKFLOW;
        }

        inline bool getCalculateCmax() const {
            return calculateCmax;
        };

        inline const std::map<std::string, int> &getReplacements() const {
            return replace;
        }

        inline bool getPartialOrderReduction() const {
            return partialOrder;
        }


    private:
        std::string inputFile;
        std::string queryFile;
        SearchType searchType;
        VerificationType verificationType;
        MemoryOptimization memOptimization;
        unsigned int k_bound{};
        Trace trace;
        bool xml_trace{};
        bool useGlobalMaxConstants{};
        bool keepDeadTokens{};
        bool enableGCDLowerGuards{};
        WorkflowMode workflow;
        long long workflowBound{};
        bool calculateCmax{};
        std::map<std::string, int> replace;
        bool partialOrder{};
        std::string outputFile;
        std::string outputQuery;
        std::string outputXMLQuery;
        std::set<size_t> querynumbers;
    };

    std::ostream &operator<<(std::ostream &out, const VerificationOptions &options);
}

#endif /* VERIFICATIONOPTIONS_HPP_ */
