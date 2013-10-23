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
		throw_if_error(static_cast<error_code>(code));
	}
}


inline void curl_throw_if_error(CURLcode code){
	if (code != CURLE_OK){
		throw_if_error(static_cast<error_code>(code));
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

	std::map < void*, use<InterfaceUnknown> > extra_info_;



	ImpEasy() 
		:
		easy_{ curl_easy_init() }
	{
		if (!easy_){
			throw error_fail();
		}

		// Set the private of easy to interface
		auto res = curl_easy_setopt(easy_, CURLOPT_PRIVATE,this->get_unknown_portable_base() );
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
		if ( option == CURLOPT_WRITEDATA ||
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

	void StorePrivate(void* key, cppcomponents::use<InterfaceUnknown> v){
		extra_info_[key] = v;
	}
	cppcomponents::use<InterfaceUnknown> GetPrivate(void* key){
		auto iter = extra_info_.find(key);
		if (iter == extra_info_.end()){
			return nullptr;
		}
		return iter->second;
	}

	bool RemovePrivate(void* key){
		return extra_info_.erase(key) > 0;
	}



};



CPPCOMPONENTS_DEFINE_FACTORY()
