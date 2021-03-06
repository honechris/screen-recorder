#include <node.h>
#include <node_buffer.h>
#include <uv.h>

#ifdef WIN32
#include "../platform/win32/export.h"
#elif __MAC__
#include "../platform/mac/export.h"
#define PTHREAD_MUTEX_RECURSIVE_NP 1
#endif

#include <queue>
#include <string>
#include <Windows.h>

namespace recorder
{
	using namespace v8;
	using namespace node;

	class Locker
	{
	public:
		Locker()
		{
#ifdef WIN32
			InitializeCriticalSection(&m_csLock);
#else
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
			pthread_mutex_init(&m_csLock, &attr);
#endif
		}
		virtual ~Locker()
		{
#ifdef WIN32
			DeleteCriticalSection(&m_csLock);
#else
			pthread_mutex_destroy(&m_csLock);
#endif
		}

		long Lock()
		{
#ifdef WIN32
			EnterCriticalSection(&m_csLock);
			return m_csLock.LockCount;
#else
			pthread_mutex_lock(&m_csLock);
			return 1;
#endif
		}
		long Unlock()
		{
#ifdef WIN32
			LeaveCriticalSection(&m_csLock);
			return m_csLock.LockCount;
#else
			pthread_mutex_unlock(&m_csLock);
			return 1;
#endif
		}

	private:
#ifdef WIN32
		CRITICAL_SECTION m_csLock;
#else
		pthread_mutex_t m_csLock;
#endif
	};

	std::string unicode_utf8(const std::wstring & wstr)
	{
		int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
		std::string ret_str = pAssii;
		free(pAssii);
		return ret_str;
	}

	std::wstring utf8_unicode(const std::string & utf8)
	{
		int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, pUnicode, unicodeLen);
		std::wstring ret_str = pUnicode;
		free(pUnicode);
		return ret_str;
	}

	std::wstring ascii_unicode(const std::string & str)
	{
		int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);

		wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);

		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);

		std::wstring ret_str = pUnicode;

		free(pUnicode);

		return ret_str;
	}

	std::string unicode_ascii(const std::wstring & wstr)
	{
		int ansiiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
		std::string ret_str = pAssii;
		free(pAssii);
		return ret_str;
	}

	std::string ascii_utf8(const std::string & str)
	{
		return unicode_utf8(ascii_unicode(str));
	}

	std::string utf8_ascii(const std::string & utf8)
	{
		return unicode_ascii(utf8_unicode(utf8));
	}

	Local<String> utf8_v8string(Isolate *isolate,const char *utf8) 
	{
#if NODE_VERSION_AT_LEAST(12,0,0)
		return String::NewFromUtf8(isolate, utf8).ToLocalChecked();
#elif NODE_VERSION_AT_LEAST(10,0,0)
		return String::NewFromUtf8(isolate, utf8);
#else
		return String::NewFromUtf8(isolate, utf8);
#endif
	}

	bool CheckParamCount(unsigned int nLength, unsigned int nCount) {
		if (nLength != nCount) {
			Isolate* isolate = Isolate::GetCurrent();
			isolate->ThrowException(Exception::TypeError(
				utf8_v8string(isolate,"Wrong number of arguments")));
			return false;
		}
		return true;
	}

	bool CheckParamValid(Local<Value> value, const char* strType) {
		bool bOK = true;
		if (strType) {
			if (strcmp(strType, "string") == 0)
				bOK = value->IsString();
			else if (strcmp(strType, "uint32") == 0)
				bOK = value->IsUint32();
			else if (strcmp(strType, "int32") == 0)
				bOK = value->IsInt32();
			else if (strcmp(strType, "bool") == 0)
				bOK = value->IsBoolean();
			else if (strcmp(strType, "buffer") == 0)
				bOK = Buffer::HasInstance(value);
			else if (strcmp(strType, "function") == 0)
				bOK = value->IsFunction();
		}
		if (!bOK) {
			Isolate* isolate = Isolate::GetCurrent();
			isolate->ThrowException(Exception::TypeError(
				utf8_v8string(isolate, "Wrong arguments")));
		}
		return bOK;
	}

#define CHECK_PARAM_COUNT(nCount) \
	if(!CheckParamCount(args.Length(), nCount)) \
		return;

#define CHECK_PARAM_VALID(value, type) \
	if(!CheckParamValid(value, type)) \
		return;

