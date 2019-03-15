# API Reference

## Namespace Bse

Members declared within the `Bse` namespace.

### Bse::init_async()

~~~cc
// namespace Bse
void
init_async (int *argc,
            char **argv,
            const char *app_name,
            const StringVector &args);
~~~

Initialize and start BSE.
Initialize the BSE library and start the main BSE thread.
Arguments specific to BSE are removed from `argc` / `argv`.
