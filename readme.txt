zSlangInterpreter was built with MinGW 4.6.2 on Windows 7 (64 bit).
There is no makefile available, only a Code::Blocks Project File.

Making zSlang should not be (very) complicated, the following steps are required:
1.) Get the Boost Library and compile the libraries that are not header-only.
2.) Make an object file from every .cpp file that is provided.
3.) Link all object files and the "random" and "regex" libraries from boost.

Understanding the code of zSlang can be difficult at times because not only
do I make heavy use of several more or less confusing boost libraries,
I have also been using precompiler macros where it seemed appropriate.

Although some design decisions seem doubtful in retrospective and some
parts feel overly complicated, all in all I am happy with the quality of the
code.