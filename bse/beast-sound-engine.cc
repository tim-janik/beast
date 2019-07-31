// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;

#include <jsonipc/jsonipc.hh>

#include <bse/bseenums.hh>      // enums API interfaces, etc
#include <bse/platform.hh>
#include <bse/regex.hh>
#include <bse/bse.hh>   // Bse::init_async
#include <limits.h>
#include <stdlib.h>

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

#undef B0 // pollution from termios.h

static Bse::ServerH bse_server;
static Jsonipc::IpcDispatcher *dispatcher = NULL;
static bool verbose = false;

// Configure websocket server
struct CustomServerConfig : public websocketpp::config::asio {
  static const size_t connection_read_buffer_size = 16384;
};
using ServerEndpoint = websocketpp::server<CustomServerConfig>;
static ServerEndpoint websocket_server;

static std::string
handle_jsonipc (const std::string &message, const websocketpp::connection_hdl &hdl)
{
  const ptrdiff_t conid = ptrdiff_t (websocket_server.get_con_from_hdl (hdl).get());
  if (verbose)
    Bse::printerr ("%p: REQUEST: %s\n", conid, message);
  const std::string reply = dispatcher->dispatch_message (message);
  if (verbose)
    Bse::printerr ("%p: REPLY:   %s\n", conid, reply);
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
  if (subprotocols.size() == 1)
    {
      if (subprotocols[0] == "auth123")
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

static void
ws_message (websocketpp::connection_hdl con, server::message_ptr msg)
{
  const std::string &message = msg->get_payload();
  // send message to BSE thread and block until its been handled
  Aida::ScopedSemaphore sem;
  auto handle_wsmsg = [&message, &con, &sem] () {
    std::string reply = handle_jsonipc (message, con);
    if (!reply.empty())
      websocket_server.send (con, reply, websocketpp::frame::opcode::text);
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
      if (string_endswith (filepath, ".html"))
        con->append_header ("Content-Type", "text/html; charset=utf-8");
      else if (string_endswith (filepath, ".js") || string_endswith (filepath, ".mjs"))
        con->append_header ("Content-Type", "application/javascript");
      else if (string_endswith (filepath, ".css"))
        con->append_header ("Content-Type", "text/css");
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

static void
print_usage (bool help)
{
  if (!help)
    {
      Bse::printout ("beast-sound-engine version %s\n", Bse::version());
      return;
    }
  Bse::printout ("Usage: beast-sound-engine [OPTIONS]\n");
  Bse::printout ("  --help     Print command line help\n");
  Bse::printout ("  --version  Print program version\n");
  Bse::printout ("  --verbose  Print requests and replies\n");
}

int
main (int argc, char *argv[])
{
  Bse::init_async (&argc, argv, argv[0]); // Bse::cstrings_to_vector (NULL)
  bse_server = Bse::init_server_instance();

  // Register BSE bindings
  const bool print_jsbse = argc >= 2 && std::string ("--js-bseapi") == argv[1];
  {
    Aida::ScopedSemaphore sem;
    auto handle_wsmsg = [print_jsbse,&sem] () {
      if (!print_jsbse)
        Jsonipc::ClassPrinter::disable();
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
                          [] (Jsonipc::JsonCallbackInfo &cbi) -> std::string* {
                            // TODO: bind Bse::ServerImpl, so far we only bind Bse::ServerIface
                            Bse::ServerIface &server_iface = Bse::ServerImpl::instance();
                            cbi.set_result (Jsonipc::to_json (server_iface, cbi.allocator()).Move());
                            return NULL;
                          });

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
  IP::tcp::endpoint endpoint_local = IP::tcp::endpoint (IP::address::from_string ("127.0.0.1"), BEAST_AUDIO_ENGINE_PORT);
  websocket_server.listen (endpoint_local);
  websocket_server.start_accept();

  using namespace Bse::AnsiColors;
  auto B1 = color (BOLD);
  auto B0 = color (BOLD_OFF);
  Bse::printout ("%sLISTEN:%s http://localhost:%d/app.html\n", B1, B0, BEAST_AUDIO_ENGINE_PORT);

  websocket_server.run();

  return 0;
}

/* Dumb echo test:
   var WebSocket = require ('ws'), c = 0; ws = new WebSocket("ws://localhost:27239/", 'auth123'); ws.onopen=e=>ws.send("Hello!");
   ws.onmessage=e=>{if(++c % 1000 == 0) console.log(e.data, c); ws.send("YO"); }; setTimeout(e=>{ws.close();console.log(c/10);},10000);
 */
