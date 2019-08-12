// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;

#include <jsonipc/jsonipc.hh>

#include <bse/bseenums.hh>      // enums API interfaces, etc
#include <bse/platform.hh>
#include <bse/randomhash.hh>
#include <bse/regex.hh>
#include <bse/bse.hh>   // Bse::init_async
#include <limits.h>
#include <stdlib.h>

// == Aida Workarounds ==
// Manually convert between Aida Handle types (that Jsonipc cannot know about)
// and shared_ptr to Iface types.
// Bse::*Seq as std::vector
template<typename Bse_Seq, typename Bse_IfaceP>
struct ConvertSeq {
  static Bse_Seq
  from_json (const Jsonipc::JsonValue &jarray)
  {
    std::vector<Bse_IfaceP> pointers = Jsonipc::from_json<std::vector<Bse_IfaceP>> (jarray);
    Bse_Seq seq;
    seq.reserve (pointers.size());
    for (auto &ptr : pointers)
      seq.emplace_back (ptr->__handle__());
    return seq;
  }
  static Jsonipc::JsonValue
  to_json (const Bse_Seq &vec, Jsonipc::JsonAllocator &allocator)
  {
    std::vector<Bse_IfaceP> pointers;
    pointers.reserve (vec.size());
    for (auto &itemhandle : vec)
      pointers.emplace_back (itemhandle.__iface__()->template as<Bse_IfaceP>());
    return Jsonipc::to_json (pointers, allocator);
  }
};

/// Convert between Aida::Any and Jsonipc::JsonValue
struct ConvertAny {
  static Aida::Any
  from_json (const Jsonipc::JsonValue &v, const Aida::Any &fallback = Aida::Any())
  {
    Aida::Any any;
    switch (v.GetType())
      {
      case rapidjson::kNullType:
        return fallback;
      case rapidjson::kFalseType:
        any.set<bool> (false);
        break;
      case rapidjson::kTrueType:
        any.set<bool> (true);
        break;
      case rapidjson::kStringType:
        any.set<std::string> (Jsonipc::from_json<std::string> (v));
        break;
      case rapidjson::kNumberType:
        if      (v.IsInt())     any.set<int32_t> (v.GetInt());
        else if (v.IsUint())    any.set<int64_t> (v.GetUint());
        else if (v.IsInt64())   any.set<int64_t> (v.GetInt64());
        else                    any.set<double>  (v.GetDouble());
      case rapidjson::kArrayType:
        sequence_from_json_array (any, v);
        break;
      case rapidjson::kObjectType:
        record_from_json_object (any, v);
        break;
    };
    return any;
  }
  static Jsonipc::JsonValue
  to_json (const Aida::Any &any, Jsonipc::JsonAllocator &allocator)
  {
    using namespace Jsonipc;
    switch (int (any.kind()))
      {
      case Aida::BOOL:          return JsonValue (any.get<bool>());
      case Aida::INT32:         return JsonValue (any.get<int32_t>());
      case Aida::INT64:         return JsonValue (any.get<int64_t>());
      case Aida::FLOAT64:       return JsonValue (any.get<double>());
      case Aida::ENUM:          return Jsonipc::to_json (any.get<std::string>(), allocator);
      case Aida::STRING:        return Jsonipc::to_json (any.get<std::string>(), allocator);
      case Aida::SEQUENCE:      return sequence_to_json_array (any.get<const Aida::AnySeq&>(), allocator);
      case Aida::RECORD:        return record_to_json_object (any.get<const Aida::AnyRec&>(), allocator);
      case Aida::INSTANCE:      return instance_to_json_object (any, allocator);
      }
    return JsonValue(); // null
  }
  static void
  sequence_from_json_array (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    const size_t l = v.Size();
    Aida::AnySeq s;
    for (size_t i = 0; i < l; ++i)
      s.push_back (ConvertAny::from_json (v[i]));
    any.set (s);
  }
  static Jsonipc::JsonValue
  sequence_to_json_array (const Aida::AnySeq &seq, Jsonipc::JsonAllocator &allocator)
  {
    const size_t l = seq.size();
    Jsonipc::JsonValue jarray (rapidjson::kArrayType);
    jarray.Reserve (l, allocator);
    for (size_t i = 0; i < l; ++i)
      jarray.PushBack (ConvertAny::to_json (seq[i], allocator).Move(), allocator);
    return jarray;
  }
  static void
  record_from_json_object (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    Aida::AnyRec r;
    for (const auto &field : v.GetObject())
      {
        const std::string key = field.name.GetString();
        if (key == "$class" || key == "$id")    // actually Aida::INSTANCE
          return instance_from_json_object (any, v);
        r[key] = ConvertAny::from_json (field.value);
      }
    any.set (r);
  }
  static Jsonipc::JsonValue
  record_to_json_object (const Aida::AnyRec &r, Jsonipc::JsonAllocator &allocator)
  {
    Jsonipc::JsonValue jobject (rapidjson::kObjectType);
    jobject.MemberReserve (r.size(), allocator);
    for (auto const &field : r)
      jobject.AddMember (Jsonipc::JsonValue (field.name.c_str(), allocator),
                         ConvertAny::to_json (field, allocator).Move(), allocator);
    return jobject;
  }
  static void
  instance_from_json_object (Aida::Any &any, const Jsonipc::JsonValue &v)
  {
    Aida::ImplicitBaseP basep = Jsonipc::Convert<Aida::ImplicitBaseP>::from_json (v);
    if (!basep)
      return;
    // FIXME: remove special casing of base object types once RemoteHandle is gone
    Bse::ObjectIfaceP objectp = std::dynamic_pointer_cast<Bse::ObjectIface> (basep);
    if (objectp)
      any.set (objectp);
    Bse::SignalMonitorIfaceP signalmonitorp = std::dynamic_pointer_cast<Bse::SignalMonitorIface> (basep);
    if (signalmonitorp)
      any.set (signalmonitorp);
  }
  static Jsonipc::JsonValue
  instance_to_json_object (const Aida::Any &any, Jsonipc::JsonAllocator &allocator)
  {
    Aida::ImplicitBaseP basep = any.get<Aida::ImplicitBaseP> ();
    return Jsonipc::Convert<Aida::ImplicitBaseP>::to_json (basep, allocator);
  }
};


