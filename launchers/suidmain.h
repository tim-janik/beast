// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#ifndef __SUIDMAIN_H__
#define __SUIDMAIN_H__
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* custom program name finder */
const char* custom_find_executable   (int        *argc,
                                      char     ***argv);
/* check for "--" and similar arguments */
int         custom_check_arg_stopper (const char *argument);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif  /* __SUIDMAIN_H__ */
