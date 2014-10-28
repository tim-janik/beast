// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SUB_IPORT_H__
#define __BSE_SUB_IPORT_H__
#include <bse/bsesource.hh>
#include <bse/bseengine.hh>
G_BEGIN_DECLS

#define BSE_TYPE_SUB_IPORT		(BSE_TYPE_ID (BseSubIPort))
#define BSE_SUB_IPORT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_IPORT, BseSubIPort))
#define BSE_SUB_IPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_IPORT, BseSubIPortClass))
#define BSE_IS_IPORT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_IPORT))
#define BSE_IS_IPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_IPORT))
#define BSE_SUB_IPORT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_IPORT, BseSubIPortClass))
#define BSE_SUB_IPORT_N_PORTS           (4)

struct BseSubIPort : BseSource {
  gchar	       **input_ports;
};
struct BseSubIPortClass : BseSourceClass
{};

G_END_DECLS

#endif /* __BSE_SUB_IPORT_H__ */