namespace Jsonipc {
// Bse::ItemSeq as std::vector
template<>      struct Convert<Bse::ItemSeq>    : ConvertSeq<Bse::ItemSeq, Bse::ItemIfaceP> {};
// Bse::PartSeq as std::vector
template<>      struct Convert<Bse::PartSeq>    : ConvertSeq<Bse::PartSeq, Bse::PartIfaceP> {};
// Bse::SuperSeq as std::vector
template<>      struct Convert<Bse::SuperSeq>   : ConvertSeq<Bse::SuperSeq, Bse::SuperIfaceP> {};
// Bse::WaveOscSeq as std::vector
template<>      struct Convert<Bse::WaveOscSeq> : ConvertSeq<Bse::WaveOscSeq, Bse::WaveOscIfaceP> {};

// Aida::Any
template<>      struct Convert<Aida::Any> : ConvertAny {};

// Bse::PartHandle as Bse::PartIfaceP (in records)
template<>
struct Convert<Aida::RemoteMember<Bse::PartHandle>> {
  static Aida::RemoteMember<Bse::PartHandle>
  from_json (const Jsonipc::JsonValue &jvalue)
  {
    Bse::PartIfaceP ptr = Jsonipc::from_json<Bse::PartIfaceP> (jvalue);
    return ptr->__handle__();
  }
  static Jsonipc::JsonValue
  to_json (const Aida::RemoteMember<Bse::PartHandle> &handle, Jsonipc::JsonAllocator &allocator)
  {
    Bse::PartIfaceP ptr = handle.__iface__()->template as<Bse::PartIfaceP>();
    return Jsonipc::to_json (ptr, allocator);
  }
};

} // Jsonipc


#include "bsebus.hh"
#include "bsecontextmerger.hh"
#include "bsecsynth.hh"
#include "bseeditablesample.hh"
#include "bsemidinotifier.hh"
#include "bsemidisynth.hh"
#include "bsepart.hh"
#include "bsepcmwriter.hh"
#include "bseproject.hh"
#include "bseserver.hh"
#include "bsesnet.hh"
#include "bsesong.hh"
#include "bsesong.hh"
#include "bsesoundfont.hh"
#include "bsesoundfontrepo.hh"
#include "bsetrack.hh"
#include "bsewave.hh"
#include "bsewaveosc.hh"
#include "bsewaverepo.hh"
#include "monitor.hh"
#include "bse/bseapi_jsonipc.cc"    // Bse_jsonipc_stub

