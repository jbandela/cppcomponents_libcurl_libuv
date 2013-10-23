#include "cppcomponents_libcurl_libuv.hpp"
#include "implementation/constants.hpp"
#include <curl/curl.h>

#include <cppcomponents_libuv/cppcomponents_libuv.hpp>

#include <array>
#include <algorithm>
#include <map>


using namespace cppcomponents;
using namespace cppcomponents_libcurl_libuv;

namespace uv = cppcomponents_libuv;

inline void curl_throw_if_error(CURLFORMcode code){
	if (code != CURL_FORMADD_OK){
		throw_if_error(-static_cast<error_code>(code));
	}
}


inline void curl_throw_if_error(CURLcode code){
	if (code != CURLE_OK){
		throw_if_error(-static_cast<error_code>(code));
	}
}
inline void curl_throw_if_error(CURLMcode code){
	if (code != CURLM_OK){
		throw_if_error(-static_cast<error_code>(code));
	}
}


struct ImpSlist :implement_runtime_class<ImpSlist, Slist_t>{

	curl_slist* list_;

	ImpSlist() :list_{ nullptr }{}

	~ImpSlist(){
		if (!list_){
			curl_slist_free_all(list_);
		}
	}

	void Append(cppcomponents::cr_string str){
		list_ = curl_slist_append(list_, str.data());
		if (!list_){
			throw error_fail();
		}
	}

	void* GetNative(){
		return list_;
	}
};

struct ImpForm :implement_runtime_class<ImpForm, Form_t>{

	curl_httppost* first_;
	curl_httppost* last_;

	ImpForm() :first_{ nullptr }, last_{ nullptr }{}

	~ImpForm(){
		if (first_){
			curl_formfree(first_);
		}
	}

