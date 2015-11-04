#include "fingerprint.h"

using namespace v8;
using v8::FunctionTemplate;

extern int initalized;

typedef struct __VERIFY_STOP__ {
    uv_async_t async;
    Nan::Persistent<Function> callback;
} VERIFY_STOP;

typedef struct __VERIFY_START__ {
    uv_async_t async;
    Nan::Persistent<Function> callback;
    int result;
} VERIFY_START;

static const char *verify_result_to_name(int result)
{
    switch (result) {
    case FP_VERIFY_MATCH: return "verfiy-succeeded";
	case FP_VERIFY_NO_MATCH: return "verify-failed";
	case FP_VERIFY_RETRY: return "verify-retry-scan";
	case FP_VERIFY_RETRY_TOO_SHORT: return "verify-swipe-too-short";
	case FP_VERIFY_RETRY_CENTER_FINGER: return "verify-finger-not-centered";
	case FP_VERIFY_RETRY_REMOVE_FINGER: return "verify-remove-and-retry";
    default: return "verify-unknown-error";
	}
}

void verify_stop_after(uv_handle_t* handle)
{
    VERIFY_STOP *data = container_of((uv_async_t *)handle, VERIFY_STOP, async);

    if(!data)
        return;

    delete data;
}

#ifdef OLD_UV_RUN_SIGNATURE
void report_verify_stop(uv_async_t *handle)
#else
void report_verify_stop(uv_async_t *handle, int status)
#endif
{
    VERIFY_STOP *data = container_of(handle, VERIFY_STOP, async);
    Nan::HandleScope();

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    callback.Call(0, NULL);
    uv_close((uv_handle_t*)&data->async, verify_stop_after);
}

static void verify_stop_cb(struct fp_dev *dev, void *user_data)
{
    VERIFY_STOP *data = (VERIFY_STOP*)user_data;

    if(!data)
        return;

    uv_async_send(&data->async);
}

int fp_async_verify_stop(struct fp_dev *dev, fp_verify_stop_cb callback, void *user_data);

NAN_METHOD(verifyStop) {
    bool ret = false;
    struct fp_dev *dev;
    VERIFY_STOP *data;

    if(info.Length() < 1)
        goto error;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0 || dev == NULL)
        goto error;

    if(info.Length() > 1) {
        data = new VERIFY_STOP;
        data->callback.Reset(v8::Local<v8::Function>::Cast(info[1]));
        uv_async_init(uv_default_loop(), &data->async, report_verify_stop);
	   ret = fp_async_verify_stop(dev, verify_stop_cb, data) == 0;
    }
    else
	   ret = fp_async_verify_stop(dev, NULL, 0) == 0;

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

void verify_start_after(uv_handle_t* handle)
{
    VERIFY_START *data = container_of((uv_async_t *)handle, VERIFY_START, async);

    if(!data)
        return;

    delete data;
}

#ifdef OLD_UV_RUN_SIGNATURE
void report_verify_start(uv_async_t *handle)
#else
void report_verify_start(uv_async_t *handle, int status)
#endif
{
    VERIFY_START *data = container_of(handle, VERIFY_START, async);
    Nan::HandleScope();

    if(!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Local<Value> argv[2];
    argv[0] = Nan::New(data->result);
    argv[1] = Nan::Null();

    if(verify_result_to_name(data->result))
        argv[1] = Nan::New(verify_result_to_name(data->result)).ToLocalChecked();

    callback.Call(2, argv);
    if(data->result == FP_VERIFY_NO_MATCH || data->result == FP_VERIFY_MATCH) {
        uv_close((uv_handle_t*)&data->async, verify_start_after);
    }
}

static void verify_start_cb(struct fp_dev *dev, int result, struct fp_img *img, void *user_data)
{
    VERIFY_START *data = (VERIFY_START*)user_data;

    if(!data)
        return;

    data->result = result;

    uv_async_send(&data->async);
    if(img) fp_img_free(img);
}

NAN_METHOD(verifyStart) {
    struct fp_print_data *fpdata = NULL;
    bool ret = false;
    struct fp_dev *dev;
    VERIFY_START *data;
    Local<Object> obj;

    if(info.Length() < 3)
        goto error;

    dev = toFPDev(info[0]->ToNumber()->Value());
    if(initalized != 0 || dev == NULL)
        goto error;

    data = new VERIFY_START;
    data->callback.Reset(v8::Local<v8::Function>::Cast(info[2]));
    obj = info[1]->ToObject();
    fpdata = fp_print_data_from_data((unsigned char*)node::Buffer::Data(obj), (size_t)node::Buffer::Length(obj));
    uv_async_init(uv_default_loop(), &data->async, report_verify_start);
	ret = fp_async_verify_start(dev, fpdata, verify_start_cb, data) == 0;
error:
    info.GetReturnValue().Set(Nan::New(ret));
}