#define CHECK_PARAM_TYPE7(type1, type2, type3, type4, type5, type6, type7) \
	if (type1 != NULL) \
		CHECK_PARAM_VALID(args[0], type1); \
	if (type2 != NULL) \
		CHECK_PARAM_VALID(args[1], type2); \
	if (type3 != NULL) \
		CHECK_PARAM_VALID(args[2], type3); \
	if (type4 != NULL) \
		CHECK_PARAM_VALID(args[3], type4); \
	if (type5 != NULL) \
		CHECK_PARAM_VALID(args[4], type5); \
	if (type6 != NULL) \
		CHECK_PARAM_VALID(args[5], type6); \
  if (type7 != NULL) \
		CHECK_PARAM_VALID(args[6], type7);

#define CHECK_PARAM_TYPE6(type1, type2, type3, type4, type5, type6) \
	if (type1 != NULL) \
		CHECK_PARAM_VALID(args[0], type1); \
	if (type2 != NULL) \
		CHECK_PARAM_VALID(args[1], type2); \
	if (type3 != NULL) \
		CHECK_PARAM_VALID(args[2], type3); \
	if (type4 != NULL) \
		CHECK_PARAM_VALID(args[3], type4); \
	if (type5 != NULL) \
		CHECK_PARAM_VALID(args[4], type5); \
	if (type6 != NULL) \
		CHECK_PARAM_VALID(args[5], type6);

#define CHECK_PARAM_TYPE5(type1, type2, type3, type4, type5) \
	CHECK_PARAM_TYPE6(type1, type2, type3, type4, type5, NULL)

#define CHECK_PARAM_TYPE4(type1, type2, type3, type4) \
	CHECK_PARAM_TYPE5(type1, type2, type3, type4, NULL)

#define CHECK_PARAM_TYPE3(type1, type2, type3) \
	CHECK_PARAM_TYPE4(type1, type2, type3, NULL)

#define CHECK_PARAM_TYPE2(type1, type2) \
	CHECK_PARAM_TYPE3(type1, type2, NULL)

