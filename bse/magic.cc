// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "magic.hh"
#include "path.hh"
#include "internal.hh"

namespace Bse {

typedef union {
  int32_t  v_int32;
  uint32_t v_uint32;
  char    *v_string;
} MagicData;

typedef enum {
  MAGIC_CHECK_ANY,
  MAGIC_CHECK_INT_EQUAL,
  MAGIC_CHECK_INT_GREATER,
  MAGIC_CHECK_INT_SMALLER,
  MAGIC_CHECK_UINT_GREATER,
  MAGIC_CHECK_UINT_SMALLER,
  MAGIC_CHECK_UINT_ZEROS,
  MAGIC_CHECK_UINT_ONES,
  MAGIC_CHECK_STRING_EQUAL,
  MAGIC_CHECK_STRING_GREATER,
  MAGIC_CHECK_STRING_SMALLER,
} MagicCheckType;

static const char *const magic_string_delims = " \t\n\r";
static const char *const magic_field_delims = " \t,";

struct FileMagic::Matcher {
  ulong          offset = 0;
  uint           data_size = 0;
  uint32         data_mask = 0xffffffff;
  MagicCheckType magic_check = MAGIC_CHECK_ANY;
  MagicData	 value = { 0 };
  uint           read_string : 1;
  uint		 need_swap : 1;
  uint           cmp_unsigned : 1;
  Matcher() :
    read_string (false), need_swap (false), cmp_unsigned (false)
  {}
  bool parse_offset (char *string);
  bool parse_type   (char *str);
  bool parse_test   (char *str);
  bool read_data    (const String &header, MagicData *data) const;
  bool check_data   (MagicData *data) const;
};

bool
FileMagic::Matcher::parse_offset (char *str)
{
  char *f = NULL;
  if (str[0] == '0')
    offset = strtol (str, &f, str[1] == 'x' ? 16 : 8);
  else
    offset = strtol (str, &f, 10);
  return !f || *f == 0;
}

bool
FileMagic::Matcher::parse_type (char *str)
{
  char *f = NULL;
  if (str[0] == 'u')
    {
      str += 1;
      cmp_unsigned = TRUE;
    }
  if (strncmp (str, "byte", 4) == 0)
    {
      str += 4;
      data_size = 1;
    }
  else if (strncmp (str, "short", 5) == 0)
    {
      str += 5;
      data_size = 2;
    }
  else if (strncmp (str, "leshort", 7) == 0)
    {
      str += 7;
      data_size = 2;
      need_swap = G_BYTE_ORDER != G_LITTLE_ENDIAN;
    }
  else if (strncmp (str, "beshort", 7) == 0)
    {
      str += 7;
      data_size = 2;
      need_swap = G_BYTE_ORDER != G_BIG_ENDIAN;
    }
  else if (strncmp (str, "long", 4) == 0)
    {
      str += 4;
      data_size = 4;
    }
  else if (strncmp (str, "lelong", 6) == 0)
    {
      str += 6;
      data_size = 4;
      need_swap = G_BYTE_ORDER != G_LITTLE_ENDIAN;
    }
  else if (strncmp (str, "belong", 6) == 0)
    {
      str += 6;
      data_size = 4;
      need_swap = G_BYTE_ORDER != G_BIG_ENDIAN;
    }
  else if (strncmp (str, "string", 6) == 0)
    {
      str += 6;
      data_size = 0;
      read_string = TRUE;
    }
  if (str[0] == '&')
    {
      str += 1;
      if (str[0] == '0')
	data_mask = strtol (str, &f, str[1] == 'x' ? 16 : 8);
      else
	data_mask = strtol (str, &f, 10);
      if (f && *f != 0)
	return FALSE;
      while (*str)
	str++;
    }
  else
    data_mask = 0xffffffff;

  return *str == 0;
}

bool
FileMagic::Matcher::parse_test (char *str)
{
  if (!read_string)
    {
      char *f = NULL;
      if (str[0] == '<' || str[0] == '>')
	{
	  if (cmp_unsigned)
	    magic_check = str[0] == '<' ? MAGIC_CHECK_UINT_SMALLER : MAGIC_CHECK_UINT_GREATER;
	  else
	    magic_check = str[0] == '<' ? MAGIC_CHECK_INT_SMALLER : MAGIC_CHECK_INT_GREATER;
	  str += 1;
	}
      else if (str[0] == '^' || str[0] == '&')
	{
	  magic_check = str[0] == '&' ? MAGIC_CHECK_UINT_ONES : MAGIC_CHECK_UINT_ZEROS;
	  str += 1;
	}
      else if (str[0] == 'x')
	{
	  magic_check = MAGIC_CHECK_ANY;
	  str += 1;
	}
      else
	{
	  str += str[0] == '=';
	  magic_check = MAGIC_CHECK_INT_EQUAL;
	}
      if (str[0] == '0')
	value.v_int32 = strtol (str, &f, str[1] == 'x' ? 16 : 8);
      else
	value.v_int32 = strtol (str, &f, 10);

      return *str == 0 || !f || *f == 0;
    }
  else
    {
      char tmp_string[MAGIC_HEADER_SIZE + 1];
      uint n = 0;
      if (str[0] == '<' || str[0] == '>')
	{
	  magic_check = str[0] == '<' ? MAGIC_CHECK_STRING_SMALLER : MAGIC_CHECK_STRING_GREATER;
	  str += 1;
	}
      else
	{
	  str += str[0] == '=';
	  magic_check = MAGIC_CHECK_STRING_EQUAL;
	}
      while (n < MAGIC_HEADER_SIZE && str[n] && !strchr (magic_string_delims, str[n]))
	{
	  if (str[n] != '\\')
	    tmp_string[n] = str[n];
	  else switch ((++str)[n])
	    {
	    case '\\':  tmp_string[n] = '\\';	break;
	    case 't':   tmp_string[n] = '\t';	break;
	    case 'n':   tmp_string[n] = '\n'; 	break;
	    case 'r':   tmp_string[n] = '\r'; 	break;
	    case 'b':   tmp_string[n] = '\b'; 	break;
	    case 'f':   tmp_string[n] = '\f'; 	break;
	    case 's':   tmp_string[n] = ' ';  	break;
	    case 'e':	tmp_string[n] = 27;   	break;
	    default:
	      if (str[n] >= '0' && str[n] <= '7')
		{
		  tmp_string[n] = str[n] - '0';
		  if (str[n + 1] >= '0' && str[n + 1] <= '7')
		    {
		      str += 1;
		      tmp_string[n] *= 8;
		      tmp_string[n] += str[n] - '0';
		      if (str[n + 1] >= '0' && str[n + 1] <= '7')
			{
			  str += 1;
			  tmp_string[n] *= 8;
			  tmp_string[n] += str[n] - '0';
			}
		    }
		}
	      else
		tmp_string[n] = str[n];
	      break;
	    }
	  n++;
	}
      tmp_string[n] = 0;
      data_size = n;
      value.v_string = g_strdup (tmp_string);

      return true;
    }
}

bool
FileMagic::Matcher::read_data (const String &header, MagicData *data) const
{
  if (data_size > MAGIC_HEADER_SIZE || // limit for data->v_string
      offset + data_size > header.size())
    return false;
  if (read_string)
    {
      data->v_string[0] = 0;
      memcpy (data->v_string, header.c_str() + offset, data_size);
      data->v_string[data_size] = 0;
    }
  else if (data_size == 4)
    {
      uint32_t ui32 = 0;
      memcpy (&ui32, header.c_str() + offset, data_size);
      data->v_uint32 = !need_swap ? ui32 : GUINT32_SWAP_LE_BE (ui32);
    }
  else if (data_size == 2)
    {
      uint16_t ui16 = 0;
      memcpy (&ui16, header.c_str() + offset, data_size);
      if (cmp_unsigned)
        data->v_uint32 = !need_swap ? ui16 : GUINT16_SWAP_LE_BE (ui16);
      else
        data->v_int32 = int16_t (!need_swap ? ui16 : GUINT16_SWAP_LE_BE (ui16));
    }
  else if (data_size == 1)
    {
      uint8_t ui8;
      memcpy (&ui8, header.c_str() + offset, data_size);
      if (cmp_unsigned)
        data->v_uint32 = ui8;
      else
        data->v_int32 = int8_t (ui8);
    }
  else
    return false;
  return true;
}

bool
FileMagic::Matcher::check_data (MagicData *data) const
{
  int cmp = 0;
  switch (magic_check)
    {
      uint l;
    case MAGIC_CHECK_ANY:
      cmp = 1;
      break;
    case MAGIC_CHECK_INT_EQUAL:
      data->v_int32 &= data_mask;
      cmp = data->v_int32 == value.v_int32;
      break;
    case MAGIC_CHECK_INT_GREATER:
      data->v_int32 &= data_mask;
      cmp = data->v_int32 > value.v_int32;
      break;
    case MAGIC_CHECK_INT_SMALLER:
      data->v_int32 &= data_mask;
      cmp = data->v_int32 < value.v_int32;
      break;
    case MAGIC_CHECK_UINT_GREATER:
      data->v_uint32 &= data_mask;
      cmp = data->v_uint32 > value.v_uint32;
      break;
    case MAGIC_CHECK_UINT_SMALLER:
      data->v_uint32 &= data_mask;
      cmp = data->v_uint32 < value.v_uint32;
      break;
    case MAGIC_CHECK_UINT_ZEROS:
      data->v_uint32 &= data_mask;
      cmp = (data->v_int32 & value.v_int32) == 0;
      break;
    case MAGIC_CHECK_UINT_ONES:
      data->v_uint32 &= data_mask;
      cmp = (data->v_int32 & value.v_int32) == value.v_int32;
      break;
    case MAGIC_CHECK_STRING_EQUAL:
      l = data_size < 1 ? strlen (data->v_string) : data_size;
      cmp = strncmp (data->v_string, value.v_string, l) == 0;
      break;
    case MAGIC_CHECK_STRING_GREATER:
      l = data_size < 1 ? strlen (data->v_string) : data_size;
      cmp = strncmp (data->v_string, value.v_string, l) > 0;
      break;
    case MAGIC_CHECK_STRING_SMALLER:
      l = data_size < 1 ? strlen (data->v_string) : data_size;
      cmp = strncmp (data->v_string, value.v_string, l) < 0;
      break;
    }
  return cmp > 0;
}

bool
FileMagic::match_header (const String &header)
{
  for (const auto &matcher : matchers_)
    {
      char data_string[MAGIC_HEADER_SIZE + 1];
      MagicData data;
      if (matcher.read_string)
	data.v_string = data_string;
      else
	data.v_uint32 = 0;
      if (!matcher.read_data (header, &data) ||
	  !matcher.check_data (&data))
	return false;
    }
  return matchers_.size() != 0;
}

static void
split_at_delim (char *&s)
{
  // skip to next delimiter
  while (*s && !strchr (magic_field_delims, *s))
    s++;
  // end strig here
  if (*s)
    do
      *(s++) = 0;
    while (strchr (magic_field_delims, *s));
  // s is now positioned *past* delimiters
}

bool
FileMagic::parse_spec (const String &magic_spec)
{
  String writable_magic = magic_spec;
  char *magic_string = writable_magic.data();
  char *p = magic_string;

  while (p && *p)
    {
      char *next_line;
      if (*p == '#' || *p == '\n')
	{
	  next_line = strchr (p, '\n');
	  if (next_line)
	    next_line++;
	}
      else
	{
          matchers_.resize (matchers_.size() + 1);
          Matcher &matcher = matchers_.back();
	  magic_string = p;
	  split_at_delim (p);
	  if (!matcher.parse_offset (magic_string))
	    {
              Bse::warning ("unable to parse magic offset \"%s\" from \"%s\"", magic_string, magic_spec);
	      return false;
	    }
	  magic_string = p;
	  split_at_delim (p);
	  if (!matcher.parse_type (magic_string))
	    {
	      Bse::warning ("unable to parse magic type \"%s\" from \"%s\"", magic_string, magic_spec);
	      return false;
	    }
          magic_string = p;
	  next_line = strchr (magic_string, '\n');
	  if (next_line)
	    *(next_line++) = 0;
	  if (!matcher.read_string)
	    split_at_delim (p);
	  if (!matcher.parse_test (magic_string))
	    {
	      Bse::warning ("unable to parse magic test \"%s\" from \"%s\"", magic_string, magic_spec);
	      return false;
	    }
	}
      p = next_line;
    }
  return true;
}

struct MagicList {
  std::mutex              mutex;
  std::vector<FileMagic*> magics;
  bool                    sorted = false;
  void
  add (FileMagic *magic)
  {
    std::lock_guard<std::mutex> locker (mutex);
    magics.push_back (magic);
    sorted = false;
  }
  void
  remove (FileMagic *magic)
  {
    std::lock_guard<std::mutex> locker (mutex);
    auto it = std::find (magics.begin(), magics.end(), magic);
    if (it != magics.end())
      magics.erase (it);
  }
  std::vector<FileMagic*>
  get_locked ()
  {
    // std::lock_guard<std::mutex> locker (mutex);
    if (!sorted)
      std::stable_sort (magics.begin(), magics.end(), [] (const FileMagic *a, const FileMagic *b) {
          return a->priority() < b->priority();
        });
    sorted = true;
    return magics;
  }
};

static MagicList& magic_list() { static MagicList ml; return ml; }

FileMagic::FileMagic (const String &fileextension, const String &magic_spec, const String &description, int priority) :
  extension_ (fileextension), description_ (description), priority_ (priority)
{
  const bool valid_magic_spec = parse_spec (magic_spec);
  assert_return (valid_magic_spec == true);
  magic_list().add (this);
}

FileMagic::~FileMagic()
{
  magic_list().remove (this);
}

FileMagic*
FileMagic::register_magic (const String &fileextensoion, const String &magic_spec, const String &description, int priority)
{
  FileMagic *magic = new FileMagic (fileextensoion, magic_spec, description, priority);
  return magic;
}

String
FileMagic::match_magic (const String &filename, size_t skip_bytes)
{
  std::lock_guard<std::mutex> locker (magic_list().mutex);
  FileMagic *magic = match_list (magic_list().get_locked(), filename, skip_bytes);
  return magic ? magic->description() : "";
}

FileMagic*
FileMagic::match_list (const std::vector<FileMagic*> &magics, const String &filename, size_t skip_bytes)
{
  String header = Path::stringread (filename, skip_bytes + MAGIC_HEADER_SIZE);
  if (header.size() <= skip_bytes)
    return nullptr;
  header = header.substr (skip_bytes);
  // consider magics with matching extensions
  for (const auto &magic : magics)
    if (string_endswith (filename, magic->extension()) &&
        magic->match_header (header))
      return magic;
  // consider magics with other extensions
  for (const auto &magic : magics)
    if (!string_endswith (filename, magic->extension()) &&
        magic->match_header (header))
      return magic;
  // no match
  return nullptr;
}

} // Bse