#undef B0 // undo pollution from termios.h

static Bse::ServerH bse_server;
static Jsonipc::IpcDispatcher *dispatcher = NULL;
static bool verbose = false;
static GPollFD embedding_pollfd = { -1, 0, 0 };
static std::string authenticated_subprotocol = "";

// Configure websocket server
struct CustomServerConfig : public websocketpp::config::asio {
  static const size_t connection_read_buffer_size = 16384;
};
using ServerEndpoint = websocketpp::server<CustomServerConfig>;
static ServerEndpoint websocket_server;

static std::string
handle_jsonipc (const std::string &message, const websocketpp::connection_hdl &hdl)
{
  ptrdiff_t conid = 0;
  if (verbose)
    {
      conid = ptrdiff_t (websocket_server.get_con_from_hdl (hdl).get());
      Bse::printerr ("%p: REQUEST: %s\n", conid, message);
    }
  const std::string reply = dispatcher->dispatch_message (message);
  if (verbose)
    {
      const bool iserror = bool (Bse::Re::search (R"(^\{("id":[0-9]+,)?"error":)", reply));
      if (iserror)
        {
          using namespace Bse::AnsiColors;
          auto R1 = color (BOLD) + color (FG_RED), R0 = color (FG_DEFAULT) + color (BOLD_OFF);
          Bse::printerr ("%p: %sREPLY:%s   %s\n", conid, R1, R0, reply);
        }
      else
        Bse::printerr ("%p: REPLY:   %s\n", conid, reply);
    }
  return reply;
}

static std::string
user_agent_nick (const std::string &useragent)
{
  using namespace Bse;
  std::string nick;
  if      (Re::search (R"(\bFirefox/)", useragent))
    nick += "Firefox";
  else if (Re::search (R"(\bElectron/)", useragent))
    nick += "Electron";
  else if (Re::search (R"(\bChrome/)", useragent))
    nick += "Chrome";
  else if (Re::search (R"(\bSafari/)", useragent))
    nick += "Safari";
  else
    nick += "Unknown";
  return nick;
}

static bool
ws_validate_connection (websocketpp::connection_hdl hdl)
{
  ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (hdl);
  // using subprotocol as auth string
  const std::vector<std::string> &subprotocols = con->get_requested_subprotocols();
  if (subprotocols.size() == 0 && authenticated_subprotocol.empty())
    return true;
  if (subprotocols.size() == 1)
    {
      if (subprotocols[0] == authenticated_subprotocol)
        {
          con->select_subprotocol (subprotocols[0]);
          return true;
        }
    }
  return false;
}

static void
ws_open_connection (websocketpp::connection_hdl hdl)
{
  ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (hdl);
  // https://github.com/zaphoyd/websocketpp/issues/694#issuecomment-454623641
  const auto &socket = con->get_raw_socket();
  const auto &address = socket.remote_endpoint().address();
  const int rport = socket.remote_endpoint().port();
  const websocketpp::http::parser::request &rq = con->get_request();
  const websocketpp::http::parser::header_list &headermap = rq.get_headers();
  std::string useragent;
  for (auto it : headermap) // request headers
    if (it.first == "User-Agent")
      useragent = it.second;
  std::string nick = user_agent_nick (useragent);
  if (!nick.empty())
    nick = "(" + nick + ")";
  using namespace Bse::AnsiColors;
  auto B1 = color (BOLD);
  auto B0 = color (BOLD_OFF);
  Bse::printout ("%p: %sACCEPT:%s %s:%d/ %s\n", ptrdiff_t (con.get()), B1, B0, address.to_string().c_str(), rport, nick);
  // Bse::printout ("User-Agent: %s\n", useragent);
}

static websocketpp::connection_hdl *bse_current_websocket_hdl = NULL;

static void
ws_message (websocketpp::connection_hdl hdl, server::message_ptr msg)
{
  const std::string &message = msg->get_payload();
  // send message to BSE thread and block until its been handled
  Aida::ScopedSemaphore sem;
  auto handle_wsmsg = [&message, &hdl, &sem] () {
    bse_current_websocket_hdl = &hdl;
    std::string reply = handle_jsonipc (message, hdl);
    bse_current_websocket_hdl = NULL;
    if (!reply.empty())
      websocket_server.send (hdl, reply, websocketpp::frame::opcode::text);
    sem.post();
  };
  bse_server.__iface_ptr__()->__execution_context_mt__().enqueue_mt (handle_wsmsg);
  sem.wait();
}

