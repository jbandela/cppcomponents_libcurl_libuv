// Leak check detection with MSVC
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

namespace{
	struct MemLeakCheckInit{
		MemLeakCheckInit(){
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			//_crtBreakAlloc = 437;
		}
	};

	MemLeakCheckInit mlcinit;
}
#endif
#include <cppcomponents_libcurl_libuv/http_client.hpp>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>
#include <cppcomponents_libuv/cppcomponents_libuv.hpp>
#include <cppcomponents/loop_executor.hpp>

#include <iostream>

	using namespace cppcomponents_libcurl_libuv;

	auto exec = cppcomponents::implement_loop_executor::create().QueryInterface<cppcomponents::ILoopExecutor>();

int async_main(cppcomponents::awaiter await){

	cppcomponents_libcurl_libuv::HttpClient client;
	auto easy = client.GetEasy();
	cppcomponents::use<IEasy> ieasy = easy;
	auto writer_func = [ieasy](char* p, std::size_t n, std::size_t nmemb) -> std::size_t{
		std::string s(p, p + (n*nmemb));
		//std::cerr << s;
		return s.size();
	};


	cppcomponents_libcurl_libuv::Request req("https://www.google.com/");
	req.CACerts = "cacert.pem";
	auto response = await(client.Fetch(req));
	std::string str;
	if (response.ErrorCode() < 0){
		str = "Error: " + response.ErrorMessage().to_string();
	}
	else{
		str = response.Body().to_string();
	}

	exec.MakeLoopExit();
	std::cerr << str<< "\n";
	return 0;

}

int main(){
	new char[50];
	auto am = cppcomponents::resumable(async_main);
	am();
	exec.Loop();
	std::cerr << "Completed successfully\n";
	return 0;
}



