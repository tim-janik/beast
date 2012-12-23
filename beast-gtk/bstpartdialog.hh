// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PART_DIALOG_H__
#define __BST_PART_DIALOG_H__

#include	"bstpianoroll.hh"
#include	"bsteventroll.hh"
#include	"bstpatternview.hh"
#include	"bstpatternctrl.hh"
#include	"bstpianorollctrl.hh"
#include	"bsteventrollctrl.hh"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define BST_TYPE_PART_DIALOG              (bst_part_dialog_get_type ())
#define BST_PART_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PART_DIALOG, BstPartDialog))
#define BST_PART_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PART_DIALOG, BstPartDialogClass))
#define BST_IS_PART_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PART_DIALOG))
#define BST_IS_PART_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PART_DIALOG))
#define BST_PART_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PART_DIALOG, BstPartDialogClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPartDialog		BstPartDialog;
typedef	struct	_BstPartDialogClass	BstPartDialogClass;
struct _BstPartDialog
{
  GxkDialog	          parent_object;

  BstPianoRoll           *proll;
  BstPianoRollController *pctrl;
  BstEventRoll           *eroll;
  BstEventRollController *ectrl;
  BstPatternView         *pview;
  BstPatternController   *pvctrl;
  SfiProxy                project;
};
struct _BstPartDialogClass
{
  GxkDialogClass parent_class;
};


/* --- prototypes --- */
GType		bst_part_dialog_get_type	(void);
void		bst_part_dialog_set_proxy	(BstPartDialog	*self,
						 SfiProxy	 part);

G_END_DECLS

#endif /* __BST_PART_DIALOG_H__ */
