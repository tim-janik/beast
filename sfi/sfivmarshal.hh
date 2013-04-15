// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_VMARSHAL_H__
#define __SFI_VMARSHAL_H__

#include <sfi/sfitypes.hh>

G_BEGIN_DECLS

/* --- hard limit --- */
#define	SFI_VMARSHAL_MAX_ARGS	5


/* --- invocations --- */
void	sfi_vmarshal_void	(void         *func,
				 void         *arg0,
				 uint	       n_args,
				 const GValue *args,  /* 1..n */
				 void         *data); /* n+1 */


/* --- internal --- */
#if GLIB_SIZEOF_VOID_P == 4
#define SFI_VMARSHAL_PTR_ID  1
#else
#define SFI_VMARSHAL_PTR_ID  2
#endif

G_END_DECLS

#endif /* __SFI_VMARSHAL_H__ */

/* vim:set ts=8 sts=2 sw=2: */
