// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_MSG_HH__
#define __BIRNET_MSG_HH__
#include <birnet/birnetutils.hh>
#include <stdarg.h>
namespace Birnet {
/* --- messaging --- */
struct Msg {
  /* message parts */
  struct Part;                           /* base for Text* and Check */
  struct Text0; typedef Text0 Title;     /* message title */
  struct Text1; typedef Text1 Primary;   /* primary message */
  struct Text2; typedef Text2 Secondary; /* secondary message (lengthy) */
  struct Text3; typedef Text3 Detail;    /* message details */
  struct Check;                          /* enable/disable message text */
  struct CustomType;
  typedef enum {
    LOG_TO_STDERR     = 1,
    LOG_TO_STDLOG     = 2,
    LOG_TO_HANDLER    = 4,
    _1FORCE32 = 0xf000000
  } LogFlags;
  /* message types */
  typedef enum {
    NONE        = 0,    /* always off */
    ALWAYS      = 1,    /* always on */
    ERROR,      WARNING,        SCRIPT,
    INFO,       DIAG,           DEBUG,
    _2FORCE32 = 0xf000000
  } Type;
  static Type        register_type      (const char         *ident,
                                         Type                default_ouput,
                                         const char         *label);
  static Type        lookup_type        (const String       &ident);
  static const char* type_ident         (Type                mtype);
  static const char* type_label         (Type                mtype);
  static uint32      type_flags         (Type                mtype);
  static inline bool check              (Type                mtype);
  static void        enable             (Type                mtype);
  static void        disable            (Type                mtype);
  static void        configure          (Type                mtype,
                                         LogFlags            log_mask,
                                         const String       &logfile);
  static void        allow_msgs         (const String       &key);
  static void        deny_msgs          (const String       &key);
  static void        configure_stdlog   (bool                redirect_stdlog_to_stderr,
                                         const String       &stdlog_filename,
                                         uint                syslog_priority);
  /* messaging */
  static inline void display         (Type                message_type,
                                      const Part &p0 = empty_part, const Part &p1 = empty_part,
                                      const Part &p2 = empty_part, const Part &p3 = empty_part,
                                      const Part &p4 = empty_part, const Part &p5 = empty_part,
                                      const Part &p6 = empty_part, const Part &p7 = empty_part,
                                      const Part &p8 = empty_part, const Part &p9 = empty_part);
  static inline void display         (const CustomType   &message_type,
                                      const char         *format,
                                      ...) BIRNET_PRINTF (2, 3);
  /* message handling */
  struct Part {
    String string;
    uint8  ptype;
  public:
    explicit            Part();
  protected:
    void                setup (uint8       ptype,
                               String      smsg);
    void                setup (uint8       ptype,
                               const char *format,
                               va_list     varargs);
  };
  typedef void (*Handler)            (const char         *domain,
                                      Type                mtype,
                                      const vector<Part> &parts);
  static void    set_thread_handler  (Handler             handler);
  static void    default_handler     (const char         *domain,
                                      Type                mtype,
                                      const vector<Part> &parts);
  static void    display_parts       (const char         *domain,
                                      Type                message_type,
                                      const vector<Part> &parts);
protected:
  static const Part   &empty_part;
  static void    display_aparts      (const char         *log_domain,
                                      Type                message_type,
                                      const Part &p0, const Part &p1,
                                      const Part &p2, const Part &p3,
                                      const Part &p4, const Part &p5,
                                      const Part &p6, const Part &p7,
                                      const Part &p8, const Part &p9);
  static void    display_vmsg        (const char         *log_domain,
                                      Type                message_type,
                                      const char         *format,
                                      va_list             args);
  BIRNET_PRIVATE_CLASS_COPY (Msg);
private:
  static Rapicorn::Atomic<int>    n_msg_types;
  static Rapicorn::Atomic<uint8*> msg_type_bits;
  static void            init_standard_types ();
  static void            key_list_change_L   (const String &keylist,
                                              bool          isenabled);
  static void            set_msg_type_L      (uint          mtype,
                                              uint32        flags,
                                              bool          enabled);
public:
  struct Text0 : public Part {  /* message title */
    explicit BIRNET_PRINTF (2, 3) Text0 (const char *format, ...) { va_list a; va_start (a, format); setup ('0', format, a); va_end (a); }
    explicit                      Text0 (const String &s)         { setup ('0', s); }
  };
  struct Text1 : public Part {  /* primary message */
    explicit BIRNET_PRINTF (2, 3) Text1 (const char *format, ...) { va_list a; va_start (a, format); setup ('1', format, a); va_end (a); }
    explicit                      Text1 (const String &s)         { setup ('1', s); }
  };
  struct Text2 : public Part {  /* secondary message (lengthy) */
    explicit BIRNET_PRINTF (2, 3) Text2 (const char *format, ...) { va_list a; va_start (a, format); setup ('2', format, a); va_end (a); }
    explicit                      Text2 (const String &s)         { setup ('2', s); }
  };
  struct Text3 : public Part {  /* message details */
    explicit BIRNET_PRINTF (2, 3) Text3 (const char *format, ...) { va_list a; va_start (a, format); setup ('3', format, a); va_end (a); }
    explicit                      Text3 (const String &s)         { setup ('3', s); }
  };
  struct Check : public Part {  /* user switch */
    explicit BIRNET_PRINTF (2, 3) Check (const char *format, ...) { va_list a; va_start (a, format); setup ('c', format, a); va_end (a); }
    explicit                      Check (const String &s)         { setup ('c', s); }
  };
  struct Custom : public Part { /* custom part / user defined */
    explicit BIRNET_PRINTF (3, 4) Custom (uint8 ctype, const char *format, ...) { va_list a; va_start (a, format); setup (ctype | 0x80, format, a); va_end (a); }
    explicit                      Custom (uint8 ctype, const String &s)         { setup (ctype | 0x80, s); }
  };
  struct CustomType {
    Type type;
    explicit CustomType (const char         *ident,
                         Type                default_ouput,
                         const char         *label = NULL) :
      type (register_type (ident, default_ouput, label))
    {}
    BIRNET_PRIVATE_CLASS_COPY (CustomType);
  };
};
/* --- inline implementations --- */
inline bool
Msg::check (Type mtype)
{
  /* this function is supposed to preserve errno */
  return (mtype >= 0 &&
          mtype < n_msg_types &&
          (msg_type_bits[mtype / 8] & (1 << mtype % 8)));
}
inline void
Msg::display (Type        message_type,
              const Part &p0, const Part &p1,
              const Part &p2, const Part &p3,
              const Part &p4, const Part &p5,
              const Part &p6, const Part &p7,
              const Part &p8, const Part &p9)
{
  /* this function is supposed to preserve errno */
  if (check (message_type))
    display_aparts (BIRNET_LOG_DOMAIN, message_type, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
}
inline void
Msg::display (const CustomType   &message_type,
              const char         *format,
              ...)
{
  if (check (message_type.type))
    {
      va_list args;
      va_start (args, format);
      display_vmsg (BIRNET_LOG_DOMAIN, message_type.type, format, args);
      va_end (args);
    }
}
} // Birnet
#endif /* __BIRNET_MSG_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
