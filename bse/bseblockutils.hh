/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_BLOCK_UTILS_H__
#define __BSE_BLOCK_UTILS_H__

#include <wchar.h> /* wmemset */
#include <bse/bseieee754.h>

G_BEGIN_DECLS

/* --- C API --- */
const
char*   bse_block_impl_name                       (void);
static inline
void    bse_block_fill_uint32                     (guint          n_values,       /* 4-byte variant of memset for ints */
                                                   guint32       *values,
                                                   guint32        vuint32);
static inline
void    bse_block_fill_float                      (guint	  n_values,       /* 4-byte variant of memset for floats */
                                                   float         *values,
                                                   const float    value);
static inline
void    bse_block_copy_uint32                     (guint	  n_values,       /* 4-byte variant of memcpy for ints */
                                                   guint32       *values,
                                                   const guint32 *ivalues);
static inline
void    bse_block_copy_float                      (guint	  n_values,       /* 4-byte variant of memcpy for floats */
                                                   gfloat        *values,
                                                   const gfloat  *ivalues);
void    bse_block_add_floats                      (guint	  n_values,
                                                   float         *ovalues,
                                                   const float   *ivalues);
void    bse_block_sub_floats                      (guint	  n_values,
                                                   float         *ovalues,
                                                   const float   *ivalues);
void    bse_block_mul_floats                      (guint	  n_values,
                                                   float         *ovalues,
                                                   const float   *ivalues);
void    bse_block_scale_floats                    (guint          n_values,
                                                   float         *ovalues,
                                                   const float   *ivalues,
                                                   const float    level);
void    bse_block_interleave2_floats              (guint	  n_ivalues,
                                                   float         *ovalues,        /* length_ovalues = n_ivalues * 2 */
                                                   const float   *ivalues,
                                                   guint          offset);        /* 0=left, 1=right */
void    bse_block_interleave2_add_floats          (guint	  n_ivalues,
                                                   float         *ovalues,        /* length_ovalues = n_ivalues * 2 */
                                                   const float   *ivalues,
                                                   guint          offset);        /* 0=left, 1=right */
void    bse_block_calc_float_range                (guint          n_values,
                                                   const float   *ivalues,
	                                           float         *min_value,
	                                           float         *max_value);
float   bse_block_calc_float_square_sum           (guint          n_values,
                                                   const float   *ivalues);
float   bse_block_calc_float_range_and_square_sum (guint          n_values,
                                                   const float   *ivalues,
	                                           float         *min_value,
	                                           float         *max_value);

G_END_DECLS

#ifdef  __cplusplus
#include <bse/bseresampler.hh>
namespace Bse {

/* --- C++ API --- */
class Block {
public:
  static const char*    impl_name            ()                              { return singleton->impl_name (); }
  static inline   void	fill                 (guint           n_values,
					      float          *values,
					      float           value);
  static inline   void	fill                 (guint           n_values,
					      guint32        *values,
					      guint32         value);
  static inline   void	copy                 (guint           n_values,
					      float          *values,
					      const float    *ivalues);
  static inline   void	copy                 (guint           n_values,
					      guint32        *values,
					      const guint32  *ivalues);
  static inline   void	add                  (guint	      n_values,
					      float          *ovalues,
					      const float    *ivalues)       { singleton->add (n_values, ovalues, ivalues); }
  static inline   void	sub                  (guint	      n_values,
					      float          *ovalues,
					      const float    *ivalues)       { singleton->sub (n_values, ovalues, ivalues); }
  static inline   void	mul                  (guint	      n_values,
					      float          *ovalues,
					      const float    *ivalues)       { singleton->mul (n_values, ovalues, ivalues); }
  static inline   void	scale                (guint	      n_values,
					      float          *ovalues,
					      const float    *ivalues,
					      const float     level)         { singleton->scale (n_values, ovalues, ivalues, level); }
  static inline   void	interleave2          (guint	      n_ivalues,
					      float          *ovalues,
					      const float    *ivalues,
					      guint           offset)        { singleton->interleave2 (n_ivalues, ovalues, ivalues, offset); }
  static inline   void	interleave2_add      (guint	      n_ivalues,
					      float          *ovalues,
					      const float    *ivalues,
					      guint           offset)        { singleton->interleave2_add (n_ivalues, ovalues, ivalues, offset); }
  static inline   void	range    	     (guint	      n_values,
					      const float    *ivalues,
					      float&          min_value,
					      float&          max_value)     { singleton->range (n_values, ivalues, min_value, max_value); }
  static inline   float	square_sum           (guint	      n_values,
					      const float    *ivalues)       { return singleton->square_sum (n_values, ivalues); }
  static inline   float	range_and_square_sum (guint	      n_values,
                                              const float    *ivalues,
					      float&          min_value,
					      float&          max_value)     { return singleton->range_and_square_sum (n_values, ivalues, min_value, max_value); }

