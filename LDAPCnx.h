#ifndef LDAPCNX_H
#define LDAPCNX_H

#include <nan.h>
#include <ldap.h>

class LDAPCnx : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  Nan::Callback * callback;
  static void Event(uv_poll_t* handle, int status, int events);
  uv_poll_t * handle = NULL;
  
 private:
  explicit LDAPCnx();
  ~LDAPCnx();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Initialize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Restart(LDAP *ld, Sockbuf *sb, LDAPURLDesc *srv, struct sockaddr *addr, struct ldap_conncb *ctx);
  static void Search(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Delete(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Bind(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Add(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Modify(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Rename(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetErr(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetErrNo(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetFD(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> constructor;
  LDAP * ld;
};

#endif
