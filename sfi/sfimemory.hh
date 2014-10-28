// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_MEMORY_H__
#define __SFI_MEMORY_H__

#include <sfi/sfitypes.hh>

G_BEGIN_DECLS


/* --- macros --- */
#define sfi_new_struct(type, n)		((type*) sfi_alloc_memblock (sizeof (type) * (n)))
#define sfi_new_struct0(type, n)	((type*) sfi_alloc_memblock0 (sizeof (type) * (n)))
#define sfi_delete_struct(type, mem)	(sfi_delete_structs (type, 1, (mem)))
#ifndef	__GNUC__
#  define sfi_delete_structs(type, n, mem)	(sfi_free_memblock (sizeof (type) * (n), (mem)))
#else					/* provide typesafety if possible */
#  define sfi_delete_structs(type, n, mem)	({ \
  type *__typed_pointer = (mem); \
  sfi_free_memblock (sizeof (type) * (n), __typed_pointer); \
})
#endif
#define	SFI_ALIGNED_SIZE(size,align)	((align) > 0 ? _SFI_INTERN_ALIGN (((gsize) (size)), ((gsize) (align))) : (gsize) (size))
#define	_SFI_INTERN_ALIGN(s, a)		(((s + (a - 1)) / a) * a)
#define	SFI_STD_ALIGN			(MAX (MAX (sizeof (float), sizeof (int)), sizeof (void*)))


/* --- implementation --- */
gpointer        sfi_alloc_memblock      (gsize           size);
gpointer        sfi_alloc_memblock0     (gsize           size);
void            sfi_free_memblock       (gsize           size,
					 gpointer        memblock);
void            sfi_alloc_report        (void);
gulong    	sfi_alloc_upper_power2  (const gulong    number);
void		_sfi_free_node_list	(gpointer	 mem,
					 gsize		 node_size);
G_END_DECLS
#endif /* __SFI_MEMORY_H__ */
/* vim:set ts=8 sts=2 sw=2: */
