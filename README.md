# SimpleArg


## Brief

SimpleArg is a header-only library for dispatching argc, argv to a user's class. 
It does not impose any constraints on how the arguments may look like - with a double dash, a single dash, 
or no dash at all. The only assumption it makes is about `=` character, which is treated as an option/value delimiter, if such option syntax is used in the application.

## Quick Start Guide

### Immediate Use

SimpleArg can be used without a perdefined set of parameters

```
#include <simplearg/arguments.h>
int main(int argc, char* argv[]) {
    using namespace simplearg;
    Arguments args{argc-1, argv+1};
    if(args.contains("--help")) {
        std::cout << "Usage: ...\n";
        return 0;
    }
    
    std::string str {};
    unsigned val {};
    if(args.getall(val, str)) {
      std::cout << "Positional parameters:" << val << ',' << str << '\n';
    } else {
      std::cerr << args.errors() << '\n';
      return 1;
    }
    // ...
    return 0;
}
```

### Dispatched Use

This mode requires some preparations steps:

#### 1. Define a class that would be used as the dispatcher:

```
#include <simplearg/arguments.h>
using namespace simplearg;
struct OptionDispatcher {
    std::string myopt {};
    bool myoption(std::string_view name, Arguments& args) {
        return args.get(myopt);
    }
    bool mycommand(std::string_view name, Arguments& args) {
        int num {};
        std::string str {};
        if (! args.getall(num, str))
            return false;
        // using num, str ...
        return true;
    }
    bool help(std::string_view name, Arguments& args) {
        std::cout << "Usage: ...\n";
        return true;        
    }
    
};
```

#### 2. Define option names

```
static constexpr simplearg::Parameters<OptionDispatcher, 3> myparams = {{
    { &OptionDispatcher::myoption,            // Method to be invoked
      "--myoption=",                          // Option name
      "a named option with a value",          // Description
      "-o= --o=" },                           // Space delimited aliases
    { &OptionDispatcher::mycommand,           // Method to be invoked
      "mycommand",                            // Verb name
      "a positional command with positional parameters", 
      "c"},
    { &OptionDispatcher::help, "help", "prints this help", "--help -h -?" },
}};
```

#### 3. Parse arguments:

```
int main(int argc, char* argv[]) {
    Arguments args{argc-1, argv+1};
    OptionDispatcher od {};
    if (!args.parse(od, myparams)) {
        std::cerr << args.errors() << '\n';
        return 1;
    }
    //...
    return 0;
}
```

#### 4. Printing Help

SimpleArg facilitates a print function that prints parameters with their descriptions:

```
print(std::cout << "Usage:\n", myparams);
```
