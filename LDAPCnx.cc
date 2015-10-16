#include "LDAPCnx.h"

static struct timeval ldap_tv = { 0, 0 };

Nan::Persistent<v8::Function> LDAPCnx::constructor;

LDAPCnx::LDAPCnx(double value) {
}

LDAPCnx::~LDAPCnx() {
  delete this->callback;
}

void LDAPCnx::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("LDAPCnx").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "search", Search);
  Nan::SetPrototypeMethod(tpl, "delete", Delete);
  Nan::SetPrototypeMethod(tpl, "initialize", Initialize);

  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("LDAPCnx").ToLocalChecked(), tpl->GetFunction());
}

void LDAPCnx::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if (info.IsConstructCall()) {
    // Invoked as constructor: `new LDAPCnx(...)`
    double value = info[0]->IsUndefined() ? 0 : info[0]->NumberValue();
    LDAPCnx* obj = new LDAPCnx(value);
    obj->Wrap(info.Holder());

    obj->callback = new Nan::Callback(info[1].As<v8::Function>());
    
    info.GetReturnValue().Set(info.Holder());
  } else {
    // Invoked as plain function `LDAPCnx(...)`, turn into construct call.
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

const char * LDAPCnx::ErrCheck() {
  int err;
  ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &err);
  if (err) {
    return ldap_err2string(err);
  }
  return NULL;
}

void LDAPCnx::Event(uv_poll_t* handle, int status, int events) {
  Nan::HandleScope scope;

  LDAPCnx *ld = (LDAPCnx *)handle->data;
  LDAPMessage * message = NULL;
  LDAPMessage * entry = NULL;

  switch(ldap_result(ld->ld, LDAP_RES_ANY, LDAP_MSG_ALL, &ldap_tv, &message)) {
  case 0:
    // timeout occurred
  case -1:
    {
      printf("MSGID is %i\n", ldap_msgid(message));
      if (ld->ErrCheck()) {
        printf("ERR\n");
      }
      break;
    }
  default:
    {
      switch ( ldap_msgtype( message ) ) {
      case LDAP_RES_SEARCH_REFERENCE:
        break;
      case LDAP_RES_SEARCH_ENTRY:
      case LDAP_RES_SEARCH_RESULT:
        {
          v8::Local<v8::Array> js_result_list = Nan::New<v8::Array>(ldap_count_entries(ld->ld, message));

          int j;
  
          for (entry = ldap_first_entry(ld->ld, message), j = 0 ; entry ;
               entry = ldap_next_entry(ld->ld, entry), j++) {
            v8::Local<v8::Object> js_result = Nan::New<v8::Object>();

            js_result_list->Set(Nan::New(j), js_result);
    
            char * dn = ldap_get_dn(ld->ld, entry);
            BerElement * berptr = NULL;
            for (char * attrname = ldap_first_attribute(ld->ld, entry, &berptr) ;
                 attrname ; attrname = ldap_next_attribute(ld->ld, entry, berptr)) {
              berval ** vals = ldap_get_values_len(ld->ld, entry, attrname);
              int num_vals = ldap_count_values_len(vals);
              v8::Local<v8::Array> js_attr_vals = Nan::New<v8::Array>(num_vals);
              js_result->Set(Nan::New(attrname).ToLocalChecked(), js_attr_vals);

              // int bin = ld->isBinary(attrname);
              int bin = 0;
              
              for (int i = 0 ; i < num_vals && vals[i] ; i++) {
                if (bin) {
                  // js_attr_vals->Set(Nan::New(i), ld->makeBuffer(vals[i]));
                } else {
                  js_attr_vals->Set(Nan::New(i), Nan::New(vals[i]->bv_val).ToLocalChecked());
                }
              } // all values for this attr added.
              ldap_value_free_len(vals);
              ldap_memfree(attrname);
            } // attrs for this entry added. Next entry.
            js_result->Set(Nan::New("dn").ToLocalChecked(), Nan::New(dn).ToLocalChecked());
            ber_free(berptr,0);
            ldap_memfree(dn);
          } // all entries done.
  
          v8::Local<v8::Value> argv[] = {
            Nan::Undefined(), // holds error
            Nan::New(ldap_msgid(message)),
            js_result_list
          };
          ld->callback->Call(3, argv);
          break;
        }
      case LDAP_RES_BIND:
      case LDAP_RES_MODIFY:
      case LDAP_RES_MODDN:
      case LDAP_RES_ADD:
      case LDAP_RES_DELETE:
        {
          printf("DELETE RESULT\n");
          v8::Local<v8::Value> argv[] = {
            Nan::Undefined(), // holds error
            Nan::New(ldap_msgid(message))
          };
          ld->callback->Call(2, argv);
          break;
        }
      default:
        {
          
          //emit an error
          // Nan::ThrowError("Unrecognized packet");
        }
      }
    }
  }
  ldap_msgfree(message);
  return;
}

void LDAPCnx::Initialize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  LDAPCnx* ld = ObjectWrap::Unwrap<LDAPCnx>(info.Holder());
  Nan::Utf8String url(info[0]);
  uv_poll_t * handle = new uv_poll_t;
  handle->data = ld;
  int fd = 0;

  if (ldap_initialize(&(ld->ld), *url) != LDAP_SUCCESS) {
    Nan::ThrowError("Error init");
    return;
  }

   ldap_get_option(ld->ld, LDAP_OPT_DESC, &fd);
   
   if ((ldap_simple_bind(ld->ld, NULL, NULL)) == -1) {
     Nan::ThrowError("Error anon bind");
     return;
   }
   ldap_get_option(ld->ld, LDAP_OPT_DESC, &fd);

   ldap_get_option(ld->ld, LDAP_OPT_DESC, &fd);

   if (fd < 0) {
     Nan::ThrowError("Connection issue");
     return;
   }
   
   uv_poll_init(uv_default_loop(), handle, fd);
   uv_poll_start(handle, UV_READABLE, Event);

   info.GetReturnValue().Set(info.This());
}

void LDAPCnx::Delete(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  LDAPCnx* ld = ObjectWrap::Unwrap<LDAPCnx>(info.Holder());
  Nan::Utf8String dn(info[0]);

  info.GetReturnValue().Set(ldap_delete(ld->ld, *dn));
}

void LDAPCnx::Search(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  LDAPCnx* ld = ObjectWrap::Unwrap<LDAPCnx>(info.Holder());
  Nan::Utf8String base(info[0]);
  Nan::Utf8String filter(info[1]);
  Nan::Utf8String attrs(info[2]);
  int msgid = 0;
  int res;
  char * attrlist[255];

  char *bufhead = strdup(*attrs);
  char *buf = bufhead;
  char **ap;
  for (ap = attrlist; (*ap = strsep(&buf, " \t,")) != NULL;)
    if (**ap != '\0')
      if (++ap >= &attrlist[255])
        break;
  
  res = ldap_search_ext(ld->ld, *base, 2, *filter , (char **)attrlist, 0,
                         NULL, NULL, NULL, 0, &msgid);

  free(bufhead);
  
  info.GetReturnValue().Set(msgid);
}