static std::string
simplify_path (const std::string &path)
{
  using namespace Bse;
  std::vector<std::string> dirs = Bse::string_split (path, "/");
  for (ssize_t i = 0; i < dirs.size(); i++)
    if (dirs[i].empty() || dirs[i] == ".")
      dirs.erase (dirs.begin() + i--);
    else if (dirs[i] == "..")
      {
        dirs.erase (dirs.begin() + i--);
        if (i >= 0)
          dirs.erase (dirs.begin() + i--);
      }
  return "/" + string_join ("/", dirs);
}

static std::string
app_path (const std::string &path)
{
  using namespace Bse;
  static const char *basepath = realpath ((runpath (RPath::INSTALLDIR) + "/app/").c_str(), NULL);
  if (basepath)
    {
      static size_t baselen = strlen (basepath);
      char *uripath = realpath ((basepath + std::string ("/") + path).c_str(), NULL);
      if (uripath && strncmp (uripath, basepath, baselen) == 0 && uripath[baselen] == '/')
        {
          std::string dest = uripath;
          free (uripath);
          return dest;
        }
    }
  return ""; // 404
}

static void
http_request (websocketpp::connection_hdl hdl)
{
  using namespace Bse;
  ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (hdl);
  const ptrdiff_t conid = ptrdiff_t (con.get());
  const auto &parts = Bse::string_split (con->get_resource(), "?");
  std::string simplepath = simplify_path (parts[0]);
  // root
  if (simplepath == "/")
    {
      con->append_header ("Content-Type", "text/html; charset=utf-8");
      con->set_body ("<!DOCTYPE html>\n"
                     "<html><head><title>Beast-Sound-Engine</title></head><body>\n"
                     "<h1>Beast-Sound-Engine</h1>\n"
                     "<a href='/app.html'>app.html</a><br/>\n"
                     "Resource: " + con->get_resource() + "<br/>\n"
                     "URI: " + con->get_uri()->str() + "<br/>\n"
                     "</body></html>\n");
      con->set_status (websocketpp::http::status_code::ok);
      if (verbose)
        Bse::printerr ("%p: GET:     %s\n", conid, simplepath);
      return;
    }
  // file
  std::string filepath = app_path (simplepath);
  if (Path::check (filepath, "drx"))
    filepath = Path::join (filepath, "index.html");
  if (Path::check (filepath, "fr"))
    {
      if (string_endswith (filepath, ".html") || string_endswith (filepath, ".htm"))
        con->append_header ("Content-Type", "text/html; charset=utf-8");
      else if (string_endswith (filepath, ".js") || string_endswith (filepath, ".mjs"))
        con->append_header ("Content-Type", "application/javascript");
      else if (string_endswith (filepath, ".css"))
        con->append_header ("Content-Type", "text/css");
      else if (string_endswith (filepath, ".ico"))
        con->append_header ("Content-Type", "image/x-icon");
      else if (string_endswith (filepath, ".gif"))
        con->append_header ("Content-Type", "image/gif");
      else if (string_endswith (filepath, ".svg"))
        con->append_header ("Content-Type", "image/svg+xml");
      else if (string_endswith (filepath, ".png"))
        con->append_header ("Content-Type", "image/png");
      else if (string_endswith (filepath, ".jpg"))
        con->append_header ("Content-Type", "image/jpeg");
      else
        con->append_header ("Content-Type", "application/octet-stream");
      Blob blob = Blob::from_file (filepath);
      con->set_body (blob.string());
      con->set_status (websocketpp::http::status_code::ok);
      if (verbose)
        Bse::printerr ("%p: FILE:    %s\n", conid, simplepath);
      return;
    }
  // 404
  con->append_header ("Content-Type", "text/html; charset=utf-8");
  con->set_status (websocketpp::http::status_code::not_found);
  con->set_body ("<!DOCTYPE html>\n"
                 "<html><head><title>404 Not Found</title></head><body>\n"
                 "<h1>Not Found</h1>\n"
                 "<p>The requested URL was not found: <tt>" + con->get_uri()->str() + "</tt></p>\n"
                 "<hr><address>" "beast-sound-engine/" + Bse::version() + "</address>\n"
                 "<hr></body></html>\n");
  if (verbose)
    Bse::printerr ("%p: 404:     %s\n", conid, simplepath);
}

