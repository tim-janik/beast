// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemain.hh"
#include "bsecategories.hh"
#include "bseprocedure.hh"
#include "bseglue.hh"
#include <sfi/sfiglue.hh>
#include <stdio.h>
#include <string.h>
#include <string>
#include <set>

std::set<std::string> needTypes;
std::set<std::string> needClasses;
std::set<std::string> excludeTypes;

bool silent = false;

void print(const gchar *format, ...)
{
  va_list args;
  
  va_start (args, format);
  if (!silent) vfprintf (stdout, format, args);
  va_end (args);
}


std::string removeBse (const std::string& name)
{
  if (strncmp (name.c_str(), "Bse", 3) == 0)
    return name.substr (3);
  else if (strncmp (name.c_str(), "BSE_", 4) == 0)
    return name.substr (4);
  else
    return name;
}

std::string getInterface (const std::string& name)
{
  int i = name.find ("+", 0);
  
  if(i >= 0)
    {
      std::string result = name.substr (0, i);
      if (strncmp (result.c_str(), "Bse", 3) == 0)
        result = name.substr (3, i-3);
      
      return result;
    }
  return "";
}

std::string getMethod (const std::string& name)
{
  std::string result;
  std::string::const_iterator ni = name.begin ();
  
  int pos = name.find ("+", 0);
  if (pos >= 0)
    ni += pos + 1;
  else if (name.size () > 4)
    ni += 4; /* assume & skip bse prefix */
  
  while (ni != name.end ())
    {
      if (*ni == '-')
        result += '_';
      else
        result += *ni;
      ni++;
    }
  return result;
}

std::string
signalName (const std::string& signal)
{
  std::string result;
  std::string::const_iterator si = signal.begin ();
  
  while (si != signal.end ())
    {
      if (*si == '-')
        result += '_';
      else
        result += *si;
      si++;
    }
  return result;
}

std::string paramName (const std::string& name)
{
  std::string result;
  std::string::const_iterator ni = name.begin ();
  while (ni != name.end ())
    {
      if (*ni == '-')
        result += '_';
      else
        result += *ni;
      ni++;
    }
  return result;
}

std::string activeInterface = "";
int indent = 0;

void printIndent ()
{
  for (int i = 0; i < indent; i++)
    print("  ");
}

void setActiveInterface (const std::string& x, const std::string& parent)
{
  if (activeInterface != x)
    {
      if (activeInterface != "")
        {
          indent--;
          printIndent ();
          print ("};\n\n");
        }
      
      activeInterface = x;
      
      if (activeInterface != "")
        {
          printIndent ();
          if (needTypes.count("Bse" + activeInterface) > 0)
            needClasses.insert(activeInterface);
          print ("class %s", activeInterface.c_str ());
          if (parent != "")
            print (" : %s", parent.c_str());
          print (" {\n");
          indent++;
        }
    }
}

std::string idlType (GType g)
{
  std::string s = g_type_name (g);
  
  if (s[0] == 'B' && s[1] == 's' && s[2] == 'e')
    {
      needTypes.insert (s);
      return s.substr(3, s.size() - 3);
    }
  switch (G_TYPE_FUNDAMENTAL (g))
    {
    case G_TYPE_INT64:
    case G_TYPE_UINT64:         return "Num";
    case G_TYPE_INT:
    case G_TYPE_UINT:           return "Int";
    case G_TYPE_STRING:         return "String";
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:         return "Real";
    case G_TYPE_BOOLEAN:        return "Bool";
    default:
      if (s == "SfiFBlock")
        return "FBlock";
      else if (s == "SfiRec")
        return "Rec";
      g_error ("bseprocidl: unsupported argument type: %s", s.c_str());
      return "*ERROR*";
    }
}

std::string symbolForInt (int i)
{
  if (i == SFI_MAXINT) return "SFI_MAXINT";
  if (i == SFI_MININT) return "SFI_MININT";
  
  char *x = g_strdup_printf ("%d", i);
  std::string result = x;
  g_free(x);
  return result;
}

