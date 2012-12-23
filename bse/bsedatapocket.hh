// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_DATA_POCKET_H__
#define __BSE_DATA_POCKET_H__

#include        <bse/bsesuper.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_DATA_POCKET              (BSE_TYPE_ID (BseDataPocket))
#define BSE_DATA_POCKET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_DATA_POCKET, BseDataPocket))
#define BSE_DATA_POCKET_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_DATA_POCKET, BseDataPocketClass))
#define BSE_IS_DATA_POCKET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_DATA_POCKET))
#define BSE_IS_DATA_POCKET_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_DATA_POCKET))
#define BSE_DATA_POCKET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_DATA_POCKET, BseDataPocketClass))


/* --- data types --- */
typedef enum	/*< skip >*/
{
  BSE_DATA_POCKET_INT		= 'i',
  BSE_DATA_POCKET_INT64		= 'q',
  BSE_DATA_POCKET_FLOAT		= 'f',
  /* BSE_DATA_POCKET_DOUBLE	= 'd', */
  BSE_DATA_POCKET_STRING	= 's',
  BSE_DATA_POCKET_OBJECT	= 'o'
} BseDataPocketType;


/* --- BseDataPocket structs --- */
typedef struct _BseDataPocket		BseDataPocket;
typedef struct _BseDataPocketClass	BseDataPocketClass;
typedef union {
  guint64  v_int64;
  guint    v_int;
  gfloat   v_float;
  gchar   *v_string;
  BseItem *v_object;
} BseDataPocketValue;
typedef struct
{
  guint	               id;
  guint	               n_items;
  struct Item {
    GQuark             quark;
    gchar              type;
    BseDataPocketValue value;
  }	              *items;
} BseDataPocketEntry;
struct _BseDataPocket
{
  BseItem	      parent_object;

  guint		      need_store;	/* for BSE_ITEM_FLAG_AGGREGATE */
  GSList	     *cr_items;
  
  guint		      free_id;

  guint		      n_entries;
  BseDataPocketEntry *entries;
};
struct _BseDataPocketClass
{
  BseItemClass	parent_class;

  void	(*entry_added)	 (BseDataPocket	*pocket,
			  guint		 entry_id);
  void	(*entry_removed) (BseDataPocket	*pocket,
			  guint		 entry_id);
  void	(*entry_changed) (BseDataPocket	*pocket,
			  guint		 entry_id);
};


/* --- prototypes --- */
guint	 _bse_data_pocket_create_entry	(BseDataPocket	   *pocket);
gboolean _bse_data_pocket_delete_entry	(BseDataPocket	   *pocket,
					 guint		    entry_id);
gboolean _bse_data_pocket_entry_set	(BseDataPocket	   *pocket,
					 guint		    id,
					 GQuark		    data_quark,
					 gchar		    type,
					 BseDataPocketValue value);
gchar	 _bse_data_pocket_entry_get	(BseDataPocket	    *pocket,
					 guint		     id,
					 GQuark		     data_quark,
					 BseDataPocketValue *value);

G_END_DECLS

#endif /* __BSE_DATA_POCKET_H__ */
