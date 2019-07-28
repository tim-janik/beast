// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;

#include <jsonipc/jsonipc.hh>

#include <bse/platform.hh>
#include <bse/regex.hh>
#include "bse/jsonbindings.cc"
#include <bse/bse.hh>   // Bse::init_async

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
  {
    Aida::ScopedSemaphore sem;
    auto handle_wsmsg = [&sem] () {
      Bse::register_json_bindings();
      sem.post();
    };
    bse_server.__iface_ptr__()->__execution_context_mt__().enqueue_mt (handle_wsmsg);
    sem.wait();
  }

  // Setup Jsonipc dispatcher
  dispatcher = new Jsonipc::IpcDispatcher();
  dispatcher->add_method ("BeastSoundEngine/init_jsonipc",
                          [] (Jsonipc::JsonCallbackInfo &cbi) -> std::string* {
                            cbi.set_result (Jsonipc::to_json (Bse::ServerImpl::instance()).Move());
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
  Bse::printout ("%sLISTEN:%s ws://localhost:%d/\n", B1, B0, BEAST_AUDIO_ENGINE_PORT);

  websocket_server.run();

  return 0;
}

/* Dumb echo test:
   var WebSocket = require ('ws'), c = 0; ws = new WebSocket("ws://localhost:27239/", 'auth123'); ws.onopen=e=>ws.send("Hello!");
   ws.onmessage=e=>{if(++c % 1000 == 0) console.log(e.data, c); ws.send("YO"); }; setTimeout(e=>{ws.close();console.log(c/10);},10000);
 */