void printPSpec (const char *dir, GParamSpec *pspec)
{
  std::string pname = paramName (pspec->name);
  
  printIndent ();
  print ("%-4s%-20s= (\"%s\", \"%s\", ",
         dir,
         pname.c_str(),
         g_param_spec_get_nick (pspec) ?  g_param_spec_get_nick (pspec) : "",
         g_param_spec_get_blurb (pspec) ?  g_param_spec_get_blurb (pspec) : ""
         );
  
  if (SFI_IS_PSPEC_INT (pspec))
    {
      int default_value, minimum, maximum, stepping_rate;
      
      default_value = sfi_pspec_get_int_default (pspec);
      sfi_pspec_get_int_range (pspec, &minimum, &maximum, &stepping_rate);
      
      print("%s, %s, %s, %s, ", symbolForInt (default_value).c_str(),
            symbolForInt (minimum).c_str(), symbolForInt (maximum).c_str(),
            symbolForInt (stepping_rate).c_str());
    }
  if (SFI_IS_PSPEC_BOOL (pspec))
    {
      GParamSpecBoolean *bspec = G_PARAM_SPEC_BOOLEAN (pspec);
      
      print("%s, ", bspec->default_value ? "TRUE" : "FALSE");
    }
  print("\":flagstodo\");\n");
}

void printMethods (const std::string& iface)
{
  BseCategorySeq *cseq;
  guint i;
  
  cseq = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE);
  for (i = 0; i < cseq->n_cats; i++)
    {
      GType type_id = g_type_from_name (cseq->cats[i]->type);
      const gchar *blurb = bse_type_get_blurb (type_id);
      BseProcedureClass *klass = (BseProcedureClass *)g_type_class_ref (type_id);
      
      /* procedures */
      std::string t = cseq->cats[i]->type;
      std::string iname = getInterface (t);
      std::string mname = getMethod (t);
      std::string rtype = klass->n_out_pspecs ?
                          idlType (klass->out_pspecs[0]->value_type) : "void";
      
      if (iname == iface)
	{
	  /* for methods, the first argument is implicit: the object itself */
	  guint first_p = iface == "" ? 0 : 1;
          
	  printIndent ();
	  print ("%s %s (", rtype.c_str(), mname.c_str ());
	  for (guint p = first_p; p < klass->n_in_pspecs; p++)
	    {
	      std::string ptype = idlType (klass->in_pspecs[p]->value_type);
	      std::string pname = paramName (klass->in_pspecs[p]->name);
	      if (p != first_p)
                print(", ");
	      print ("%s %s", ptype.c_str(), pname.c_str());
	    }
	  print(") {\n");
	  indent++;
          
	  if (blurb)
	    {
	      char *ehelp = g_strescape (blurb, 0);
	      printIndent ();
	      print ("Info help = \"%s\";\n", ehelp);
	      g_free (ehelp);
	    }
          
	  for (guint p = first_p; p < klass->n_in_pspecs; p++)
	    printPSpec ("In", klass->in_pspecs[p]);
          
	  for (guint p = 0; p < klass->n_out_pspecs; p++)
	    printPSpec ("Out", klass->out_pspecs[p]);
          
	  indent--;
	  printIndent ();
	  print ("}\n");
	}
      g_type_class_unref (klass);
    }
  bse_category_seq_free (cseq);
}

