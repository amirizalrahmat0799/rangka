#pragma once

#include <httplib.h>

#include "store.hpp"

/// Adds the /api routes to a server. Kept apart from main() so the tests can run the same routes.
void register_routes(httplib::Server& server, Store& store);
