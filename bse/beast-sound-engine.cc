// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "beast-sound-engine.hh"
#include "bsemain.hh"
#include "path.hh"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;

#include <limits.h>
#include <stdlib.h>

#undef B0 // undo pollution from termios.h

static Jsonipc::IpcDispatcher *dispatcher = NULL;
static bool verbose = false, verbose_binary = false;
static GPollFD embedding_pollfd = { -1, 0, 0 };
static std::string authenticated_subprotocol = "";

// Configure websocket server
struct CustomServerConfig : public websocketpp::config::asio {
  static const size_t connection_read_buffer_size = 16384;
};
using ServerEndpoint = websocketpp::server<CustomServerConfig>;
static ServerEndpoint websocket_server;

/// Check if two connection_hdl pointers are the same
static inline bool
websocketpp_connection_hdl_equals (const websocketpp::connection_hdl &a, const websocketpp::connection_hdl &b)
{
  // the connection_hdl is implemented as a weak_ptr, which provide no operator==().
  // but a and b can be ordered via owner_before, by means of comparing the offsets of the
  // weak_ptr control blocks. which means get_con_from_hdl() would yield the same connection_ptr.
  return !a.owner_before (b) && !b.owner_before (a);
}

// Provide websocket connection dependent InstanceMap
static std::vector<std::pair<websocketpp::connection_hdl, Jsonipc::InstanceMap*>> ws_instance_maps;

