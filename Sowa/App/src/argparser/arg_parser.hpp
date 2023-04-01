#ifndef SW_ARG_PARSER_HPP_
#define SW_ARG_PARSER_HPP_

#include <vector>
#include <string>

struct ArgOptionValue {
    std::string value{""};

    ArgOptionValue();
    ArgOptionValue(std::string);
    ArgOptionValue(int);
    ArgOptionValue(double);
    ArgOptionValue(bool);

    template<typename T>
    T As();
};
struct ArgOption {
    ArgOption();
    ArgOption(std::string key, ArgOptionValue value);

    std::string key;
    ArgOptionValue value;
};

// ./app parameter --flag --flag parameter -f -xy --option=something
class ArgParser {
    public:
        //* ArgParser(argc - 1, argv + 1);
        ArgParser(int argc, char const** argv);

        inline const std::vector<std::string>& GetFlags() { return m_Flags; }
        inline const std::vector<std::string>& GetParameters() { return m_Params; }
        inline const std::vector<ArgOption>& GetOptions() { return m_Options; }

        bool HasFlag(std::string flag);
        ArgOptionValue GetOption(std::string key, ArgOptionValue fallback = ArgOptionValue());
    private:
        std::vector<std::string> m_Flags{};
        std::vector<std::string> m_Params{};
        std::vector<ArgOption> m_Options{};
};

#endif // SW_COMMAND_PARSER_HPP_