// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <bse/bse.hh>
#include <bse/platform.hh>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;


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
}

static Bse::ServerH bse_server;

// Configure websocket server
struct CustomServerConfig : public websocketpp::config::asio {
  static const size_t connection_read_buffer_size = 16384;
};
using ServerEndpoint = websocketpp::server<CustomServerConfig>;
static ServerEndpoint websocket_server;

static void
ws_message (websocketpp::connection_hdl con, server::message_ptr msg)
{
  const std::string &message = msg->get_payload();
  // send message to BSE thread and block until its been handled
  Aida::ScopedSemaphore sem;
  auto handle_wsmsg = [&message, &con, &sem] () {
    const std::string reply = "ECHO: " + message;
    if (!reply.empty())
      websocket_server.send (con, reply, websocketpp::frame::opcode::text);
    sem.post();
  };
  bse_server.__iface_ptr__()->__execution_context_mt__().enqueue_mt (handle_wsmsg);
  sem.wait();
}

int
main (int argc, char *argv[])
{
  Bse::init_async (&argc, argv, argv[0]); // Bse::cstrings_to_vector (NULL)
  bse_server = Bse::init_server_instance();

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
  websocket_server.set_message_handler (&ws_message);
  websocket_server.init_asio();
  websocket_server.clear_access_channels (websocketpp::log::alevel::all);
  websocket_server.set_reuse_addr (true);
  namespace IP = boost::asio::ip;
  IP::tcp::endpoint endpoint_local = IP::tcp::endpoint (IP::address::from_string ("127.0.0.1"), BEAST_AUDIO_ENGINE_PORT);
  websocket_server.listen (endpoint_local);
  websocket_server.start_accept();

#undef B0 // pollution from termios.h
  using namespace Bse::AnsiColors;
  auto B1 = color (BOLD);
  auto B0 = color (BOLD_OFF);
  Bse::printout ("%sLISTEN:%s ws://localhost:%d/\n", B1, B0, BEAST_AUDIO_ENGINE_PORT);

  websocket_server.run();

  return 0;
}
