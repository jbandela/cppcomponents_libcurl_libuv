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


	easy.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, cppcomponents::make_delegate<Callbacks::WriteFunction>(writer_func));
	easy.SetPointerOption(Constants::Options::CURLOPT_URL, "https://www.google.com/");
	easy.SetPointerOption(Constants::Options::CURLOPT_CAINFO, "cacert.pem");
	easy.SetInt32Option(Constants::Options::CURLOPT_SSL_VERIFYHOST, 2);


	await(client.Fetch());


	exec.MakeLoopExit();
	std::cerr << "Completed successfully\n";
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



