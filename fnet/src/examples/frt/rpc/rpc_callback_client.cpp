// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fnet/frt/supervisor.h>
#include <vespa/fnet/frt/target.h>
#include <vespa/fnet/frt/rpcrequest.h>
#include <vespa/fastos/app.h>

#include <vespa/log/log.h>
LOG_SETUP("rpc_callback_client");

struct RPC : public FRT_Invokable
{
    uint32_t invokeCnt;
    RPC() : invokeCnt(0) {}
    void Prod(FRT_RPCRequest *req);
    void Init(FRT_Supervisor *s);
};

void
RPC::Prod(FRT_RPCRequest *req)
{
    (void) req;
    ++invokeCnt;
}

void
RPC::Init(FRT_Supervisor *s)
{
    FRT_ReflectionBuilder rb(s);
    //-------------------------------------------------------------------
    rb.DefineMethod("prod", "", "",
                    FRT_METHOD(RPC::Prod), this);
    //-------------------------------------------------------------------
}


class MyApp : public FastOS_Application
{
public:
    int Main() override;
};

int
MyApp::Main()
{
    if (_argc < 2) {
        printf("usage  : rpc_server <connectspec>\n");
        return 1;
    }
    bool ok = true;
    RPC rpc;
    fnet::frt::StandaloneFRT server;
    FRT_Supervisor & orb = server.supervisor();
    rpc.Init(&orb);

    FRT_Target *target = orb.Get2WayTarget(_argv[1]);
    FRT_RPCRequest *req = orb.AllocRPCRequest();

    printf("invokeCnt: %d\n", rpc.invokeCnt);

    req->SetMethodName("callBack");
    req->GetParams()->AddString("prod");
    target->InvokeSync(req, 10.0);

    if(req->IsError()) {
        printf("[error(%d): %s]\n",
               req->GetErrorCode(),
               req->GetErrorMessage());
        ok = false;
    }

    printf("invokeCnt: %d\n", rpc.invokeCnt);

    req = orb.AllocRPCRequest(req);
    req->SetMethodName("callBack");
    req->GetParams()->AddString("prod");
    target->InvokeSync(req, 10.0);

    if(req->IsError()) {
        printf("[error(%d): %s]\n",
               req->GetErrorCode(),
               req->GetErrorMessage());
        ok = false;
    }

    printf("invokeCnt: %d\n", rpc.invokeCnt);

    req = orb.AllocRPCRequest(req);
    req->SetMethodName("callBack");
    req->GetParams()->AddString("prod");
    target->InvokeSync(req, 10.0);

    if(req->IsError()) {
        printf("[error(%d): %s]\n",
               req->GetErrorCode(),
               req->GetErrorMessage());
        ok = false;
    }

    printf("invokeCnt: %d\n", rpc.invokeCnt);
    if (rpc.invokeCnt != 3) {
        ok = false;
    }

    req->SubRef();
    target->SubRef();
    return ok ? 0 : 1;
}


int
main(int argc, char **argv)
{
    MyApp myapp;
    return myapp.Entry(argc, argv);
}
