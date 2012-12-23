// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_SNET_H__
#define	__BSE_SNET_H__

#include	<bse/bsesuper.hh>
#include	<bse/bseglobals.hh> /* FIXME */


G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_SNET		   (BSE_TYPE_ID (BseSNet))
#define BSE_SNET(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SNET, BseSNet))
#define BSE_SNET_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SNET, BseSNetClass))
#define BSE_IS_SNET(object)	   (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SNET))
#define BSE_IS_SNET_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SNET))
#define BSE_SNET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SNET, BseSNetClass))
#define BSE_SNET_USER_SYNTH(src)   ((BSE_OBJECT_FLAGS (src) & BSE_SNET_FLAG_USER_SYNTH) != 0)


/* --- BseSNet flags --- */
typedef enum	/*< skip >*/
{
  BSE_SNET_FLAG_USER_SYNTH	= 1 << (BSE_SUPER_FLAGS_USHIFT + 0)
} BseSNetFlags;
#define BSE_SNET_FLAGS_USHIFT	(BSE_SUPER_FLAGS_USHIFT + 1)


/* --- BseSNet object --- */
typedef struct
{
  gchar     *name;
  guint      context : 31;
  guint	     input : 1;
  BseModule *src_omodule;	/* output */
  guint	     src_ostream;
  BseModule *dest_imodule;	/* input */
  guint	     dest_istream;
} BseSNetPort;
struct _BseSNet
{
  BseSuper	 parent_object;

  SfiRing	*sources;	/* of type BseSource* */
  SfiRing	*isources;	/* internal (protected) sources */
  GSList	*iport_names;
  GSList	*oport_names;
  GBSearchArray *port_array;	/* of type BseSNetPort* */

  GSList	*tmp_context_children;

  guint		 port_unregistered_id;
};
struct _BseSNetClass
{
  BseSuperClass parent_class;
};
struct _BseMidiContext {
  BseMidiReceiver *midi_receiver;
  guint            midi_channel;
  guint            voice_id;
};


/* --- prototypes --- */
guint            bse_snet_create_context        (BseSNet         *snet,
                                                 BseMidiContext   mcontext,
                                                 BseTrans        *trans);
guint            bse_snet_context_clone_branch  (BseSNet         *self,
                                                 guint            context,
                                                 BseSource       *context_merger,
                                                 BseMidiContext   mcontext,
                                                 BseTrans        *trans);
gboolean         bse_snet_context_is_branch     (BseSNet         *self,
                                                 guint            context_id);
void             bse_snet_intern_child          (BseSNet         *self,
                                                 gpointer         child);
BseMidiContext   bse_snet_get_midi_context      (BseSNet         *snet,
                                                 guint            context_handle);
const gchar*     bse_snet_iport_name_register   (BseSNet         *snet,
                                                 const gchar     *tmpl_name);
gboolean         bse_snet_iport_name_registered (BseSNet         *snet,
                                                 const gchar     *name);
void             bse_snet_iport_name_unregister (BseSNet         *snet,
                                                 const gchar     *name);
const gchar*     bse_snet_oport_name_register   (BseSNet         *snet,
                                                 const gchar     *tmpl_name);
gboolean         bse_snet_oport_name_registered (BseSNet         *snet,
                                                 const gchar     *name);
void             bse_snet_oport_name_unregister (BseSNet         *snet,
                                                 const gchar     *name);
void             bse_snet_set_iport_src         (BseSNet         *snet,
                                                 const gchar     *port_name,
                                                 guint            snet_context,
                                                 BseModule       *omodule,
                                                 guint            ostream,
                                                 BseTrans        *trans);
void             bse_snet_set_iport_dest        (BseSNet         *snet,
                                                 const gchar     *port_name,
                                                 guint            snet_context,
                                                 BseModule       *imodule,
                                                 guint            istream,
                                                 BseTrans        *trans);
void             bse_snet_set_oport_src         (BseSNet         *snet,
                                                 const gchar     *port_name,
                                                 guint            snet_context,
                                                 BseModule       *omodule,
                                                 guint            ostream,
                                                 BseTrans        *trans);
void             bse_snet_set_oport_dest        (BseSNet         *snet,
                                                 const gchar     *port_name,
                                                 guint            snet_context,
                                                 BseModule       *imodule,
                                                 guint            istream,
                                                 BseTrans        *trans);


G_END_DECLS

#endif /* __BSE_SNET_H__ */
