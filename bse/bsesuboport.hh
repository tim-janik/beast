// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SUB_OPORT_H__
#define __BSE_SUB_OPORT_H__
#include <bse/bsesource.hh>
#include <bse/bseengine.hh>
G_BEGIN_DECLS

#define BSE_TYPE_SUB_OPORT		(BSE_TYPE_ID (BseSubOPort))
#define BSE_SUB_OPORT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_OPORT, BseSubOPort))
#define BSE_SUB_OPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_OPORT, BseSubOPortClass))
#define BSE_IS_OPORT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_OPORT))
#define BSE_IS_OPORT_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_OPORT))
#define BSE_SUB_OPORT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_OPORT, BseSubOPortClass))
#define	BSE_SUB_OPORT_N_PORTS	(4)

struct BseSubOPort : BseSource {
  gchar	       **output_ports;
};
struct BseSubOPortClass : BseSourceClass
{};

G_END_DECLS

#endif /* __BSE_SUB_OPORT_H__ */
