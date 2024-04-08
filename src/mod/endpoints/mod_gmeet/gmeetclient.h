//
// Created by xiongqimin on 2023/12/19.
//

#ifndef GMEET_CLIENT_GMEETCLIENT_API_H
#define GMEET_CLIENT_GMEETCLIENT_API_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GMTC_STATIC
#define GMTC_C_EXPORT
#else // dynamic library
#ifdef _WIN32
#ifdef GMTC_EXPORTS
#define GMTC_C_EXPORT __declspec(dllexport) // building the library
#else
#define GMTC_C_EXPORT __declspec(dllimport) // using the library
#endif
#else // not WIN32
#define GMTC_C_EXPORT
#endif
#endif

#ifdef _WIN32
#ifdef CAPI_STDCALL
#define GMTC_API __stdcall
#else
#define GMTC_API
#endif
#else // not WIN32
#define GMTC_API
#endif

#define GMT_ERR_SUCCESS 0
#define GMT_ERR_INVALID -1   // invalid argument
#define GMT_ERR_FAILURE -2   // runtime error
#define GMT_ERR_NOT_AVAIL -3 // element not available
#define GMT_ERR_TOO_SMALL -4 // buffer too small

typedef void gmeet_client_t;
typedef void (*gmtcLogCallbackFunc)(int level, const char *message);

void printVersion();

/**
 * 加入 GMeet 会议
 * @param room_id  会议室ID
 * @param room_secret  会议室密码
 * @return
 */
gmeet_client_t *join_room(char *room_id, char *room_secret);


int connect(gmeet_client_t* client);
int exchange_sdp(gmeet_client_t *client, const char *sdp, char *answer, int size);
int leave_room(gmeet_client_t* client);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //GMEET_CLIENT_GMEETCLIENT_API_H
