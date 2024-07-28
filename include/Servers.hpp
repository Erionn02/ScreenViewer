#pragma once

#include "ScreenViewerSessionsServer.hpp"
#include "AuthenticatedServerSideClientSession.hpp"
#include "ProxySession.hpp"
#include "UsersManager.hpp"

using SessionsServer = ScreenViewerSessionsServer<AuthenticatedServerSideClientSession, UsersManager>;
using ProxyServer = ScreenViewerSessionsServer<ProxySession, UsersManager>;