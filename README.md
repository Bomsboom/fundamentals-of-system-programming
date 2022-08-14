# fundamentals-of-system-programming
## Practical work "File I/O and dynamic libraries" (C/Linux OS)

# Task description
 Develop in C for Linux OS:
 * a program that allows you to perform a recursive search for files, starting with specified directory, using dynamic (shared) plug-in libraries
 * a dynamic library that implements optoin:
   + Option: ` --ipv4-addr <value> `
   - Search for files containing the specified IPv4 address value either in text form (x.x.x.x) or in binary form (little-endian or big-endian)
   + ` * Example: --ipv4-addr 192.168.8.1 * `

The program should be a console utility,
which is configured by passing arguments in the startup line and/or using
environment variables : ` lab1epuN3247 [options [directory]] `

  The program must perform a recursive search for files that meet the criteria
that are set by options on the command line. The available search criteria (and,
accordingly, the available options) are determined by the presence in a given directory
of dynamic libraries that extend the functionality of the program (hereinafter — plugins).
When running without a directory name to search for, the program displays reference
information on options and plugins available at the time of launch.
Options supported by the program:
  * ` -P dir ` -- ` directory with plugins `
  * ` -A ` -- ` Combining plugin options using the "And" operation
(works by default). `
  * ` -O ` -- ` Combining plugin options using the "OR" operation. `
  * ` -N ` -- ` Inverting the search term (after combining plugin options with -A or -O). `
  * ` -v ` -- ` Output of the program version and information about the program (
performer's full name, group number, laboratory variant number). `
  * ` -h ` -- ` Output of help on options. `

  All options supported
by plugins and their description are displayed. By default, plugins are searched in the same directory where
the executable file of the program is located, and if the -P option is set, then in the directory specified
in this option. In the case of running a program with several options specifying
search criteria, these criteria are combined by the logical operation "AND" (the same if
option -A is set) or the logical operation "OR" (if option -O is set). If the -N option is set,
then after combining all the search conditions by "AND" or "OR", it changes to the
opposite.
Plugins are dynamic libraries in ELF format with
an arbitrary name and extension .so and interface functions.
