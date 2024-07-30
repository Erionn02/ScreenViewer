#pragma once

#include "ScreenViewerSessionsServer.hpp"
#include "AuthenticatedSession.hpp"
#include "ProxySession.hpp"
#include "UsersManager.hpp"

using SessionsServer = ScreenViewerSessionsServer<AuthenticatedSession, UsersManager>;
using ProxyServer = ScreenViewerSessionsServer<ProxySession, UsersManager>;