static std::string
handle_jsonipc (const std::string &message, const websocketpp::connection_hdl &hdl)
{
  ptrdiff_t conid = 0;
  if (verbose)
    {
      conid = ptrdiff_t (websocket_server.get_con_from_hdl (hdl).get());
      Bse::printerr ("%p: REQUEST: %s\n", conid, message);
    }
  Jsonipc::InstanceMap *imap = nullptr;
  for (const auto &pair : ws_instance_maps)
    if (websocketpp_connection_hdl_equals (pair.first, hdl))
      {
        imap = pair.second;
        break;
      }
  if (!imap)
    {
      imap = new Jsonipc::InstanceMap();
      ws_instance_maps.push_back (std::make_pair (hdl, imap));
    }
  Jsonipc::Scope message_scope (*imap);
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

static std::vector<ServerEndpoint::connection_ptr> ws_opened_connections;

static void
ws_open (websocketpp::connection_hdl hdl)
{
  ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (hdl);
  ws_opened_connections.push_back (con);
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

static void
ws_close (websocketpp::connection_hdl hdl)
{
  websocketpp::lib::error_code ec;
  ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (hdl, ec);
  if (ec)
    {
      Bse::printerr ("%s: failed to access connection: %s", __func__, ec.message());
      return;
    }
  using namespace Bse::AnsiColors;
  const auto B1 = color (BOLD);
  const auto B0 = color (BOLD_OFF);
  const ptrdiff_t conid = ptrdiff_t (con.get());
  auto it = std::find (ws_opened_connections.begin(), ws_opened_connections.end(), con);
  Bse::printout ("%p: %sCLOSED%s%s\n", conid, B1, B0,
                 it != ws_opened_connections.end() ? "" : " (unknown)");
  if (it != ws_opened_connections.end())
    ws_opened_connections.erase (it);
  for (auto it = ws_instance_maps.begin(); it != ws_instance_maps.end(); ++it)
    if (websocketpp_connection_hdl_equals (it->first, hdl))
      {
        Jsonipc::InstanceMap *imap = it->second;
        ws_instance_maps.erase (it);
        delete imap;
        break;
      }
}

static websocketpp::connection_hdl *bse_current_websocket_hdl = NULL;

static void
ws_message (websocketpp::connection_hdl hdl, server::message_ptr msg)
{
  const std::string &message = msg->get_payload();
  // send message to BSE thread and block until its been handled
  Bse::jobs += [&message, &hdl] () {
    bse_current_websocket_hdl = &hdl;
    std::string reply = handle_jsonipc (message, hdl);
    bse_current_websocket_hdl = NULL;
    if (!reply.empty())
      websocket_server.send (hdl, reply, websocketpp::frame::opcode::text);
  };
}

/// Provide an IPC handler implementation that marshals and sends binary data onto the wire.
struct IpcHandlerImpl : Bse::IpcHandler {
  virtual
  ~IpcHandlerImpl()
  {}
  virtual ptrdiff_t
  current_connection_id () override
  {
    if (bse_current_websocket_hdl)
      {
        websocketpp::lib::error_code ec;
        ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (*bse_current_websocket_hdl, ec);
        if (!ec) // not ec.value() in (websocketpp::error::bad_connection, ...)
          return ptrdiff_t (con.get());
      }
    return 0;
  }
  virtual BinarySender
  create_binary_sender () override
  {
    BSE_ASSERT_RETURN (bse_current_websocket_hdl != nullptr, BinarySender());
    websocketpp::connection_hdl weak_hdl = *bse_current_websocket_hdl;
    auto binary_sender = [weak_hdl] (std::string message) {
      websocketpp::lib::error_code ec;
      websocket_server.send (weak_hdl, message, websocketpp::frame::opcode::binary, ec);
      if (ec)
        return false;   // invalid connection or send failed
      if (verbose_binary)
        {
          ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (weak_hdl, ec);
          const ptrdiff_t conid = ptrdiff_t (con.get());
          Bse::printerr ("%p: BINARY:  len=%d hash=%016x\n", conid, message.size(), Bse::fnv1a_consthash64 (message.data(), message.size()));
          if (0)
            {
              std::string hex;
              for (size_t i = 0; i < message.size(); i++)
                {
                  if (i && 0 == i % 16)
                    hex += "\n ";
                  else if (0 == i % 8)
                    hex += " ";
                  hex += Bse::string_format (" %02x", message[i]);
                }
              Bse::printerr ("%s\n", hex);
            }
        }
      return true; // connection alive and message queued
    };
    return binary_sender;
  }
};

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
  static const char *apppath = realpath ((runpath (RPath::INSTALLDIR) + "/app/").c_str(), NULL);
  static size_t apppath_length = strlen (apppath ? apppath : "");
  static const char *docpath = realpath ((runpath (RPath::INSTALLDIR) + "/doc/").c_str(), NULL);
  static size_t docpath_length = strlen (docpath ? docpath : "");
  if (apppath && docpath)
    {
      char *uripath = realpath ((apppath + std::string ("/") + path).c_str(), NULL);
      std::string dest = uripath ? uripath : "";
      free (uripath);
      if ((dest.compare (0, apppath_length, apppath) == 0 && dest[apppath_length] == '/') ||
          (dest.compare (0, docpath_length, docpath) == 0 && dest[docpath_length] == '/'))
        return dest;
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
                     "<a href='/index.html'>index.html</a><br/>\n"
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
  if (!filepath.empty() && Path::check (filepath, "drx"))
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
      websocketpp::lib::error_code ec;
      ServerEndpoint::connection_ptr con = websocket_server.get_con_from_hdl (weak_hdl, ec);
      if (ec) // ec.value() == websocketpp::error::bad_connection
        {
          disconnect_id (handler_id);
          return; // `this` is deleted
        }
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
        const bool result = disconnect_id (handler_id);
        cbi.set_result (Jsonipc::to_json (result, cbi.allocator()).Move());
        return NULL;
      }
    return new std::string (cbi.invalid_params);
  }
  static bool
  disconnect_id (ssize_t handler_id)
  {
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
    return result;
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

static int
nonblock_fd (int fd)
{
  if (fd >= 0)
    {
      long r, d_long;
      do
        d_long = fcntl (fd, F_GETFL);
      while (d_long < 0 && errno == EINTR);
      d_long |= O_NONBLOCK;
      do
        r = fcntl (fd, F_SETFL, d_long);
      while (r < 0 && errno == EINTR);
    }
  return fd;
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
  Bse::printout ("  --binary     Print binary requests\n");
  Bse::printout ("  --js-bseapi  Print Javascript bindings for Bse\n");
  Bse::printout ("  --embed <fd> Parent process socket for embedding\n");
}

int
main (int argc, char *argv[])
{
  Bse::this_thread_set_name ("BeastSoundEngineMain");
  Bse::StringVector args = Bse::init_args (&argc, argv);
  Bse::init_async (argv[0], args);
  IpcHandlerImpl ipchandlerimpl;
  Bse::ServerImpl::instance().set_ipc_handler (&ipchandlerimpl);

  randomize_subprotocol();

  // Ensure Bse has everything properly loaded
  Bse::jobs += [] () {
    BSE_SERVER.load_assets();
  };

  // Register BSE bindings
  const bool print_jsbse = argc >= 2 && std::string ("--js-bseapi") == argv[1];
  Bse::jobs += [print_jsbse] () {
    if (print_jsbse)
      Jsonipc::ClassPrinter::recording (true);
    bse_jsonipc_stub1();
    bse_jsonipc_stub2();
    bse_jsonipc_stub3();
    bse_jsonipc_stub4();
    if (print_jsbse)
      Bse::printout ("%s\n", Jsonipc::ClassPrinter::to_string());
    // fixups, we know Bse::Server is a singleton
    Jsonipc::Class<Bse::ServerIface> jsonipc__Bse_ServerIface;
    jsonipc__Bse_ServerIface.eternal();
    Jsonipc::Class<Bse::ServerImpl> jsonipc__Bse_ServerImpl;
    jsonipc__Bse_ServerImpl.eternal();
  };
  if (print_jsbse)
    return 0;

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
        else if (arg_name == "binary")
          {
            verbose_binary = true;
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
      nonblock_fd (embedding_pollfd.fd);
      Bse::jobs += [] () {
        embedding_pollfd.events = G_IO_IN | G_IO_HUP; // G_IO_PRI | (G_IO_IN | G_IO_HUP) | (G_IO_OUT | G_IO_ERR);
        GSource *source = g_source_simple (BSE_PRIORITY_NORMAL,
                                           [] (void*, int *timeoutp) -> int { // pending
                                             return embedding_pollfd.events && embedding_pollfd.revents;
                                           },
                                           [] (void*) { // dispatch,
                                             if (embedding_pollfd.revents & G_IO_IN)
                                               {
                                                 static char b[512];
                                                 ssize_t n = read (embedding_pollfd.fd, b, 512); // clear input
                                                 (void) n;
                                               }
                                             if (embedding_pollfd.revents & G_IO_HUP)
                                               {
                                                 // stop polling, fd possibly closed
                                                 embedding_pollfd.events = 0;
                                                 // stop open asio connections
                                                 for (auto ri = ws_opened_connections.rbegin(); ri != ws_opened_connections.rend(); ri++)
                                                   {
                                                     const ptrdiff_t conid = ptrdiff_t (ri->get());
                                                     websocketpp::lib::error_code ec;
                                                     (*ri)->close (websocketpp::close::status::going_away, "", ec); // omit_handshake
                                                     if (ec)
                                                       Bse::printerr ("%p: CLOSE:   %s\n", conid, ec.message());
                                                   }
                                                 // stop listening, so asio::run() can stop
                                                 if (websocket_server.is_listening())
                                                   websocket_server.stop_listening();
                                               }
                                           },
                                           nullptr, // data,
                                           nullptr, // destroy,
                                           &embedding_pollfd, nullptr);
        g_source_attach (source, bse_main_context);
      };
    }

  const int BEAST_AUDIO_ENGINE_PORT = 27239;    // 0x3ea67 % 32768

  // setup websocket and run asio loop
  websocket_server.set_user_agent ("beast-sound-engine/" + Bse::version());
  websocket_server.set_http_handler (&http_request);
  websocket_server.set_validate_handler (&ws_validate_connection);
  websocket_server.set_open_handler (&ws_open);
  websocket_server.set_close_handler (&ws_close);
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
  std::string fullurl = Bse::string_format ("http://127.0.0.1:%d/index.html", localhost_port);
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
