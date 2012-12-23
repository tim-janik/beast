// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SUPER_H__
#define __BSE_SUPER_H__

#include        <bse/bsecontainer.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define	BSE_TYPE_SUPER		    (BSE_TYPE_ID (BseSuper))
#define BSE_SUPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUPER, BseSuper))
#define BSE_SUPER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUPER, BseSuperClass))
#define BSE_IS_SUPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUPER))
#define BSE_IS_SUPER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUPER))
#define BSE_SUPER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUPER, BseSuperClass))


/* --- BseSuper member macros --- */
#define BSE_SUPER_NEEDS_CONTEXT(object)		  ((BSE_OBJECT_FLAGS (object) & BSE_SUPER_FLAG_NEEDS_CONTEXT) != 0)


/* --- bse super flags --- */
typedef enum                            /*< skip >*/
{
  BSE_SUPER_FLAG_NEEDS_CONTEXT		 = 1 << (BSE_CONTAINER_FLAGS_USHIFT + 0),
} BseSuperFlags;
#define BSE_SUPER_FLAGS_USHIFT	       (BSE_CONTAINER_FLAGS_USHIFT + 1)


/* --- BseSuper object --- */
struct _BseSuper
{
  BseContainer	 parent_object;

  SfiTime	 creation_time;
  SfiTime	 mod_time;

  /* for BseProject */
  guint          context_handle;
};
struct _BseSuperClass
{
  BseContainerClass parent_class;
  
  void		(*modified)		(BseSuper	*super,
					 SfiTime	 stamp);
  void          (*compat_finish)        (BseSuper       *super,
                                         guint           vmajor,
                                         guint           vminor,
                                         guint           vmicro);
};


G_END_DECLS

#endif /* __BSE_SUPER_H__ */
