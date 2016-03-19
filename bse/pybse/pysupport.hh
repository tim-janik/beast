// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PYSUPPORT_HH__
#define __BSE_PYSUPPORT_HH__

#include <bse/bse.hh>
#include <Python.h>

namespace Bse {

bool py_init_async (const StringVector &args = StringVector());

bool
py_init_async (const StringVector &args)
{
  // avoid double initialization
  if (!init_needed())
    return true; // success
  // convert sys.argv to char **argv;
  PyObject *pyargv = PySys_GetObject (const_cast<char*> ("argv"));
  const size_t pyargc = pyargv && PyList_Check (pyargv) ? PyList_Size (pyargv) : 0;
  for (size_t i = 0; i < pyargc; i++)
    if (!PyString_Check (PyList_GetItem (pyargv, i)))
      {
        pyargv = NULL;
        break;
      }
  if (pyargc && !pyargv)
    PyErr_Warn (PyExc_Warning, Rapicorn::string_format ("%s: failed to readout sys.argv", __func__).c_str());
  if (PyErr_Occurred())
    return false; // indicate exception
  std::vector<String> sargv;
  for (size_t i = pyargv ? 0 : pyargc; i < pyargc; i++)
    printerr("argv[%d]: %s\n", i, PyString_AsString (PyList_GetItem (pyargv, i)));
  for (size_t i = pyargv ? 0 : pyargc; i < pyargc; i++)
    sargv.push_back (PyString_AsString (PyList_GetItem (pyargv, i)));
  if (sargv.size() == 0)
    sargv.push_back (Rapicorn::program_name());
  char *cargv[sargv.size() + 1];
  for (size_t i = 0; i < sargv.size(); i++)
    cargv[i] = const_cast<char*> (sargv[i].c_str());
  cargv[sargv.size()] = NULL;
  // initialize libbse
  int cargc = sargv.size();
  cargv[0] = Py_GetProgramName();                       // correct Python's 'fabricated' argv[0]
  init_async (&cargc, cargv, NULL, args);
  // propagate argv adjustments back to sys.argv
  if (cargc < ssize_t (sargv.size()))
    {
      cargv[0] = const_cast<char*> (sargv[0].c_str());  // retain Python's 'fabricated' argv[0]
      PySys_SetArgv (cargc, cargv);
    }
  return !PyErr_Occurred();
}

} // Bse

#endif /* __BSE_PYSUPPORT_HH__ */
