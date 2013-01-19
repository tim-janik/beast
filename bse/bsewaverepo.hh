// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_WAVE_REPO_H__
#define	__BSE_WAVE_REPO_H__
#include	<bse/bsesuper.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_WAVE_REPO	        (BSE_TYPE_ID (BseWaveRepo))
#define BSE_WAVE_REPO(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_REPO, BseWaveRepo))
#define BSE_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))
#define BSE_IS_WAVE_REPO(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_REPO))
#define BSE_IS_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_REPO))
#define BSE_WAVE_REPO_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))
/* --- BseWaveRepo object --- */
struct _BseWaveRepo
{
  BseSuper	 parent_object;
  GList		*waves;
};
struct _BseWaveRepoClass
{
  BseSuperClass parent_class;
};
/* --- prototypes --- */
G_END_DECLS
#endif /* __BSE_WAVE_REPO_H__ */
