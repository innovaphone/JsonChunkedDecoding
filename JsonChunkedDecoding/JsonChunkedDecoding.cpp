
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/socket.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/json_api.h"
#include "common/ilib/str.h"
#include "common/ilib/json.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"
#include "common/lib/app_updates.h"

#include "JsonChunkedDecoding_tasks.h"
#include "JsonChunkedDecoding.h"

#define DBG(x) //debug->printf x

using namespace AppJsonChunkedDecoding;

/*-----------------------------------------------------------------------------------------------*/
/* class JsonChunkedDecodingService                                                              */
/*-----------------------------------------------------------------------------------------------*/

JsonChunkedDecodingService::JsonChunkedDecodingService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, class IWebserverPluginProvider * const webserverPluginProvider, class IDatabaseProvider * databaseProvider, AppServiceArgs * args) : AppService(iomux, localSocketProvider, args)
{
    this->iomux = iomux;
    this->localSocketProvider = localSocketProvider;
    this->webserverPluginProvider = webserverPluginProvider;
    this->databaseProvider = databaseProvider;
}

JsonChunkedDecodingService::~JsonChunkedDecodingService()
{

}

void JsonChunkedDecodingService::AppServiceApps(istd::list<AppServiceApp> * appList)
{
    appList->push_back(new AppServiceApp("inno-jsonchunkeddecoding"));
    appList->push_back(new AppServiceApp("inno-jsonchunkeddecodingadmin"));
}

void JsonChunkedDecodingService::AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList)
{
    pluginList->push_back(new AppInstancePlugin("inno.jsonchunkeddecodingmanager", "inno-jsonchunkeddecoding.png", "inno.jsonchunkeddecodingmanagertexts"));
}

class AppInstance * JsonChunkedDecodingService::CreateInstance(AppInstanceArgs * args)
{
    return new JsonChunkedDecoding(iomux, this, args);
}

/*-----------------------------------------------------------------------------------------------*/
/* class JsonChunkedDecoding                                                                     */
/*-----------------------------------------------------------------------------------------------*/

JsonChunkedDecoding::JsonChunkedDecoding(IIoMux * const iomux, class JsonChunkedDecodingService * service, AppInstanceArgs * args) : AppInstance(service, args), AppUpdates(iomux), ConfigContext(nullptr, this)
{
    this->stopping = false;
    this->iomux = iomux;
    this->service = service;
    this->webserverPlugin = service->webserverPluginProvider->CreateWebserverPlugin(iomux, service->localSocketProvider, this, args->webserver, args->webserverPath, this);
    this->logFlags |= LOG_APP;
    this->logFlags |= LOG_APP_WEBSOCKET;
	this->currentTask = nullptr;


    RegisterJsonApi(this);
    ConfigInitComplete();
    Log("App instance started");
}

JsonChunkedDecoding::~JsonChunkedDecoding()
{
}

void JsonChunkedDecoding::ConfigInitComplete()
{
    delete currentTask;
    currentTask = nullptr;
    webserverPlugin->HttpListen(nullptr, nullptr, nullptr, nullptr, _BUILD_STRING_);
    webserverPlugin->HttpListen("json", new class RestApi());                           // added this line
    webserverPlugin->WebsocketListen();
    Log("App instance initialized");
}

void JsonChunkedDecoding::JsonChunkedDecodingSessionClosed(class JsonChunkedDecodingSession * session)
{
    sessionList.remove(session);
    delete session;
    if (stopping) {
        TryStop();
    }
}

void JsonChunkedDecoding::ServerCertificateUpdate(const byte * cert, size_t certLen)
{
    Log("JsonChunkedDecoding::ServerCertificateUpdate cert:%x certLen:%u", cert, certLen);
}

void JsonChunkedDecoding::WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host)
{
    if (!this->stopping) {
        class JsonChunkedDecodingSession * session = new JsonChunkedDecodingSession(this);
        this->sessionList.push_back(session);
    }
    else {
        plugin->Cancel(wsr_cancel_type_t::WSP_CANCEL_UNAVAILABLE);
    }
}

void JsonChunkedDecoding::WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize)
{
    if (requestType == WS_REQUEST_GET) {
        if (plugin->BuildRedirect(resourceName, _BUILD_STRING_, strlen(_BUILD_STRING_))) {
            return;
        }
    }
    plugin->Cancel(WSP_CANCEL_NOT_FOUND);
}

void JsonChunkedDecoding::WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser)
{
    Log("WebserverPlugin closed");
    if (lastUser) {
        delete webserverPlugin;
        webserverPlugin = nullptr;
        TryStop();
    }
}

void JsonChunkedDecoding::Stop()
{
    TryStop();
}

void JsonChunkedDecoding::TryStop()
{
    if (!stopping) {
        Log("App instance stopping");
        stopping = true;
        for (std::list<JsonChunkedDecodingSession *>::iterator it = sessionList.begin(); it != sessionList.end(); ++it) {
            (*it)->Close();
        }
    }
    
    if (!webserverPlugin && sessionList.empty()) appService->AppStopped(this);
}
/*-----------------------------------------------------------------------------------------------*/
/* class JsonChunkedDecodingSession                                                              */
/*-----------------------------------------------------------------------------------------------*/

JsonChunkedDecodingSession::JsonChunkedDecodingSession(class JsonChunkedDecoding * instance) : AppUpdatesSession(instance, instance->webserverPlugin, instance, instance)
{
    this->instance = instance;

    this->currentTask = nullptr;
    this->currentSrc = nullptr;
    this->closed = false;
    this->closing = false;

    this->admin = false;
    this->adminApp = false;
}

