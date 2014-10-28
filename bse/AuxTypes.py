# Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
"""AuxTypes.py - BSE Auxillary Type Initializers

More details at http://beast.testbit.org/
"""
import Decls # Aida.Decls

auxillary_initializers = {
  # Domain specific types
  (Decls.BOOL,      'Trigger')  : ('nick', 'blurb', 'hints'),
  (Decls.INT32,     'Note')     : ('nick', 'blurb', 'default', 'hints'),
  (Decls.INT32,     'Octave')   : ('nick', 'blurb', 'default', 'hints'),
  (Decls.INT32,     'FineTune') : ('nick', 'blurb', 'hints'),
  (Decls.FLOAT64,   'Freq')     : ('nick', 'blurb', 'default', 'hints'),
  (Decls.FLOAT64,   'Balance')  : ('nick', 'blurb', 'default', 'hints'),
  (Decls.FLOAT64,   'Perc')     : ('nick', 'blurb', 'default', 'hints'),
  (Decls.FLOAT64,   'Gain')     : ('nick', 'blurb', 'default', 'min', 'max', 'step', 'hints'),
  (Decls.FLOAT64,   'DBVolume') : ('nick', 'blurb', 'defaultdb', 'mindb', 'maxdb', 'hints'),
  # Deprecated compatibility definitions
  (Decls.BOOL,      'SfiBool')  : ('nick', 'blurb', 'default', 'hints'),
  (Decls.INT32,     'SfiInt')   : ('nick', 'blurb', 'default', 'min', 'max', 'step', 'hints'),
  (Decls.INT64,     'SfiNum')   : ('nick', 'blurb', 'default', 'min', 'max', 'step', 'hints'),
  (Decls.INT32,     'SfiUInt')  : ('nick', 'blurb', 'default', 'hints'),
  (Decls.FLOAT64,   'SfiReal')  : ('nick', 'blurb', 'default', 'min', 'max', 'step', 'hints'),
  (Decls.ENUM,      'SfiEnum')  : ('nick', 'blurb', 'default', 'hints'),
  (Decls.SEQUENCE,  'SfiSeq')   : ('nick', 'blurb', 'hints'),
  (Decls.STRING,   'SfiString') : ('nick', 'blurb', 'hints', 'default'),
}

# register extension hooks
__Aida__.add_auxillary_initializers (auxillary_initializers)
