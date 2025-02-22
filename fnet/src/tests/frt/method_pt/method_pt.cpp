// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <vespa/fnet/frt/supervisor.h>
#include <vespa/fnet/frt/target.h>
#include <vespa/fnet/frt/rpcrequest.h>
class Test;
class SimpleHandler;

class MediumHandler1;
class MediumHandler2;
class MediumHandler3;

class ComplexHandler1;
class ComplexHandler2;
class ComplexHandler3;

//-------------------------------------------------------------

Test             *_test;

std::unique_ptr<fnet::frt::StandaloneFRT> _server;
FRT_Supervisor   *_supervisor;
FRT_Target       *_target;
SimpleHandler    *_simpleHandler;
MediumHandler1   *_mediumHandler1;
MediumHandler2   *_mediumHandler2;
MediumHandler3   *_mediumHandler3;
ComplexHandler1  *_complexHandler1;
ComplexHandler2  *_complexHandler2;
ComplexHandler3  *_complexHandler3;

bool              _mediumHandlerOK;
bool              _complexHandlerOK;

//-------------------------------------------------------------

class MediumA
{
public:

  /**
   * Destructor.  No cleanup needed for base class.
   */
  virtual ~MediumA(void) { }

  virtual void foo() = 0;
};


class MediumB
{
public:

  /**
   * Destructor.  No cleanup needed for base class.
   */
  virtual ~MediumB(void) { }

  virtual void bar() = 0;
};

//-------------------------------------------------------------

#ifdef __clang__
#define UNUSED_MEMBER [[maybe_unused]]
#else
#define UNUSED_MEMBER
#endif

class ComplexA
{
private:
  UNUSED_MEMBER uint32_t _fill1;
  UNUSED_MEMBER uint32_t _fill2;
  UNUSED_MEMBER uint32_t _fill3;

public:

  /**
   * Destructor.  No cleanup needed for base class.
   */
  virtual ~ComplexA(void) { }

  ComplexA() : _fill1(1), _fill2(2), _fill3(3) {}
  virtual void foo() {}
};


class ComplexB
{
private:
  UNUSED_MEMBER uint32_t _fill1;
  UNUSED_MEMBER uint32_t _fill2;
  UNUSED_MEMBER uint32_t _fill3;

public:

  /**
   * Destructor.  No cleanup needed for base class.
   */
  virtual ~ComplexB(void) { }

  ComplexB() : _fill1(1), _fill2(2), _fill3(3) {}
  virtual void bar() {}
};

//-------------------------------------------------------------

class SimpleHandler : public FRT_Invokable
{
public:
  void RPC_Method(FRT_RPCRequest *req);
};

//-------------------------------------------------------------

class MediumHandler1 : public FRT_Invokable,
                       public MediumA,
                       public MediumB
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};


class MediumHandler2 : public MediumA,
                       public FRT_Invokable,
                       public MediumB
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};


class MediumHandler3 : public MediumA,
                       public MediumB,
                       public FRT_Invokable
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};

//-------------------------------------------------------------

class ComplexHandler1 : public FRT_Invokable,
                        public ComplexA,
                        public ComplexB
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};


class ComplexHandler2 : public ComplexA,
                        public FRT_Invokable,
                        public ComplexB
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};


class ComplexHandler3 : public ComplexA,
                        public ComplexB,
                        public FRT_Invokable
{
public:
    void foo() override {}
    void bar() override {}
    void RPC_Method(FRT_RPCRequest *req);
};

//-------------------------------------------------------------

