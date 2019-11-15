// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "jsonipc.hh"
#include <iostream>

// == Testing ==
enum ErrorType {
  NONE,
  INVALID,
  FATAL
};

struct Copyable {
  int i = 111;
  float f = -0.05;
  std::string hello = "hello";
};

struct Base {
  Base() = default;
  Base(const Base&) = default;
  virtual ~Base() {}
};
struct Base2 {
  Base2() = default;
  Base2(const Base2&) = default;
  virtual ~Base2() {}
  Copyable randomize()  { Copyable c; c.i = rand(); c.f = rand() / 10.0; return c; }
};
struct Derived : Base, Base2 {
  const std::string name_;
  Derived (const std::string &name) : name_ (name) {}
  void   dummy0 ()                                { printf ("dummy0: NOP\n"); }
  bool   dummy1 (bool b)                          { printf ("dummy1: b=%s\n", b ? "true" : "false"); return b; }
  void   dummy2 (std::string s, int i)            { printf ("dummy2: s=\"%s\" i=%d\n", s.c_str(), i); }
  size_t dummy3 (Derived &d) const                { printf ("dummy3: Derived=%p this=%p\n", &d, this); return size_t (&d); }
  bool   dummy4 (float f, std::string s, long l)  { printf ("dummy4: this=%s %f '%s' %ld\n", name_.c_str(), f, s.c_str(), l); return 1; }
  void   dummy5 (long l, const char *c, double d) { printf ("dummy5: this=%s %ld '%s' %f\n", name_.c_str(), l, c, d); }
  std::string dummy6 (int, const std::string &)   { return ""; }
  Derived* dummy7 () { return NULL; }
  Derived& dummy8 () { return *dummy7(); }
  Derived  dummy9 () { return dummy8(); }
};

static size_t
json_objectid (const Jsonipc::JsonValue &value)
{
  if (value.IsObject())
    {
      auto it = value.FindMember ("$id");
      if (it != value.MemberEnd())
        return Jsonipc::from_json<size_t> (it->value);
    }
  return 0;
}

template<typename R> R
parse_result (size_t id, const std::string json_reply)
{
  rapidjson::Document document;
  document.Parse (json_reply.data(), json_reply.size());
  if (!document.HasParseError())
    {
      size_t id_ = 0;
      const Jsonipc::JsonValue *result = NULL;
      for (const auto &m : document.GetObject())
        if (m.name == "id")
          id_ = Jsonipc::from_json<size_t> (m.value, 0);
        else if (m.name == "result")
          result = &m.value;
      if (id_ == id && result)
        return Jsonipc::from_json<R> (*result);
    }
  return R();
}

