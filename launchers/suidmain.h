/* suidmain - suid wrapper to acquire capabilities and drop root suid
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
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