void initTest() {
    _server = std::make_unique<fnet::frt::StandaloneFRT>();
  _supervisor      = &_server->supervisor();
  _simpleHandler   = new SimpleHandler();
  _mediumHandler1  = new MediumHandler1();
  _mediumHandler2  = new MediumHandler2();
  _mediumHandler3  = new MediumHandler3();
  _complexHandler1 = new ComplexHandler1();
  _complexHandler2 = new ComplexHandler2();
  _complexHandler3 = new ComplexHandler3();

  ASSERT_TRUE(_supervisor != nullptr);
  ASSERT_TRUE(_simpleHandler != nullptr);
  ASSERT_TRUE(_mediumHandler1 != nullptr);
  ASSERT_TRUE(_mediumHandler2 != nullptr);
  ASSERT_TRUE(_mediumHandler3 != nullptr);
  ASSERT_TRUE(_complexHandler1 != nullptr);
  ASSERT_TRUE(_complexHandler2 != nullptr);
  ASSERT_TRUE(_complexHandler3 != nullptr);

  ASSERT_TRUE(_supervisor->Listen(0));
  std::string spec = vespalib::make_string("tcp/localhost:%d",
                                           _supervisor->GetListenPort());
  _target = _supervisor->GetTarget(spec.c_str());
  ASSERT_TRUE(_target != nullptr);

  FRT_ReflectionBuilder rb(_supervisor);

  //-------------------------------------------------------------------

  rb.DefineMethod("simpleMethod", "", "",
                  FRT_METHOD(SimpleHandler::RPC_Method),
                  _simpleHandler);

  //-------------------------------------------------------------------

  rb.DefineMethod("mediumMethod1", "", "",
                  FRT_METHOD(MediumHandler1::RPC_Method),
                  _mediumHandler1);

  rb.DefineMethod("mediumMethod2", "", "",
                  FRT_METHOD(MediumHandler2::RPC_Method),
                  _mediumHandler2);

  rb.DefineMethod("mediumMethod3", "", "",
                  FRT_METHOD(MediumHandler3::RPC_Method),
                  _mediumHandler3);

  //-------------------------------------------------------------------

  rb.DefineMethod("complexMethod1", "", "",
                  FRT_METHOD(ComplexHandler1::RPC_Method),
                  _complexHandler1);

  rb.DefineMethod("complexMethod2", "", "",
                  FRT_METHOD(ComplexHandler2::RPC_Method),
                  _complexHandler2);

  rb.DefineMethod("complexMethod3", "", "",
                  FRT_METHOD(ComplexHandler3::RPC_Method),
                  _complexHandler3);

  //-------------------------------------------------------------------

  _mediumHandlerOK  = true;
  _complexHandlerOK = true;
}


void finiTest() {
  delete _complexHandler1;
  delete _complexHandler2;
  delete _complexHandler3;
  delete _mediumHandler1;
  delete _mediumHandler2;
  delete _mediumHandler3;
  delete _simpleHandler;
  _target->SubRef();
  _server.reset();
}


TEST("method pt") {
  FRT_RPCRequest *req = _supervisor->AllocRPCRequest();
  req->SetMethodName("simpleMethod");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  //-------------------------------- MEDIUM

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("mediumMethod1");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("mediumMethod2");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("mediumMethod3");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  //-------------------------------- COMPLEX

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("complexMethod1");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("complexMethod2");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  req->SubRef();
  req = _supervisor->AllocRPCRequest();
  req->SetMethodName("complexMethod3");
  _target->InvokeSync(req, 60.0);
  EXPECT_TRUE(!req->IsError());

  if (_mediumHandlerOK) {
      fprintf(stderr, "Interface inheritance OK for method handlers\n");
  } else {
      fprintf(stderr, "Interface inheritance NOT ok for method handlers\n");
  }

  if (_complexHandlerOK) {
      fprintf(stderr, "Object inheritance OK for method handlers\n");
  } else {
      fprintf(stderr, "Object inheritance NOT ok for method handlers\n");
  }

  req->SubRef();
}

//-------------------------------------------------------------

void
SimpleHandler::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  EXPECT_TRUE(this == _simpleHandler);
}

//-------------------------------------------------------------

void
MediumHandler1::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _mediumHandlerOK = (_mediumHandlerOK &&
                      this == _mediumHandler1);
}


void
MediumHandler2::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _mediumHandlerOK = (_mediumHandlerOK &&
                      this == _mediumHandler2);
}


void
MediumHandler3::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _mediumHandlerOK = (_mediumHandlerOK &&
                      this == _mediumHandler3);
}

//-------------------------------------------------------------

void
ComplexHandler1::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _complexHandlerOK = (_complexHandlerOK &&
                       this == _complexHandler1);
}


void
ComplexHandler2::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _complexHandlerOK = (_complexHandlerOK &&
                       this == _complexHandler2);
}


void
ComplexHandler3::RPC_Method(FRT_RPCRequest *req)
{
  (void) req;
  _complexHandlerOK = (_complexHandlerOK &&
                       this == _complexHandler3);
}

//-------------------------------------------------------------

TEST_MAIN() {
    initTest();
    TEST_RUN_ALL();
    finiTest();
}