JsonChunkedDecodingSession::~JsonChunkedDecodingSession()
{
}

void JsonChunkedDecodingSession::AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src)
{
    if (currentSrc) free(currentSrc);
    currentSrc = src ? _strdup(src) : nullptr;
    AppWebsocketMessageComplete();
}

void JsonChunkedDecodingSession::AppWebsocketAppInfo(const char * app, class json_io & msg, word base)
{

}

bool JsonChunkedDecodingSession::AppWebsocketConnectComplete(class json_io & msg, word info)
{
    const char * appobj = msg.get_string(info, "appobj");
    if (appobj && !strcmp(appobj, sip)) admin = true;
    if (!strcmp(app, "inno-jsonchunkeddecodingadmin")) adminApp = true;
    return true;
}

void JsonChunkedDecodingSession::AppWebsocketClosed()
{
    closed = true;
    if (currentTask && currentTask->complete) currentTask->Finished();
    TryClose();
}

void JsonChunkedDecodingSession::ResponseSent()
{
    if (currentTask) {
        currentTask->Finished();
    }
    else {
        AppWebsocketMessageComplete();
    }
}

void JsonChunkedDecodingSession::TryClose()
{
    closing = true;
    if (!closed) {
        AppWebsocketClose();
        return;
    }
    if (currentTask) {
        currentTask->Stop();
        return;
    }
    if (closed && !currentTask) {
        instance->JsonChunkedDecodingSessionClosed(this);
    }
}

void JsonChunkedDecodingSession::Close()
{
    TryClose();
}

bool JsonChunkedDecodingSession::CheckSession(class ITask * task)
{
    if (closing) {
        if (task) delete task;
        currentTask = nullptr;
        TryClose();
        return false;
    }
    return true;
}

/*-----------------------------------------------------------------------------------------------*/
/* class ResApi                                                                                  */
/*-----------------------------------------------------------------------------------------------*/

RestApi::RestApi()
{

}

RestApi::~RestApi()
{
}

void RestApi::WebserverPluginHttpListenResult(IWebserverPlugin* plugin, ws_request_type_t requestType, char* resourceName, const char* registeredPathForRequest, ulong64 dataSize)
{
    if (requestType == WS_REQUEST_PUT) {
        plugin->Accept(new RestApiPut(this));
    }
    else {
        plugin->Cancel(WSP_CANCEL_NOT_FOUND);
    }
}

/*-----------------------------------------------------------------------------------------------*/
/* class ResApiPut                                                                               */
/*-----------------------------------------------------------------------------------------------*/

RestApiPut::RestApiPut(class RestApi* rest)
{
    this->rest = rest;
    this->fill = 0;
    this->last = false;
}

RestApiPut::~RestApiPut()
{
}

void RestApiPut::WebserverPutRequestAcceptComplete(IWebserverPut* const webserverPut)
{
    // start receiving data
    webserverPut->Recv(data, DATA_SIZE_MAX);                        // Recive the first 256 data
}

void  RestApiPut::WebserverPutRecvResult(IWebserverPut* const webserverPut, void* buffer, size_t len)
{

    if (!buffer || !len) {
        last = true;
    }
    if (len) {
        // decode
        data[fill + len] = 0;           // Write the terminating 0 after the incomplete data length + the new data
        class json_io json(data);
        json.decode();
        json.dump();
        word root = json.get_object(JSON_ID_ROOT, 0);
        // get array of objects
        word a = json.get_array(root, "data");
        if (a != JSON_ID_NONE) {
            word base = JSON_ID_NONE;
            for (base = json.get_object(a, 0); base != JSON_ID_NONE && !(json.get_flags(base) & JSON_FLAG_INCOMPLETE); base = json.get_object(a, base)) {
                // get object properties
                const char* name = json.get_string(base, "name");
                // process the data
                // ...
                debug->printf("%s", name);

            }
            if (base != JSON_ID_NONE) {                         // if this condition isn´t true the end of the JSON has been reached
                char tmp[DATA_SIZE_MAX * 2 + 1];                  // temporary buffer to save the incomplete data
                char* x = tmp;                                  // pointer to temporary buffer
                debug->printf("Buffer before write: %s", tmp);
                json.write(0, x, a);                            // writes the incomplete data to the tmp buffer + the terminating 0
                debug->printf("Buffer after write: %s", tmp);
                fill = x - tmp;                                // calculate the length of the incomplete data by subtracting the 2 memory adresses, which is faster then strlen(tmp)
                debug->printf("Fill size: %d", fill);
                memcpy(data, tmp, fill + 1);                    // copy the incomplete data to the data buffer
            }
            else
                fill = 0;
        }
    }
    // Check if there is remaining data from the inital request, which may not be JSON data
    if (!last) {
        // append next chunk to the end of the buffer which contains the incomplete data
        webserverPut->Recv(&data[fill], DATA_SIZE_MAX);
    }
    else {
        // send 202 OK response with no content data
        webserverPut->SetResultCode(WEBDAV_RESULT_OK, 0);
        webserverPut->Send(0, 0);
    }
}

void  RestApiPut::WebserverPutRecvCanceled(IWebserverPut* const webserverPut, void* buffer)
{
}

void RestApiPut::WebserverPutSendResult(IWebserverPut* const webserverPut)
{
    // response is sent, close
    webserverPut->Close();
}

void RestApiPut::WebserverPutCloseComplete(IWebserverPut* const webserverPut)
{
    // clean-up
    delete webserverPut;
    delete this;
}