class EventHub {
  static ssize_t new_id () { static ssize_t idgen = -1000000; return --idgen; }
  struct EventHandler {
    websocketpp::connection_hdl weak_hdl;       // connection weak_ptr
    Bse::ObjectIfaceW           weak_obj;       // ObjectIface weak_ptr
    Aida::IfaceEventConnection  event_con;      // internally weak_ptr
    const ssize_t               handler_id = 0;
    EventHandler (const EventHandler&) = delete;
    EventHandler& operator= (const EventHandler&) = delete;
    EventHandler (ssize_t handlerid, Bse::ObjectIfaceW obj, websocketpp::connection_hdl chdl) :
      weak_hdl (chdl), weak_obj (obj), handler_id (handlerid)
    {
      BSE_ASSERT_RETURN (handler_id != 0);
    }
    ~EventHandler()
    {
      event_con.disconnect();
    }
    void
    event (const Aida::Event &event)
    {
      Bse::ObjectIfaceP obj = weak_obj.lock();
      ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (weak_hdl);
      if (obj && con)
        {
          rapidjson::Document d (rapidjson::kObjectType);
          auto &a = d.GetAllocator();
          d.AddMember ("method", "Bse/EventHub/event", a);
          Jsonipc::JsonValue jarray (rapidjson::kArrayType);
          jarray.PushBack (Jsonipc::to_json<ssize_t> (handler_id, a).Move(), a);
          jarray.PushBack (ConvertAny::record_to_json_object (event.fields(), a).Move(), a);
          d.AddMember ("params", jarray, a); // move-semantics!
          rapidjson::StringBuffer buffer;
          rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
          d.Accept (writer);
          std::string message { buffer.GetString(), buffer.GetSize() };
          websocket_server.send (weak_hdl, message, websocketpp::frame::opcode::text);
          if (verbose)
            {
              const ptrdiff_t conid = ptrdiff_t (con.get());
              Bse::printerr ("%p: NOTIFY:  %s\n", conid, message);
            }
        }
    }
  };
  using HandlerMap = std::map<ssize_t, EventHandler>;
  static HandlerMap& handlers() { static HandlerMap hmap; return hmap; }
public:
  static std::string*
  connect (Jsonipc::CallbackInfo &cbi)
  {
    if (cbi.n_args() == 2 && bse_current_websocket_hdl)
      {
        auto obj = Jsonipc::from_json<Bse::ObjectIfaceP> (cbi.ntharg (0));
        auto eventname = Jsonipc::from_json<std::string> (cbi.ntharg (1));
        if (obj && !eventname.empty())
          {
            websocketpp::connection_hdl chdl = *bse_current_websocket_hdl;
            const ssize_t handler_id = new_id();
            // handlers()[handler_id] = EventHandler (handler_id, obj, chdl);
            auto pitb = handlers().emplace (std::piecewise_construct, std::forward_as_tuple (handler_id),
                                            std::forward_as_tuple (handler_id, obj, chdl));
            EventHandler *ehandler = &pitb.first->second;
            ehandler->event_con = obj->__attach__ (eventname, [ehandler] (const Aida::Event &event) { ehandler->event (event); });
            cbi.set_result (Jsonipc::to_json (handler_id, cbi.allocator()).Move());
            return NULL;
          }
      }
    return new std::string (cbi.invalid_params);
  }
  static std::string*
  disconnect (Jsonipc::CallbackInfo &cbi)
  {
    if (cbi.n_args() == 1)
      {
        const ssize_t handler_id = Jsonipc::from_json<ssize_t> (cbi.ntharg (0));
        HandlerMap &hmap = handlers();
        auto it = hmap.find (handler_id);
        bool result = false;
        if (it != hmap.end())
          {
            EventHandler &ehandler = it->second;
            ehandler.event_con.disconnect();
            hmap.erase (it);
            result = true;
          }
        cbi.set_result (Jsonipc::to_json (result, cbi.allocator()).Move());
        return NULL;
      }
    return new std::string (cbi.invalid_params);
  }
  /* TODO: delete all EventHandler entries on connection death
   * TODO: clean up EventHandler entries on object deletion
   * TODO: implement periodic GC sweep for event connections, instead of tracking all
   * connection + object destructions. E.g. to simplify mattaers, check all weak_ptrs
   * of all connections once every 257th or so connect() calls, to have an upper bound
   * of leaky stale connections.
   */
};

