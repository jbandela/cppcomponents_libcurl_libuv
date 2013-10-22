#include "cppcomponents_libcurl_libuv.hpp"
#include "implementation/constants.hpp"
#include <curl/curl.h>

#include <cppcomponents_libuv/cppcomponents_libuv.hpp>


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