	void AddNameValue(cppcomponents::cr_string name, cppcomponents::cr_string value){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_COPYNAME, name.data(),
			CURLFORM_NAMELENGTH, (long)name.size(),
			CURLFORM_COPYCONTENTS, value.data(),
			CURLFORM_CONTENTSLENGTH, (long)value.size(),
			CURLFORM_END);
		curl_throw_if_error(res);
	}
	void AddNameFileContents(cppcomponents::cr_string name, cppcomponents::cr_string file){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_COPYNAME, name.data(),
			CURLFORM_NAMELENGTH, (long)name.size(),
			CURLFORM_FILECONTENT, file.data(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}
	void AddFileNoName(cppcomponents::cr_string file){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_FILE, file.data(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}
	void AddFileNoNameWithContentType(cppcomponents::cr_string file, cppcomponents::cr_string content_type){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_FILE, file.data(),
			CURLFORM_CONTENTTYPE, content_type.data(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}
	void AddFileWithName(cppcomponents::cr_string file, cppcomponents::cr_string filename){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_FILE, file.data(),
			CURLFORM_FILENAME, filename.data(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}
	void AddFileWithNameContentType(cppcomponents::cr_string file, cppcomponents::cr_string filename, cppcomponents::cr_string content_type){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_FILE, file.data(),
			CURLFORM_CONTENTTYPE, content_type.data(),
			CURLFORM_FILENAME, filename.data(),
			CURLFORM_END);

		curl_throw_if_error(res);
	}

	// buffer must be kept around until after post is complete
	void AddFileAsBuffer(cppcomponents::use<cppcomponents::IBuffer> buffer, cppcomponents::cr_string filename, cppcomponents::cr_string content_type){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_BUFFERPTR, buffer.Begin(),
			CURLFORM_BUFFERLENGTH, (long)buffer.Size(),
			CURLFORM_CONTENTTYPE, content_type.data(),
			CURLFORM_BUFFER, filename.data(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}

	void AddContentHeader(cppcomponents::use<ISlist> list){
		auto res = curl_formadd(&first_, &last_,
			CURLFORM_CONTENTHEADER, list.GetNative(),
			CURLFORM_END);

		curl_throw_if_error(res);

	}

	void* GetNative(){
		return first_;
	}
};

template<class Delegate>
 use<cppcomponents::delegate<Delegate>> DelegateFromVoid(void* userdata){
	// user data is the delegate pointer
	use<InterfaceUnknown> iunk{ cppcomponents::reinterpret_portable_base<InterfaceUnknown>(
		static_cast<portable_base*>(userdata)), true };
	return iunk.QueryInterface<Delegate>();
}

 struct ImpEasy :implement_runtime_class<ImpEasy, Easy_t>
 {
	 CURL* easy_;

	 std::array < char, CURL_ERROR_SIZE + 1> error_buffer_;

	 use<IForm> form_;

	 use<Callbacks::WriteFunction> write_function_;
	 use<Callbacks::ReadFunction> read_function_;
	 use<Callbacks::HeaderFunction> header_function;
	 use<Callbacks::ProgressFunction> progress_function_;

	 std::map < const void*, use<InterfaceUnknown> > extra_info_;



	 ImpEasy()
		 :
		 easy_{ curl_easy_init() }
	 {
		 if (!easy_){
			 throw error_fail();
		 }

		 // Set the private of easy to interface
		 auto res = curl_easy_setopt(easy_, CURLOPT_PRIVATE, this->get_unknown_portable_base());
		 curl_throw_if_error(res);
		 std::fill(error_buffer_.begin(), error_buffer_.end(), 0);
	 }

	 ~ImpEasy(){
		 if (easy_){
			 curl_easy_cleanup(easy_);
		 }
	 }
	 void SetInt32Option(std::int32_t option, std::int32_t parameter){
		 auto res = curl_easy_setopt(easy_, static_cast<CURLoption>(option), (long)parameter);
		 curl_throw_if_error(res);

	 }
	 void SetPointerOption(std::int32_t option, void* parameter){
		 if (option == CURLOPT_WRITEDATA ||
			 option == CURLOPT_READDATA ||
			 option == CURLOPT_PROGRESSDATA ||
			 option == CURLOPT_HEADERDATA ||
			 option == CURLOPT_PRIVATE
			 )
		 {

			 throw error_invalid_arg();
		 }
		 auto res = curl_easy_setopt(easy_, static_cast<CURLoption>(option), parameter);
		 curl_throw_if_error(res);

	 }
	 void SetInt64Option(std::int32_t option, std::int64_t parameter){
		 auto res = curl_easy_setopt(easy_, static_cast<CURLoption>(option), (curl_off_t)parameter);
		 curl_throw_if_error(res);

	 }

	 static std::size_t WriteFunctionRaw(char* ptr, std::size_t size, std::size_t nmemb, void* userdata){
		 // user data is the delegate pointer
		 use<InterfaceUnknown> iunk{ cppcomponents::reinterpret_portable_base<InterfaceUnknown>(
			 static_cast<portable_base*>(userdata)), true };
		 auto func = iunk.QueryInterface<Callbacks::WriteFunction>();
		 return func(ptr, size, nmemb);
	 }
	 static std::size_t ReadFunctionRaw(void* ptr, std::size_t size, std::size_t nmemb, void* userdata){
		 // user data is the
	 }
	 static std::size_t HeaderFunctionRaw(void* ptr, std::size_t size, std::size_t nmemb, void* userdata){
		 // user data is the
	 }

	 int ProgressFunctionRaw(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow){

	 }

	 void SetFunctionOption(std::int32_t option, cppcomponents::use<cppcomponents::InterfaceUnknown> function){

		 if (option == CURLOPT_WRITEFUNCTION){

		 }

	 }
	 void* GetNative(){
		 return easy_;
	 }

	 void StorePrivate(const void* key, cppcomponents::use<InterfaceUnknown> v){
		 extra_info_[key] = v;
	 }
	 cppcomponents::use<InterfaceUnknown> GetPrivate(const void* key){
		 auto iter = extra_info_.find(key);
		 if (iter == extra_info_.end()){
			 return nullptr;
		 }
		 return iter->second;
	 }

	 bool RemovePrivate(const void* key){
		 return extra_info_.erase(key) > 0;
	 }



	 std::int32_t GetInt32Info(std::int32_t info){
		 long result{};
		 auto res = curl_easy_getinfo(easy_, static_cast<CURLINFO>(info), &result);
		 curl_throw_if_error(res);
		 return res;
	 }
	 double GetDoubleInfo(std::int32_t info){
		 double result{};
		 auto res = curl_easy_getinfo(easy_, static_cast<CURLINFO>(info), &result);
		 curl_throw_if_error(res);
		 return res;

	 }
	 cppcomponents::cr_string GetStringInfo(std::int32_t info){
		 char* result{};
		 auto res = curl_easy_getinfo(easy_, static_cast<CURLINFO>(info), &result);
		 curl_throw_if_error(res);
		 return cr_string{ result };

	 }
	 std::vector<std::string> GetListInfo(std::int32_t info){
		 curl_slist* result{};
		 auto res = curl_easy_getinfo(easy_, static_cast<CURLINFO>(info), &result);
		 curl_throw_if_error(res);
		 if (!result){
			 throw error_fail();
		 }
		 try{
			 std::vector<std::string> vec;
			 for (auto node = result; node != nullptr; node = node->next){
				 vec.push_back(node->data);
			 }
			 return vec;
		 }
		 catch (...){
			 curl_slist_free_all(result);
			 throw;
		 }
	 }



};

struct IImp :define_interface<cppcomponents::uuid<0xd9185c90, 0xd32e, 0x4909, 0xafaa, 0xc9a50557ae28>>
{
	void* GetImp();

	CPPCOMPONENTS_CONSTRUCT(IImp, GetImp);
};

inline std::string multi_id(){ return "cppcomponents_libcurl_libuv_dll!Multi"; }
typedef cppcomponents::runtime_class<multi_id, cppcomponents::object_interfaces<IMulti, IImp>> Multi_t;
typedef cppcomponents::use_runtime_class<Multi_t> Multi;


inline std::string response_id(){ return "cppcomponents_libcurl_libuv_dll!Response"; }
typedef cppcomponents::runtime_class<response_id, cppcomponents::object_interfaces<IResponse, IImp>,factory_interface<NoConstructorFactoryInterface>> Response_t;
typedef cppcomponents::use_runtime_class<Response_t> Response;


struct ImpResponse :implement_runtime_class<ImpResponse, Response_t>
{
	use<IEasy> easy_;
	std::vector<char> body_;
	std::vector<std::pair<std::string, std::string>> headers_;

	ImpResponse(use<IEasy> e) :easy_{ e }{}

	void AddToBody(const char* first, const char* last){
		body_.insert(body_.end(), first, last);
	}

	void AddToHeader(const char* first, const char* last){
		auto colon = std::find(first, last, ':');
		std::string name{ first, colon };
		if (colon != last){
			++colon;
		}
		std::string value{ colon, last };
		headers_.push_back(std::make_pair(name, value));
	}

	void* IImp_GetImp(){
		return this;
	}

	cppcomponents::use<IEasy> Request(){
		return easy_;
	}
	cppcomponents::cr_string Body(){
		if (!body_.size()){
			return cr_string{};
		}
		return cr_string{ &body_[0], body_.size() };
	}

	std::vector<std::pair<std::string, std::string>> Headers(){
		return headers_;
	}


};


struct ImpMulti :implement_runtime_class<ImpMulti, Multi_t>
{
	static const int pollid = 0;
	static const int responseid = 0;
	static const int promiseid = 0;

	CURLM* multi_;

	uv::Timer timeout_;

	static use<IEasy> ieasy_from_easy(CURL* easy){
		char* charpeasy = 0;
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &charpeasy);
		void* vpeasy = charpeasy;
		auto iunk = use<InterfaceUnknown>{reinterpret_portable_base<InterfaceUnknown>(static_cast<portable_base*>(vpeasy)), true};
		return iunk.QueryInterface<IEasy>();
	}

	static void curl_perform(use<uv::IPoll>, int status, int events, curl_socket_t sockfd, ImpMulti* pthis)
	{
		int running_handles;
		int flags = 0;
		char *done_url;
		CURLMsg *message;
		int pending;
		pthis->timeout_.Stop();

		if (events & uv::Constants::PollEvent::Readable)
			flags |= CURL_CSELECT_IN;
		if (events & uv::Constants::PollEvent::Writable)
			flags |= CURL_CSELECT_OUT;



		curl_multi_socket_action(pthis->multi_, sockfd, flags,
			&running_handles);

		while ((message = curl_multi_info_read(pthis->multi_,  &pending))) {


			switch (message->msg) {
			case CURLMSG_DONE:
				curl_easy_getinfo(message->easy_handle, CURLINFO_EFFECTIVE_URL,
					&done_url);
				printf("%s DONE\n", done_url);
				{
					auto easy = message->easy_handle;
					auto ieasy = ieasy_from_easy(easy);
				
					pthis->RemoveAndCallFuture(ieasy,message->data.result);

				}

				break;
			default:
				fprintf(stderr, "CURLMSG default\n");
				abort();
			}
		}
	}



	static void start_timeout(CURLM *multi, long timeout_ms, void *userp)
	{
		try{

			auto pthis = static_cast<ImpMulti*>(userp);
			auto& timeout = pthis->timeout_;
			if (timeout_ms <= 0)
				timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it in
								a bit */
			timeout.Start([multi](use<uv::ITimer>, int status){
				int running_handles;
				curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0,
					&running_handles);
			}, std::chrono::milliseconds{ timeout_ms });
		}
		catch (...){
			// swallow exceptions
			int running_handles;
			curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, CURL_CSELECT_ERR,
				&running_handles);
		}
	}

	static int handle_socket(CURL *easy, curl_socket_t s, int action, void *userp,
		void *)
	{
		try{


			auto pthis = static_cast<ImpMulti*>(userp);
			auto ieasy = ieasy_from_easy(easy);
			auto iunkpoll = ieasy.GetPrivate(&pollid);
			use<uv::IPoll> poll;
		
			if (action == CURL_POLL_IN || action == CURL_POLL_OUT) {
				if (!iunkpoll) {
					poll = uv::Poll{ s, false };
					ieasy.StorePrivate(&pollid, poll);
				}
			}

			switch (action) {
			case CURL_POLL_IN:
				using namespace std::placeholders;
				poll.Start(uv::Constants::PollEvent::Readable, std::bind(curl_perform, _1, _2, _3, s,pthis));
				break;
			case CURL_POLL_OUT:
				poll.Start(uv::Constants::PollEvent::Writable, std::bind(curl_perform, _1, _2, _3, s,pthis));


				break;
			case CURL_POLL_REMOVE:
				if (poll) {
					poll.Stop();
					ieasy.RemovePrivate(&pollid);
				}
				break;
			default:
				return -1;
			}

			return 0;

		}
		catch (...){
			return -1;
			// swallow exceptions
		}
	}

	ImpMulti() :multi_{ curl_multi_init() }{
		if (!multi_){
			throw error_fail();
		}
		curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, handle_socket);
		curl_multi_setopt(multi_, CURLMOPT_SOCKETDATA, static_cast<void*>(this));
		curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, start_timeout);
		curl_multi_setopt(multi_, CURLMOPT_TIMERDATA, static_cast<void*>(this));

	}

	void RemovePrivate(cppcomponents::use<IEasy>& easy){
			easy.RemovePrivate(&promiseid);
			easy.RemovePrivate(&pollid);
			easy.RemovePrivate(&responseid);
	}

	cppcomponents::Future<cppcomponents::use<IResponse>> Add(cppcomponents::use<IEasy> easy){
		// Store the promise
		try{

			auto promise = make_promise<use<IResponse>>();
			easy.StorePrivate(&promiseid, promise);

			//use<uv::IPoll> poll = uv::Poll{};
			//easy.StorePrivate(&pollid, poll);

			auto response = ImpResponse::create(easy);
			easy.StorePrivate(&responseid, response);

			auto res = curl_multi_add_handle(multi_, static_cast<CURL*>(easy.GetNative()));
			curl_throw_if_error(res);
			return promise.QueryInterface < IFuture<use<IResponse>> >();
		}
		catch (...){
			RemovePrivate(easy);
			throw;
		}

	}
	template<class I>
	use<I> GetPrivateSafe(use<IEasy>& easy, const void* key){
		auto iunk = easy.GetPrivate(key);
		if (!iunk){
			throw error_fail();
		}
		return iunk.QueryInterface<I>();
	}
	void RemoveAndCallFuture(use<IEasy> easy, CURLcode code){
		auto res = curl_multi_remove_handle(multi_, static_cast<CURL*>(easy.GetNative()));
		curl_throw_if_error(res);
		auto response = GetPrivateSafe<IResponse>(easy, &responseid);
		auto promise = GetPrivateSafe < IPromise<use<IResponse>>>(easy,&promiseid);
		RemovePrivate(easy);

		if (code != CURLE_OK){
			promise.SetError(-code);
		}
		else{
			promise.Set(response);
		}

	}
	void Remove(cppcomponents::use<IEasy> easy){
		RemoveAndCallFuture(easy, CURLE_OK);
	}
	void* GetNative(){
		return multi_;
	}




	void* IImp_GetImp(){
		return this;
	}
};



CPPCOMPONENTS_DEFINE_FACTORY()
