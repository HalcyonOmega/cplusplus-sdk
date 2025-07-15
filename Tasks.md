# Tasks

## Primary

- [ ] Add Elicitation Client-Side feature

# Fetch and link Poco

include(FetchContent)
FetchContent_Declare(
Poco
GIT_REPOSITORY https://github.com/pocoproject/poco.git
GIT_TAG poco-1.14.2-release
)

set(ENABLE_XML OFF CACHE BOOL "Disable Poco XML" FORCE)
set(ENABLE_JSON OFF CACHE BOOL "Disable Poco JSON" FORCE)
set(ENABLE_DATA OFF CACHE BOOL "Disable Poco Data" FORCE)
set(ENABLE_DATA_SQLITE OFF CACHE BOOL "Disable Poco SQLite" FORCE)
set(ENABLE_DATA_ODBC OFF CACHE BOOL "Disable Poco ODBC" FORCE)
set(ENABLE_DATA_MYSQL OFF CACHE BOOL "Disable Poco MySQL" FORCE)
set(ENABLE_CRYPTO OFF CACHE BOOL "Disable Poco Crypto" FORCE)
set(ENABLE_NETSSL OFF CACHE BOOL "Disable Poco NetSSL" FORCE)
set(ENABLE_UTIL OFF CACHE BOOL "Disable Poco Util" FORCE)
set(ENABLE_JWT OFF CACHE BOOL "Disable Poco JWT" FORCE)
set(ENABLE_MONGODB OFF CACHE BOOL "Disable Poco MongoDB" FORCE)
set(ENABLE_REDIS OFF CACHE BOOL "Disable Poco Redis" FORCE)
set(ENABLE_ZIP OFF CACHE BOOL "Disable Poco Zip" FORCE)
set(ENABLE_PROMETHEUS OFF CACHE BOOL "Disable Poco Prometheus" FORCE)

FetchContent_MakeAvailable(Poco)