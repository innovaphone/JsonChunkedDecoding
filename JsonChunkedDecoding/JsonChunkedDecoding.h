
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

namespace AppJsonChunkedDecoding {

    class JsonChunkedDecodingService : public AppService {
        class AppInstance * CreateInstance(AppInstanceArgs * args) override;
        void AppServiceApps(istd::list<AppServiceApp> * appList) override;
        void AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList) override;

    public:
        JsonChunkedDecodingService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, IWebserverPluginProvider * const webserverPluginProvider, IDatabaseProvider * databaseProvider, AppServiceArgs * args);
        ~JsonChunkedDecodingService();

        class IIoMux * iomux;
        class ISocketProvider * localSocketProvider;
        class IWebserverPluginProvider * webserverPluginProvider;
        class IDatabaseProvider * databaseProvider;
    };

    class JsonChunkedDecoding : public AppInstance, public AppUpdates, public UWebserverPlugin, public JsonApiContext, public ConfigContext
    {
        void WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser) override;
        void WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host) override;
        void WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize) override;

        void ServerCertificateUpdate(const byte * cert, size_t certLen) override;
        void Stop() override;

        void TryStop();

        bool stopping;
        class ITask * currentTask;
        std::list<class JsonChunkedDecodingSession *> sessionList;

    public:
        JsonChunkedDecoding(IIoMux * const iomux, JsonChunkedDecodingService * service, AppInstanceArgs * args);
        ~JsonChunkedDecoding();

        void ConfigInitComplete();
        void JsonChunkedDecodingSessionClosed(class JsonChunkedDecodingSession * session);

        const char * appPwd() { return args.appPassword; };

        class IIoMux * iomux;
        class JsonChunkedDecodingService * service;
        class IWebserverPlugin * webserverPlugin;

    };

    class ConfigInit : public ITask, public UTask {
        class JsonChunkedDecoding * instance;
        class ITask * task;
        void Start(class UTask * user) override {};
        void TaskComplete(class ITask * const task) override { instance->ConfigInitComplete(); };
        void TaskFailed(class ITask * const task) override {};
    public:
        ConfigInit(class JsonChunkedDecoding * instance, class IDatabase * database) {
            this->instance = instance;
            task = instance->CreateInitTask(database);
            task->Start(this);
        }
    };

    class JsonChunkedDecodingSession : public AppUpdatesSession {
        void AppWebsocketAccept(class UWebsocket * uwebsocket) { instance->webserverPlugin->WebsocketAccept(uwebsocket); };
        char * AppWebsocketPassword() override { return (char *)instance->appPwd(); };
        void AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src) override;
        void AppWebsocketAppInfo(const char * app, class json_io & msg, word base) override;
        bool AppWebsocketConnectComplete(class json_io & msg, word info) override;
        void AppWebsocketClosed() override;

        void ResponseSent() override;

        void TryClose();

        bool closed;
        bool closing;
        bool admin;
        bool adminApp;

    public:
        JsonChunkedDecodingSession(class JsonChunkedDecoding * instance);
        ~JsonChunkedDecodingSession();

        bool CheckSession(class ITask * task);

        class JsonChunkedDecoding * instance;
        char * currentSrc;
        class ITask * currentTask;
        void Close();
    };

    class RestApi : public UWebserverPlugin {
    public:
        RestApi();
        ~RestApi();
        void WebserverPluginHttpListenResult(IWebserverPlugin* plugin, ws_request_type_t requestType, char* resourceName, const char* registeredPathForRequest, ulong64 dataSize) override;

    };

#define DATA_SIZE_MAX 256

    class RestApiPut : public UWebserverPut {
    public:
        RestApiPut(class RestApi* rest);
        ~RestApiPut();
        void WebserverPutRequestAcceptComplete(IWebserverPut* const webserverPut) override;
        void WebserverPutRecvResult(IWebserverPut* const webserverPut, void* buffer, size_t len) override;
        void WebserverPutRecvCanceled(IWebserverPut* const webserverPut, void* buffer) override;
        void WebserverPutSendResult(IWebserverPut* const webserverPut) override;
        void WebserverPutCloseComplete(IWebserverPut* const webserverPut) override;

        class RestApi* rest;
        char data[DATA_SIZE_MAX * 2 + 1];
        size_t fill;
        bool last;

    };

}