  typedef Resampler::Resampler2 Resampler2;
  static inline
  Resampler2*           create_resampler2    (BseResampler2Mode      mode,
                                              BseResampler2Precision precision)	  { return singleton->create_resampler2 (mode, precision); }
  static inline
  bool                  test_resampler2	     (bool                   verbose)	  { return singleton->test_resampler2 (verbose); }


    
  class Impl {
  protected:
    virtual      ~Impl                  ();
    virtual const char* impl_name       () = 0;
    virtual void  add                   (guint	         n_values,
                                         float          *ovalues,
                                         const float    *ivalues) = 0;
    virtual void  sub                   (guint	         n_values,
                                         float          *ovalues,
                                         const float    *ivalues) = 0;
    virtual void  mul                   (guint	         n_values,
                                         float          *ovalues,
                                         const float    *ivalues) = 0;
    virtual void  scale                 (guint           n_values,
                                         float          *ovalues,
                                         const float    *ivalues,
                                         const float     level) = 0;
    virtual void  interleave2           (guint	         n_ivalues,
                                         float          *ovalues,	/* length_ovalues = n_ivalues * 2 */
                                         const float    *ivalues,
                                         guint           offset) = 0;   /* 0=left, 1=right */
    virtual void  interleave2_add       (guint	         n_ivalues,
                                         float          *ovalues,	/* length_ovalues = n_ivalues * 2 */
                                         const float    *ivalues,
                                         guint           offset) = 0;   /* 0=left, 1=right */
    virtual void  range                 (guint	         n_values,
                                         const float    *ivalues,
					 float&          min_value,
					 float&          max_value) = 0;
    virtual float square_sum            (guint	         n_values,
                                         const float    *ivalues) = 0;
    virtual float range_and_square_sum	(guint	         n_values,
                                         const float    *ivalues,
					 float&          min_value,
					 float&          max_value) = 0;
    virtual
    Resampler2*   create_resampler2     (BseResampler2Mode      mode,
                                         BseResampler2Precision precision) = 0;
    virtual bool  test_resampler2	(bool			verbose) = 0;
  friend class Block;
    static  void  substitute            (Impl           *substitute_impl);
  };
  static Impl*  default_singleton       ();
  static Impl*  current_singleton       ();
private:
  static Impl  *singleton;
};

/* --- C++ implementation bits --- */
inline void
Block::fill (guint           n_values,
             float          *values,
             float           value)
{
  BIRNET_STATIC_ASSERT (sizeof (float) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  const union { float f; guint32 vuint32; } u = { value };
  wmemset ((wchar_t*) values, u.vuint32, n_values);
}

inline void
Block::fill (guint           n_values,
             guint32        *values,
             guint32         value)
{
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) values, value, n_values);
}

inline void
Block::copy (guint          n_values,
             guint32       *values,
             const guint32 *ivalues)
{
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemcpy ((wchar_t*) values, (const wchar_t*) ivalues, n_values);
}

inline void
Block::copy (guint         n_values,
             gfloat       *values,
             const gfloat *ivalues)
{
  BIRNET_STATIC_ASSERT (sizeof (float) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemcpy ((wchar_t*) values, (const wchar_t*) ivalues, n_values);
}

} // Bse
#endif  /* __cplusplus */

/* --- C implementation bits --- */
G_BEGIN_DECLS

static inline void
bse_block_fill_uint32 (guint    n_values,
		       guint32 *values,
		       guint32  vuint32)
{
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) values, vuint32, n_values);
}

static inline void
bse_block_fill_float (guint	   n_values,
		      float       *values,
		      const float  value)
{
  BIRNET_STATIC_ASSERT (sizeof (float) == 4);
  const union { float f; guint32 vuint32; } u = { value };
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) values, u.vuint32, n_values);
}

static inline void
bse_block_copy_uint32 (guint	      n_values,
		       guint32       *values,
		       const guint32 *ivalues)
{
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemcpy ((wchar_t*) values, (const wchar_t*) ivalues, n_values);
}

static inline void
bse_block_copy_float (guint	    n_values,
		      gfloat       *values,
		      const gfloat *ivalues)
{
  BIRNET_STATIC_ASSERT (sizeof (float) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemcpy ((wchar_t*) values, (const wchar_t*) ivalues, n_values);
}

G_END_DECLS

#endif /* __BSE_BLOCK_UTILS_H__ */