#define CHECK_PARAM_TYPE1(type1) \
	CHECK_PARAM_TYPE2(type1, NULL)

	typedef enum {
		uvcb_type_duration = 1,
		uvcb_type_error,
		uvcb_type_device_change,
		uvcb_type_preview_image,
		uvcb_type_preview_audio
	}uvCallbackType;

	typedef struct {
		uint64_t duration;
	}uvCallBackDataDruation;

	typedef struct {
		int error;
	}uvCallBackDataError;

	typedef struct {
		int type;
	}uvCallBackDataDeviceChange;

	typedef struct {
		int size;
		int width;
		int height;
		int type;
		uint8_t *data;
	}uvCallBackDataPreviewImage;

	typedef struct {
		uvCallbackType type;
		int *data;
	}uvCallBackChunk;

	static Persistent<Function>* cb_uv_duration = NULL;
	static Persistent<Function>* cb_uv_error = NULL;
	static Persistent<Function>* cb_uv_device_change = NULL;
	static Persistent<Function>* cb_uv_preview_image = NULL;

	static uv_async_t s_async = { 0 };
	static Locker locker;

	static std::queue<uvCallBackChunk> cb_chunk_queue;

	void PushUvChunk(const uvCallBackChunk &chunk) {
		locker.Lock();
		cb_chunk_queue.push(chunk);
		locker.Unlock();

		uv_async_send(&s_async);
	}

	void DispatchUvRecorderDuration(Isolate* isolate, uvCallBackDataDruation *data) {
		if (!cb_uv_duration) return;

		const unsigned argc = 1;
		Local<Value> argv[argc] = { Uint32::New(isolate, data->duration) };
		Local<Value> recv;
		Local<Function>::New(isolate, *cb_uv_duration)->Call(isolate->GetCurrentContext(), recv, argc, argv);
	}

	void DispatchUvRecorderError(Isolate* isolate, uvCallBackDataError *data) {
		if (!cb_uv_error) return;

		const unsigned argc = 1;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->error)
		};
		Local<Value> recv;
		Local<Function>::New(isolate, *cb_uv_error)->Call(isolate->GetCurrentContext(), recv, argc, argv);
	}

	void DispatchUvRecorderDeviceChange(Isolate* isolate, uvCallBackDataDeviceChange *data) {
		if (!cb_uv_device_change) return;

		const unsigned argc = 1;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->type)
		};
		Local<Value> recv;
		Local<Function>::New(isolate, *cb_uv_device_change)->Call(isolate->GetCurrentContext(), recv, argc, argv);
	}

	void DispatchUvRecorderPreviewImage(Isolate* isolate, uvCallBackDataPreviewImage *data) {
		if (!cb_uv_preview_image) return;

		const unsigned argc = 5;
		Local<Value> argv[argc] = {
			Uint32::New(isolate, data->size)
		};
		Local<Value> recv;
		Local<Function>::New(isolate, *cb_uv_preview_image)->Call(isolate->GetCurrentContext(), recv, argc, argv);
	}

	void OnRecorderDuration(uint64_t duration) {
		uvCallBackDataDruation *data = new uvCallBackDataDruation;
		data->duration = duration;

		DispatchUvRecorderDuration(Isolate::GetCurrent(), data);
		delete data;

		return;//do not use libuv now

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_duration;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderError(int error) {
		uvCallBackDataError *data = new uvCallBackDataError;
		data->error = error;

		DispatchUvRecorderError(Isolate::GetCurrent(), data);
		delete data;

		return;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_error;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderDeviceChange(int type) {
		uvCallBackDataDeviceChange *data = new uvCallBackDataDeviceChange;
		data->type = type;

		DispatchUvRecorderDeviceChange(Isolate::GetCurrent(), data);
		delete data;

		return;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_device_change;
		uv_cb_chunk.data = (int*)data;

		PushUvChunk(uv_cb_chunk);
	}

	void OnRecorderPreviewImage(const unsigned char *data,unsigned int size,int width,int height,int type){
		char *buff = new char[sizeof(uvCallBackDataPreviewImage) + size];
		uvCallBackDataPreviewImage *image = (uvCallBackDataPreviewImage*)buff;

		memcpy(image->data,data,size);
		image->size = size;
		image->width = width;
		image->height = height;
		image->type = type;

		DispatchUvRecorderPreviewImage(Isolate::GetCurrent(), image);

		return;

		uvCallBackChunk uv_cb_chunk;
		uv_cb_chunk.type = uvcb_type_preview_image;
		uv_cb_chunk.data = (int*)image;

		PushUvChunk(uv_cb_chunk);
	}


	void OnUvCallback(uv_async_t *handle) {
		locker.Lock();
		while (!cb_chunk_queue.empty()) {
			uvCallBackChunk &chunk = cb_chunk_queue.front();
			cb_chunk_queue.pop();
			Isolate* isolate = Isolate::GetCurrent();
			switch (chunk.type)
			{
			case uvcb_type_duration:
				DispatchUvRecorderDuration(isolate, (uvCallBackDataDruation*)chunk.data);
				break;
			case uvcb_type_error:
				DispatchUvRecorderError(isolate, (uvCallBackDataError*)chunk.data);
				break;
			case uvcb_type_device_change:
				DispatchUvRecorderDeviceChange(isolate, (uvCallBackDataDeviceChange*)chunk.data);
				break;
			case uvcb_type_preview_image:
				DispatchUvRecorderPreviewImage(isolate,(uvCallBackDataPreviewImage*)chunk.data);
				break;
			default:
				break;
			}

			delete chunk.data;
		}

		locker.Unlock();
	}

	void GetSpeakers(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		AMRECORDER_DEVICE *devices;
		int ret = recorder_get_speakers(&devices);
		if (ret > 0) {

			Local<Array> array = Array::New(isolate, ret);

			for (int i = 0; i<ret; i++) {
				Local<Object> device = Object::New(isolate);
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "id"), utf8_v8string(isolate, devices[i].id));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "name"), utf8_v8string(isolate, devices[i].name));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "isDefault"), Number::New(isolate, devices[i].is_default));
				array->Set(isolate->GetCurrentContext(), i, device);
			}

			delete[]devices;

			args.GetReturnValue().Set(array);
		}
		else {
			args.GetReturnValue().Set(utf8_v8string(isolate, "get speaker list failed."));
		}
	}

	void GetMics(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		AMRECORDER_DEVICE *devices;
		int ret = recorder_get_mics(&devices);
		if (ret > 0) {

			Local<Array> array = Array::New(isolate, ret);

			for (int i = 0; i<ret; i++) {
				Local<Object> device = Object::New(isolate);
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "id"), utf8_v8string(isolate, devices[i].id));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "name"), utf8_v8string(isolate, devices[i].name));
				device->Set(isolate->GetCurrentContext(), utf8_v8string(isolate, "isDefault"), Number::New(isolate, devices[i].is_default));
				array->Set(isolate->GetCurrentContext(), i, device);
			}

			delete[]devices;

			args.GetReturnValue().Set(array);
		}
		else {
			args.GetReturnValue().Set(utf8_v8string(isolate, "get mic list failed."));
		}
	}

	void GetCameras(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		args.GetReturnValue().Set(utf8_v8string(isolate, "get camera list not support for now."));
	}

	void SetDurationCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		Persistent<Function>* callback = new Persistent<Function>;
		callback->Reset(isolate, args[0].As<Function>());

		locker.Lock();
		cb_uv_duration = callback;
		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetErrorCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		Persistent<Function>* callback = new Persistent<Function>;
		callback->Reset(isolate, args[0].As<Function>());

		locker.Lock();
		cb_uv_error = callback;
		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetDeviceChangeCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		Persistent<Function>* callback = new Persistent<Function>;
		callback->Reset(isolate, args[0].As<Function>());

		locker.Lock();
		cb_uv_device_change = callback;
		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void SetPreviewImageCallBack(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();
		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("function");

		Persistent<Function>* callback = new Persistent<Function>;
		callback->Reset(isolate, args[0].As<Function>());

		locker.Lock();
		cb_uv_preview_image = callback;
		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Init(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		//v_qb v_frame_rate v_output a_speaker a_mic
		CHECK_PARAM_COUNT(7);
		CHECK_PARAM_TYPE7("uint32", "uint32", " string", "string", "string", "string", "string");

		int error = 0;

		AMRECORDER_SETTING settings;
		AMRECORDER_CALLBACK callbacks;

		callbacks.func_duration = OnRecorderDuration;
		callbacks.func_error = OnRecorderError;
		callbacks.func_device_change = OnRecorderDeviceChange;
		callbacks.func_preview_image = OnRecorderPreviewImage;

		settings.v_left = 0;
		settings.v_top = 0;
		settings.v_width = GetSystemMetrics(SM_CXSCREEN);
		settings.v_height = GetSystemMetrics(SM_CYSCREEN);
		settings.v_bit_rate = 64000;


		settings.v_qb = args[0]->Uint32Value();
		settings.v_frame_rate = args[1]->Uint32Value();

		String::Utf8Value utf8Output(Local<String>::Cast(args[2]));
		sprintf_s(settings.output, 260, "%s", *utf8Output);

		String::Utf8Value utf8SpeakerName(Local<String>::Cast(args[3]));
		sprintf_s(settings.a_speaker.name, 260, "%s", *utf8SpeakerName);

		String::Utf8Value utf8SpeakerId(Local<String>::Cast(args[4]));
		sprintf_s(settings.a_speaker.id, 260, "%s", *utf8SpeakerId);

		String::Utf8Value utf8MicName(Local<String>::Cast(args[5]));
		sprintf_s(settings.a_mic.name, 260, "%s", *utf8MicName);

		String::Utf8Value utf8MicId(Local<String>::Cast(args[6]));
		sprintf_s(settings.a_mic.id, 260, "%s", *utf8MicId);

		error = recorder_init(settings, callbacks);

		//if(error = 0)//registe all call back to uv callback,this wont be succed,don't know why
		//  uv_async_init(uv_default_loop(), &s_async, OnUvCallback);

		args.GetReturnValue().Set(Int32::New(isolate, error));
	}

	void Release(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_release();

		//close uv call back,this will crahs ,don't know why
		//uv_close((uv_handle_t*)&s_async, NULL);

		locker.Lock();
		cb_uv_duration = NULL;
		cb_uv_error = NULL;
		cb_uv_device_change = NULL;
		cb_uv_preview_image = NULL;
		locker.Unlock();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Start(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		int error = recorder_start();

		args.GetReturnValue().Set(Int32::New(isolate, error));
	}

	void Stop(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_stop();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Pause(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_pause();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Resume(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		recorder_resume();

		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Wait(const FunctionCallbackInfo<Value> &args) {
		Isolate* isolate = args.GetIsolate();

		CHECK_PARAM_COUNT(1);
		CHECK_PARAM_TYPE1("uint32");

		int timestamp = args[0]->Uint32Value();

		Sleep(timestamp);


		args.GetReturnValue().Set(Boolean::New(isolate, true));
	}

	void Initialize(Local<Object> exports)
	{
		NODE_SET_METHOD(exports, "GetSpeakers", GetSpeakers);
		NODE_SET_METHOD(exports, "GetMics", GetMics);
		NODE_SET_METHOD(exports, "GetCameras", GetCameras);
		NODE_SET_METHOD(exports, "SetDurationCallBack", SetDurationCallBack);
		NODE_SET_METHOD(exports, "SetDeviceChangeCallBack", SetDeviceChangeCallBack);
		NODE_SET_METHOD(exports, "SetErrorCallBack", SetErrorCallBack);
		NODE_SET_METHOD(exports, "SetPreviewImageCallBack", SetPreviewImageCallBack);
		NODE_SET_METHOD(exports, "Init", Init);
		NODE_SET_METHOD(exports, "Release", Release);
		NODE_SET_METHOD(exports, "Start", Start);
		NODE_SET_METHOD(exports, "Stop", Stop);
		NODE_SET_METHOD(exports, "Pause", Pause);
		NODE_SET_METHOD(exports, "Resume", Resume);
		NODE_SET_METHOD(exports, "Wait", Wait);
	}

	NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

} // namespace recorder
