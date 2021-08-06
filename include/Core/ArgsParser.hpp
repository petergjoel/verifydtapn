#ifndef ARGSPARSER_HPP_
#define ARGSPARSER_HPP_

#include "VerificationOptions.hpp"
#include "boost/lexical_cast.hpp"

#include <utility>
#include <vector>
#include <map>
#include <string>
#include <iosfwd>

namespace VerifyTAPN {
    const int WIDTH = 40; // TODO: determine this based on registered switches

    struct Version {
    public:
        Version(unsigned int maj, unsigned int min, unsigned int build) : maj(maj), min(min), build(build) {};
        unsigned int maj;
        unsigned int min;
        unsigned int build;
        // TODO: maybe add revision?
    };

    std::ostream &operator<<(std::ostream &out, const Version &version);


    typedef std::map<std::string, std::string> option_map;
    typedef option_map::value_type option;

    class Switch {
    public:
        Switch(std::string name, std::string long_name, std::string description) : name(std::move(name)),
                                                                                   long_name(std::move(
                                                                                           long_name)),
                                                                                   description(std::move(
                                                                                           description)),
                                                                                   handled_option(
                                                                                           false) {};

        virtual ~Switch() = default;
    public:
        inline const std::string &getShortName() const { return name; };

        inline const std::string &getLongName() const { return long_name; };

        inline const std::string &getDescription() const { return description; };

        virtual bool handles(const std::string &flag) const;

        virtual option parse(const std::string &flag);

        virtual void print(std::ostream &out) const;

        inline bool handledOption() const { return handled_option; };

        virtual option getDefaultOption() const { return option(long_name, "0"); };
    private:
        std::string name;
        std::string long_name;
        std::string description;
    protected:
        bool handled_option;
    };

    std::ostream &operator<<(std::ostream &out, const Switch &flag);

    class SwitchWithStringArg : public Switch {
    public:
        SwitchWithStringArg(const std::string &name, const std::string &long_name, const std::string &description,
                            std::string default_value) : Switch(name, long_name, description),
                                                         default_value(std::move(default_value)) {};

        ~SwitchWithStringArg() override = default;

        option parse(const std::string &flag) override;

        void print(std::ostream &out) const override;

        option getDefaultOption() const override { return option(getLongName(), default_value); };
    private:
        std::string default_value;
    };

    class SwitchWithArg : public Switch {
    public:
        SwitchWithArg(const std::string &name, const std::string &long_name, const std::string &description,
                      unsigned int default_value) : Switch(name, long_name, description),
                                                    default_value(default_value) {};

        ~SwitchWithArg() override = default;

        option parse(const std::string &flag) override;

        void print(std::ostream &out) const override;

        option getDefaultOption() const override {
            return option(getLongName(), std::to_string(default_value));
        };
    private:
        unsigned int default_value;
    };

    class ArgsParser {
        typedef std::vector<Switch *> parser_vec;
    public:
        ArgsParser() : parsers(), version(3, 4, 1) { initialize(); };

        virtual ~ArgsParser() { for (auto p : parsers) delete p; };

        VerificationOptions parse(int argc, char *argv[]) const;

    private:
        VerificationOptions createVerificationOptions(const option_map &map) const;

        VerificationOptions verifyInputFiles(VerificationOptions, std::string model_file, std::string query_file) const;

        unsigned int tryParseInt(const option &option) const;

        unsigned long long tryParseLongLong(const option &option) const;

        std::vector<std::string> parseIncPlaces(const std::string &string) const;

        std::map<std::string, int> parseReplace(const option &option) const;

        std::set<size_t> extractValues(const option &option) const;

        void initialize();

        void printHelp() const;

        void printVersion() const;

    private: // data
        parser_vec parsers;
        VerifyTAPN::Version version;
    };
}

#endif /* ARGSPARSER_HPP_ */
