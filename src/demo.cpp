#include <simplearg/arguments.h>
#include <iostream>
using namespace simplearg;

struct Test {
    unsigned u {};
    short i {};
    std::string s;
    bool foo(std::string_view name, Arguments& args) {
        args.errors(std::string{name} + ' ');
        if( !args.getall(u, s, i) ) return false;
        std::cout << "Got: " << name << ' ' << u << ' ' << s << ' ' << i << '\n';
        return true;
    }
    bool option(std::string_view name, Arguments& args) {
        args.errors(std::string{name} + ' ');
        if( !args.getall(s) ) return false;
        std::cout << "Got: " << name << s << '\n';
        return true;
    }
    bool bar(std::string_view name, Arguments& args) {
        args.errors(std::string{name} + ' ');
        if (! args.getall(u, s)) return false;
        std::cout << "Got: " << name << ' ' << u << ' ' << s << '\n';
        return true;
    }
    bool dash(std::string_view name, Arguments&) {
        std::cout << "Got: " << name << '\n';
        return true;
    }
    bool ddsh(std::string_view name, Arguments&) {
        std::cout << "Got: " << name << '\n';
        return true;
    }
    bool help(std::string_view name, Arguments&) {
        print(std::cout << "Usage:\n", params);
        return true;
    }
    static constexpr simplearg::Parameters<Test, 6> params = {{
        {&Test::option, "--option=", "a parameter with one option", "" },
        {&Test::foo, "foo", "a foo parameter", "f"},
        {&Test::bar, "bar", "a bar parameter", "b ba bbar"},
        {&Test::dash, "-", "a dash parameter", "" },
        {&Test::ddsh, "--", "a double dash parameter", "" },
        {&Test::help, "help", "prints this help", "--help -h -?" },
    }};
};


int main(int argc, char* argv[]) {
    Arguments args{argc-1, argv+1};
    Test test {};
    if (!args.parse(test, Test::params)) {
        std::cerr << args.errors() << '\n';
        return 1;
    }
    std::cout << argv[1] << '\n';

    if(args.contains("--help")) {
        std::cout << "Usage: ...\n";
    }
    std::string str {};
    unsigned val {};
    if(args.getall(val, str)) {
      std::cout << "Positional parameters:" << val << ',' << str << '\n';
    }

    return 0;
}
