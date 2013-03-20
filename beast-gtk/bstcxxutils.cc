// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstcxxutils.hh"
#include "bstusermessage.hh"
#include <birnet/birnet.hh>
using namespace Birnet;
static void
bstcxx_message_handler (const char              *domain,
                        Msg::Type                mtype,
                        const vector<Msg::Part> &parts)
{
  BIRNET_STATIC_ASSERT (BSE_MSG_NONE    == (int) BST_MSG_NONE);
  BIRNET_STATIC_ASSERT (BSE_MSG_ALWAYS  == (int) BST_MSG_ALWAYS);
  BIRNET_STATIC_ASSERT (BSE_MSG_ERROR   == (int) BST_MSG_ERROR);
  BIRNET_STATIC_ASSERT (BSE_MSG_WARNING == (int) BST_MSG_WARNING);
  BIRNET_STATIC_ASSERT (BSE_MSG_SCRIPT  == (int) BST_MSG_SCRIPT);
  BIRNET_STATIC_ASSERT (BSE_MSG_INFO    == (int) BST_MSG_INFO);
  BIRNET_STATIC_ASSERT (BSE_MSG_DIAG    == (int) BST_MSG_DIAG);
  BIRNET_STATIC_ASSERT (BSE_MSG_DEBUG   == (int) BST_MSG_DEBUG);
  String title, primary, secondary, details, checkmsg;
  for (uint i = 0; i < parts.size(); i++)
    switch (parts[i].ptype)
      {
      case '0': title     += (title.size()     ? "\n" : "") + parts[i].string; break;
      case '1': primary   += (primary.size()   ? "\n" : "") + parts[i].string; break;
      case '2': secondary += (secondary.size() ? "\n" : "") + parts[i].string; break;
      case '3': details   += (details.size()   ? "\n" : "") + parts[i].string; break;
      case 'c': checkmsg  += (checkmsg.size()  ? "\n" : "") + parts[i].string; break;
      }
  BstMessage msg = { 0, };
  msg.log_domain = domain;
  msg.type = BstMsgType (mtype);
  msg.ident = Msg::type_ident (mtype);
  msg.label = Msg::type_label (mtype);
  msg.title = title.c_str();
  msg.primary = primary.c_str();
  msg.secondary = secondary.c_str();
  msg.details = details.c_str();
  msg.config_check = checkmsg.c_str();
  msg.janitor = bse_script_janitor();
  msg.process = g_strdup (Rapicorn::ThisThread::name().c_str());
  msg.pid = Rapicorn::ThisThread::thread_pid();
  msg.n_msg_bits = 0;
  msg.msg_bits = NULL;
  bst_message_handler (&msg);
}
extern "C" void
bst_message_handler_install (void)
{
  Msg::set_thread_handler (bstcxx_message_handler);
}
extern "C" void
bst_message_handler_uninstall (void)
{
  Msg::set_thread_handler (NULL);
}