static void
test_jsonipc (bool dispatcher_shell = false)
{
  using namespace Jsonipc;
  rapidjson::Document doc;
  auto &a = doc.GetAllocator();

  // test basics
  JSONIPC_ASSERT_RETURN (false == from_json<bool> (JsonValue()));
  JSONIPC_ASSERT_RETURN (true == from_json<bool> (JsonValue (true)));
  JSONIPC_ASSERT_RETURN (true == from_json<bool> (JsonValue(), true));
  JSONIPC_ASSERT_RETURN (false == from_json<bool> (JsonValue(), false));
  JSONIPC_ASSERT_RETURN (from_json<bool> (to_json (true, a)) == true);
  JSONIPC_ASSERT_RETURN (from_json<bool> (to_json (false, a)) == false);
  JSONIPC_ASSERT_RETURN (from_json<size_t> (to_json (1337, a)) == 1337);
  JSONIPC_ASSERT_RETURN (from_json<ssize_t> (to_json (-1337, a)) == -1337);
  JSONIPC_ASSERT_RETURN (from_json<float> (to_json (-0.5, a)) == -0.5);
  JSONIPC_ASSERT_RETURN (from_json<double> (to_json (1e20, a)) == 1e20);
  JSONIPC_ASSERT_RETURN (from_json<const char*> (to_json ("Om", a)) == std::string ("Om"));
  JSONIPC_ASSERT_RETURN (from_json<std::string> (to_json (std::string ("Ah"), a)) == "Ah");
  JSONIPC_ASSERT_RETURN (strcmp ("HUM", from_json<const char*> (to_json ((const char*) "HUM", a))) == 0);

  // register test classes and methods
  Jsonipc::Enum<ErrorType> enum_ErrorType ("Error");
  enum_ErrorType
    .set (ErrorType::NONE, "NONE")
    .set (ErrorType::INVALID, "INVALID")
    .set (ErrorType::FATAL, "FATAL")
    ;

  Jsonipc::Class<Base> class_Base;
  Jsonipc::Class<Base2> class_Base2;
  Jsonipc::Serializable<Copyable> class_Copyable;
  class_Copyable
    // .copy ([] (const Copyable &o) { return std::make_shared<std::decay<decltype (o)>::type> (o); })
    .set ("i", &Copyable::i)
    .set ("f", &Copyable::f)
    .set ("hello", &Copyable::hello)
    ;
  Jsonipc::Class<Derived> class_Derived;
  class_Derived
    .inherit<Base>()
    .inherit<Base2>()
    .eternal()  // MUST unregister thisid before objects go out of scope
    .set ("dummy0", &Derived::dummy0)
    .set ("dummy1", &Derived::dummy1)
    .set ("dummy2", &Derived::dummy2)
    .set ("dummy3", &Derived::dummy3)
    .set ("dummy4", &Derived::dummy4)
    .set ("dummy5", &Derived::dummy5)
    .set ("dummy6", &Derived::dummy6)
    .set ("dummy7", &Derived::dummy7)
    .set ("dummy8", &Derived::dummy8)
    .set ("dummy9", &Derived::dummy9)
    .set ("randomize", &Derived::randomize)
    ;

  // Provide scope and instance ownership during dispatch_message()
  InstanceMap imap;
  Scope temporary_scope (imap); // needed by to_/from_json and dispatcher

  // test bindings
  Derived obja ("obja");
  JSONIPC_ASSERT_RETURN (to_json (obja, a) == to_json (obja, a));
  JSONIPC_ASSERT_RETURN (&obja == from_json<Derived*> (to_json (obja, a)));
  JSONIPC_ASSERT_RETURN (ptrdiff_t (&static_cast<Base&> (obja)) == ptrdiff_t (&obja));
  JSONIPC_ASSERT_RETURN (ptrdiff_t (&static_cast<Base2&> (obja)) > ptrdiff_t (&obja));
  // given the same id, Base and Base2 need to unwrap to different addresses due to multiple inheritance
  JSONIPC_ASSERT_RETURN (&obja == from_json<Base*> (to_json (obja, a)));
  JSONIPC_ASSERT_RETURN (from_json<Base2*> (to_json (obja, a)) == &static_cast<Base2&> (obja));
  Derived objb ("objb");
  JSONIPC_ASSERT_RETURN (&obja != &objb);
  // const size_t idb = class_Derived.wrap_object (objb);
  JSONIPC_ASSERT_RETURN (from_json<std::shared_ptr<Derived>> (to_json (objb, a)).get() == &objb);
  JSONIPC_ASSERT_RETURN (ptrdiff_t (&static_cast<Base&> (objb)) == ptrdiff_t (&objb));
  JSONIPC_ASSERT_RETURN (ptrdiff_t (&static_cast<Base2&> (objb)) > ptrdiff_t (&objb));
  JSONIPC_ASSERT_RETURN (from_json<std::shared_ptr<Base>> (to_json (objb, a)).get() == &static_cast<Base&> (objb));
  JSONIPC_ASSERT_RETURN (from_json<std::shared_ptr<Base2>> (to_json (objb, a)).get() == &static_cast<Base2&> (objb));
  JSONIPC_ASSERT_RETURN (to_json (obja, a) != to_json (objb, a));
  Derived objc ("objc");
  JSONIPC_ASSERT_RETURN (&objc == &from_json<Derived&> (to_json (objc, a)));
  JSONIPC_ASSERT_RETURN (&objc == &from_json<Base&> (to_json (objc, a)));
  JSONIPC_ASSERT_RETURN (&objc == &from_json<Base2&> (to_json (objc, a)));
  JSONIPC_ASSERT_RETURN (to_json (objb, a) != to_json (objc, a));
  JSONIPC_ASSERT_RETURN (to_json (objc, a) == to_json (objc, a));
  const JsonValue jva = to_json (obja, a);
  const JsonValue jvb = to_json (objb, a);
  const JsonValue jvc = to_json (objc, a);
  JSONIPC_ASSERT_RETURN (from_json<Derived*> (jva) == &obja);
  JSONIPC_ASSERT_RETURN (&from_json<Derived> (jvb) == &objb);
  JSONIPC_ASSERT_RETURN (&from_json<Derived&> (jvc) == &objc);

  // Serializable tests
  std::string result;
  Copyable c1 { 2345, -0.5, "ehlo" };
  JsonValue jvc1 = to_json (c1, a);
  Copyable c2 = from_json<Copyable&> (jvc1);
  JSONIPC_ASSERT_RETURN (c1.i == c2.i && c1.f == c2.f && c1.hello == c2.hello);

  // dispatcher tests
  IpcDispatcher dispatcher;
  Derived d1 ("dood");
  JsonValue jvd1 = to_json (d1, a);
  const size_t d1id = json_objectid (jvd1);
  JSONIPC_ASSERT_RETURN (d1id == 4); // used in the next few lines
  result = dispatcher.dispatch_message (R"( {"id":123,"method":"randomize","params":[{"$id":4}]} )");
  const Copyable c0;
  const Copyable *c3 = parse_result<Copyable*> (123, result);
  JSONIPC_ASSERT_RETURN (c3 && (c3->i != c0.i || c3->f != c0.f));
  result = dispatcher.dispatch_message (R"( {"id":444,"method":"randomize","params":[{"$id":4}]} )");
  const Copyable *c4 = parse_result<Copyable*> (444, result);
  JSONIPC_ASSERT_RETURN (c4 && (c4->i != c3->i || c4->f != c3->f));
  result = dispatcher.dispatch_message (R"( {"id":111,"method":"randomize","params":[{"$id":4}]} )");
  const Copyable *c5 = parse_result<Copyable*> (111, result);
  JSONIPC_ASSERT_RETURN (c5 && (c5->i != c4->i || c5->f != c4->f));

  // CLI test server
  if (dispatcher_shell)
    {
      for (std::string line; std::getline (std::cin, line); )
        std::cout << dispatcher.dispatch_message (line) << std::endl;
      // Feed example lines:
      // {"id":123,"method":"dummy3","params":[2],"this":1}
    }

  // unregister thisids for objects living on the stack
  forget_json_id (json_objectid (jva));
  JSONIPC_ASSERT_RETURN (from_json<Derived*> (jva) == nullptr);
  forget_json_id (json_objectid (jvb));
  JSONIPC_ASSERT_RETURN (from_json<Derived*> (jvb) == (Derived*) nullptr);
  forget_json_id (json_objectid (jvc));
  JSONIPC_ASSERT_RETURN (from_json<Derived*> (jvc) == (Derived*) nullptr);
}

#ifdef STANDALONE
int
main (int argc, char *argv[])
{
  const bool dispatcher_shell = argc > 1 && 0 == strcmp (argv[1], "--shell");
  test_jsonipc (dispatcher_shell);
  printf ("  OK       %s\n", argv[0]);
  return 0;
}
#endif