/* FIXME: we might want to have a sfi_glue_iface_parent () method */
void printInterface (const std::string& iface, const std::string& parent = "")
{
  std::string idliface = removeBse (iface);

  if (excludeTypes.count (iface))
    return;

  setActiveInterface (idliface, parent);
  printMethods (idliface);
  
  if (iface != "")
    {
      /* signals */
      GType type_id = g_type_from_name (iface.c_str());
      if (G_TYPE_IS_INSTANTIATABLE (type_id))
	{
	  guint n_sids;
	  guint *sids = g_signal_list_ids (type_id, &n_sids);
	  for (guint s = 0; s < n_sids; s++)
	    {
	      GSignalQuery query;
	      g_signal_query (sids[s], &query);

              // FIXME: some core types are implemented as plugins, and thus have
              // class destructors that are executed right after type registration.
              // that's why we can't query their signals here
              if (!query.signal_id)
                continue;

	      // FIXME: how to deal with Bse::MidiEvent?
	      if (signalName (query.signal_name) == "midi_event")
		continue;

	      printIndent();
	      print ("signal %s (", signalName (query.signal_name).c_str());
	      for (guint p = 0; p < query.n_params; p++)
		{
		  std::string ptype = idlType (query.param_types[p]);
		  std::string pname = ""; pname += char('a' + p);
		  if (p != 0)
                    print(", ");
		  print ("%s %s", ptype.c_str(), pname.c_str());
                }
	      print(");\n");
	    }
	}
      else
	{
	  print("/* type %s (%s) is not instantiatable */\n", g_type_name (type_id), iface.c_str());
	}
     
      /* properties */
      GObjectClass *klass = (GObjectClass *)g_type_class_ref (type_id);
      if (klass)
	{
	  guint n_properties = 0;
	  GParamSpec **properties = g_object_class_list_properties (klass, &n_properties);
	 
	  for (guint i = 0; i < n_properties; i++)
	    {
	      if (properties[i]->owner_type == type_id)
		printPSpec (("property "+idlType(properties[i]->value_type)+" ").c_str(), properties[i]);
	    }
	  g_free (properties);
	  g_type_class_unref (klass);
	}

      const gchar **children = sfi_glue_iface_children (iface.c_str());
      while (*children)
        printInterface (*children++, idliface);
    }
}

static void
printChoices (void)
{
  GType *children;
  guint n, i;
  
  children = g_type_children (G_TYPE_ENUM, &n);
  for (i = 0; i < n; i++)
    {
      const gchar *name = g_type_name (children[i]);
      GEnumClass *eclass = (GEnumClass *)g_type_class_ref (children[i]);
      gboolean regular_choice = strcmp (name, "BseErrorType") != 0;
      GEnumValue *val;

      if (needTypes.count (name) && !excludeTypes.count (name))
	{
	  /* enum definition */
	  printIndent ();
	  print ("choice %s {\n", removeBse(name).c_str());
          indent++;
	  for (val = eclass->values; val->value_name; val++)
	    {
	      bool neutral = (!regular_choice && val == eclass->values);
	      printIndent();
              if (neutral)
                print ("%s = (Neutral, \"%s\"),\n", removeBse (val->value_name).c_str(),
                       val->value_nick);
              else
                print ("%s = (%d, \"%s\"),\n", removeBse (val->value_name).c_str(),
                       val->value, val->value_nick);
	    }
          indent--;
	  printIndent ();
	  print ("};\n\n", name);
	}
      /* cleanup */
      g_type_class_unref (eclass);
    }
  g_free (children);
}

void
printForwardDecls ()
{
  std::set<std::string>::iterator ci;
  
  for (ci = needClasses.begin(); ci != needClasses.end(); ci++)
    {
      printIndent();
      print ("class %s;\n", ci->c_str());
    }
}

int
main (int argc, char **argv)
{
  /* exclude all types given in a file, passed as first argument, from generation */
  if (argc == 2)
    {
      FILE *excludefile = fopen (argv[1], "r");
      if (!excludefile)
	fprintf (stderr, "excludefile %s not found\n", argv[1]);
      else
	{
	  char buffer[1024];
	  while (fgets (buffer, 1024, excludefile))
	    {
	      buffer[strlen(buffer) - 1] = 0;
	      printf ("// bseprocidl: type %s excluded from generation\n", buffer);
	      excludeTypes.insert (buffer);
	    }
	}
    }
  g_thread_init (NULL);
  bse_init_inprocess (&argc, &argv, "BseProcIDL", NULL);
  
  sfi_glue_context_push (bse_glue_context_intern ("BseProcIdl"));
  std::string s = sfi_glue_base_iface ();
  
  /* small hackery to collect all enum types that need to be printed */
  silent = true;
  // needTypes.insert (g_type_name (BSE_TYPE_MIDI_SIGNAL_TYPE));
  printInterface (s);
  printInterface ("");
  silent = false;
  
  print ("namespace Bse {\n");
  indent++;
  printChoices ();
  printForwardDecls ();
  printInterface (s);
  printInterface ("");  /* prints procedures without interface */
  indent--;
  print ("};\n");
  
  
  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