static void
randomize_subprotocol()
{
  const char *const c64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "_-";
  /* We use subprotocol randomization as authentication, so:
   * a) Authentication happens *before* messages interpretation, so an
   *    unauthenticated sender cannot cause crahses via e.g. rapidjson exceptions.
   * b) To serve as working authentication measure, the subprotocol random string
   *    must be cryptographically-secure.
   */
  Bse::KeccakCryptoRng csprng;
  authenticated_subprotocol = "auth.";
  for (size_t i = 0; i < 43; ++i)                               // 43 * 6 bit >= 256 bit
    authenticated_subprotocol += c64[csprng.random() % 64];     // each step adds 6 bits
}

static void
print_usage (bool help)
{
  if (!help)
    {
      Bse::printout ("beast-sound-engine version %s\n", Bse::version());
      return;
    }
  Bse::printout ("Usage: beast-sound-engine [OPTIONS]\n");
  Bse::printout ("  --help       Print command line help\n");
  Bse::printout ("  --version    Print program version\n");
  Bse::printout ("  --verbose    Print requests and replies\n");
  Bse::printout ("  --embed <fd> Parent process socket for embedding\n");
}

int
main (int argc, char *argv[])
{
  Bse::this_thread_set_name ("BeastSoundEngineMain");
  Bse::init_async (&argc, argv, argv[0]); // Bse::cstrings_to_vector (NULL)
  bse_server = Bse::init_server_instance();

  randomize_subprotocol();

  // Ensure Bse has everything properly loaded
  bse_server.load_assets();

  // Register BSE bindings
  const bool print_jsbse = argc >= 2 && std::string ("--js-bseapi") == argv[1];
  {
    Aida::ScopedSemaphore sem;
    auto handle_wsmsg = [print_jsbse, &sem] () {
      if (print_jsbse)
        Jsonipc::ClassPrinter::recording (true);
      Bse_jsonipc_stub();
      if (print_jsbse)
        Bse::printout ("%s\n", Jsonipc::ClassPrinter::to_string());
      // fixups, we know Bse::Server is a singleton
      Jsonipc::Class<Bse::ServerIface> jsonipc__Bse_ServerIface;
      jsonipc__Bse_ServerIface.eternal();
      Jsonipc::Class<Bse::ServerImpl> jsonipc__Bse_ServerImpl;
      jsonipc__Bse_ServerImpl.eternal();
      sem.post();
    };
    bse_server.__iface_ptr__()->__execution_context_mt__().enqueue_mt (handle_wsmsg);
    sem.wait();
    if (print_jsbse)
      return 0;
  }

  // Setup Jsonipc dispatcher
  dispatcher = new Jsonipc::IpcDispatcher();
  dispatcher->add_method ("$jsonipc.initialize",
                          [] (Jsonipc::CallbackInfo &cbi) -> std::string* {
                            Bse::ServerIface &server_iface = Bse::ServerImpl::instance();
                            cbi.set_result (Jsonipc::to_json (server_iface, cbi.allocator()).Move());
                            return NULL;
                          });
  dispatcher->add_method ("Bse/EventHub/connect", &EventHub::connect);
  dispatcher->add_method ("Bse/EventHub/disconnect", &EventHub::disconnect);

  // parse arguments
  bool seen_dashdash = false;
  std::vector<std::string> words;
  for (size_t i = 0; i < argc; i++)
    if (!argv[i])
      continue;
    else if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == 0)
      seen_dashdash = true;
    else if (!seen_dashdash && argv[i][0] == '-')
      {
        const char *arg = argv[i] + 1 + (argv[i][1] == '-');
        const char *eq = strchr (arg, '=');
        const std::string arg_name = !eq ? arg : std::string (arg, eq - arg);
        if (arg_name == "version")
          {
            print_usage (false);
            return 0;
          }
        else if (arg_name == "embed" && i + 1 < argc)
          embedding_pollfd.fd = Bse::string_to_int (argv[++i]);
        else if (arg_name == "h" || arg_name == "help")
          {
            print_usage (true);
            return 0;
          }
        else if (arg_name == "v" || arg_name == "verbose")
          {
            verbose = true;
          }
        else
          {
            Bse::printerr ("%s: invalid argument: %s\n", argv[0], argv[i]);
            print_usage (true);
            return 1;
          }
      }
    else
      words.push_back (argv[i]);

  // add keep-alive-fd monitoring
  if (embedding_pollfd.fd >= 0)
    {
      Aida::ScopedSemaphore sem;
      auto handle_wsmsg = [&sem] () {
        embedding_pollfd.events = G_IO_IN | G_IO_HUP; // G_IO_PRI | (G_IO_IN | G_IO_HUP) | (G_IO_OUT | G_IO_ERR);
        GSource *source = g_source_simple (BSE_PRIORITY_NORMAL,
                                           [] (void*, int *timeoutp) -> int { // pending
                                             return embedding_pollfd.revents != 0;
                                           },
                                           [] (void*) { // dispatch,
                                             if (embedding_pollfd.revents & G_IO_IN)
                                               {
                                                 static char b[512];
                                                 ssize_t n = read (embedding_pollfd.fd, b, 512); // clear input
                                                 (void) n;
                                               }
                                             if (embedding_pollfd.revents & G_IO_HUP)
                                               _exit (0);
                                           },
                                           nullptr, // data,
                                           nullptr, // destroy,
                                           &embedding_pollfd, nullptr);
        g_source_attach (source, bse_main_context);
        sem.post();
      };
      bse_server.__iface_ptr__()->__execution_context_mt__().enqueue_mt (handle_wsmsg);
      sem.wait();
    }

  const int BEAST_AUDIO_ENGINE_PORT = 27239;    // 0x3ea67 % 32768

  // setup websocket and run asio loop
  websocket_server.set_user_agent ("beast-sound-engine/" + Bse::version());
  websocket_server.set_http_handler (&http_request);
  websocket_server.set_validate_handler (&ws_validate_connection);
  websocket_server.set_open_handler (&ws_open_connection);
  websocket_server.set_message_handler (&ws_message);
  websocket_server.init_asio();
  websocket_server.clear_access_channels (websocketpp::log::alevel::all);
  websocket_server.set_reuse_addr (true);
  namespace IP = boost::asio::ip;
  size_t localhost_port = embedding_pollfd.fd >= 0 ? 0 : BEAST_AUDIO_ENGINE_PORT;
  IP::tcp::endpoint endpoint_local = IP::tcp::endpoint (IP::address::from_string ("127.0.0.1"), localhost_port);
  websocket_server.listen (endpoint_local);
  websocket_server.start_accept();
  if (localhost_port == 0)
    {
      websocketpp::lib::asio::error_code ec;
      localhost_port = websocket_server.get_local_endpoint (ec).port();
    }
  using namespace Bse::AnsiColors;
  auto B1 = color (BOLD);
  auto B0 = color (BOLD_OFF);
  std::string fullurl = Bse::string_format ("http://127.0.0.1:%d/app.html", localhost_port);
  if (embedding_pollfd.fd >= 0)
    {
      websocketpp::lib::asio::error_code ec;
      fullurl += Bse::string_format ("?subprotocol=%s", authenticated_subprotocol);
      std::string embed = "{ \"url\": \"" + fullurl + "\" }";
      ssize_t n;
      do
        n = write (embedding_pollfd.fd, embed.data(), embed.size());
      while (n < 0 && errno == EINTR);
      Bse::printerr ("%sEMBED:%s  %s\n", B1, B0, embed);
    }

  if (embedding_pollfd.fd < 0)
    {
      Bse::printout ("%sLISTEN:%s %s\n", B1, B0, fullurl);
    }

  websocket_server.run();

  return 0;
}

/* Dumb echo test:
   var WebSocket = require ('ws'), c = 0; ws = new WebSocket("ws://localhost:27239/", 'auth123'); ws.onopen=e=>ws.send("Hello!");
   ws.onmessage=e=>{if(++c % 1000 == 0) console.log(e.data, c); ws.send("YO"); }; setTimeout(e=>{ws.close();console.log(c/10);},10000);
 */
