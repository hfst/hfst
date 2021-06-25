#include <cstring>
#include <cstdio>
#include <vector>

#define no_argument 0
#define required_argument 1
#define optional_argument 2

struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

char * optarg = 0;
int optopt = 0;
int optind = 1;

std::vector<char *> free_arguments;
std::vector<char *> other_arguments;

int getopt_long(int argc, char * /*const*/ argv [], const char * optstring,
                 const struct option * longopts, int * longindex)
{
  // check that there are more args
  if (optind > (argc-1))
    {
      unsigned int i=1;
      for (auto other_argument : other_arguments)
        {
          argv[i] = other_argument;
          i++;
        }
      optind = (int)i;
      for (auto free_argument : free_arguments)
        {
          argv[i] = free_argument;
          i++;
        }
      return -1;
    }

  // skip free arguments
  while(*(argv[optind]) != '-')
    {
      free_arguments.push_back(argv[optind]);
      optind++;
      if (optind > (argc-1))
        {
          unsigned int i=1;
          for (auto other_argument : other_arguments)
            {
              argv[i] = other_argument;
              i++;
            }
          optind = (int)i;
          for (auto free_argument : free_arguments)
            {
              argv[i] = free_argument;
              i++;
            }
          return -1;
        }
    }

  other_arguments.push_back(argv[optind]);

  // strdup because we are possibly modifying the argument
  char * arg = strdup(argv[optind]); // free() should be called at the end...

  // skip initial '-' signs
  while(*arg == '-')
    arg++;

  // empty arg string
  if (*arg == '\0')
    {
      optopt = -2;
      return '?';
    }

  // whether arg is used in its short form: -f(=bar)
  bool short_option = false;
  arg++;
  if (*arg == '\0' || *arg == '=')
    short_option = true;
  arg--;

  // whether option argument is given after an '=' sign (--foo=bar, -f=bar)
  bool eq_used = false;
  char * argptr = arg;  // this will point to the char after the '=' sign if eq_used is true
  while (*argptr != '\0')
    {
      if (*argptr == '=')
        {
          *argptr = '\0';  // change '=' into '\0' to make string comparison easier
          argptr++;
          eq_used = true;
          break;
        }
      argptr++;
    }

  // Go through all possible option strings
  while(longopts->name != 0)
    {
      // match found, short or long format
      if (strcmp(longopts->name, arg) == 0 || (short_option && longopts->val == (int)*arg))
        {
          optind++;
          // no argument
          if (longopts->has_arg == no_argument)
            {
              // argument given for an option that does not take one
              if (eq_used)
                {
                  fprintf(stderr, "warning: argument ignored for option '--%s'\n", longopts->name);
                }
              return longopts->val;
            }
          // required argument
          else if (longopts->has_arg == required_argument || longopts->has_arg == optional_argument)
            {
              if (eq_used)
                {
                  // we already have a pointer to the argument
                  optarg = strdup(argptr);
                  argptr--;
                  *argptr = '='; // change '\0' back to '=' (not sure if this is needed...)
                  return longopts->val;
                }
              // no more args, argument thus missing
              if (optind > (argc-1))
                {
                  if (longopts->has_arg == required_argument)
                    {
                      optopt = longopts->val;
                      return ':';
                    }
                  else
                    {
                      optopt = 0;
                      return longopts->val;
                    }
                }
              // next arg is required argument (cannot be optional argument)
              if (longopts->has_arg == required_argument)
                {
                  optarg = strdup(argv[optind]);
                  other_arguments.push_back(argv[optind]);
                  optind++;
                  return longopts->val;
                }
              else
                {
                  optopt = 0;
                  return longopts->val;
                }
            }
          // this should not happen
          else
            {
              return 0;
            }
        }
      longopts++;
    }

  // no match found
  optind++;
  optopt = -2;
  if (short_option)
    optopt = (int)*arg;
  return '?';
}

