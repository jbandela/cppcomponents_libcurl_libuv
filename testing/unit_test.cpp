// Leak check detection with MSVC
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

namespace{
	struct MemLeakCheckInit{
		MemLeakCheckInit(){
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			//_crtBreakAlloc = 310;
		}
	};

	MemLeakCheckInit mlcinit;
}
#endif
#include <cppcomponents_libcurl_libuv/cppcomponents_libcurl_libuv.hpp>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>
#include <cppcomponents_libuv/cppcomponents_libuv.hpp>
#include <cppcomponents/loop_executor.hpp>

#include <iostream>

	using namespace cppcomponents_libcurl_libuv;

	auto exec = cppcomponents::implement_loop_executor::create().QueryInterface<cppcomponents::ILoopExecutor>();

int async_main(cppcomponents::awaiter await){

	auto promise = cppcomponents::make_promise<void>();
	Easy easy;
	cppcomponents::use<IEasy> ieasy = easy;
	auto writer_func = [ieasy](char* p, std::size_t n, std::size_t nmemb) -> std::size_t{
		std::string s(p, p + (n*nmemb));
		std::cerr << s;
		return s.size();
	};

	auto done_func = [promise](cppcomponents::use<IEasy> easy, std::int32_t ec)mutable{
		if (ec != Constants::Errors::CURLE_OK){
			std::cout << "Error: " << easy.GetErrorDescription() << "\n";
		}
		promise.Set();
	};
	easy.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, cppcomponents::make_delegate<Callbacks::WriteFunction>(writer_func));
	easy.SetPointerOption(Constants::Options::CURLOPT_URL, "https://www.google.com/");
	easy.SetPointerOption(Constants::Options::CURLOPT_CAINFO, "cacert.pem");
	easy.SetInt32Option(Constants::Options::CURLOPT_SSL_VERIFYHOST, 2);
	Curl::DefaultMulti().Add(easy, cppcomponents::make_delegate<Callbacks::CompletedFunction>(done_func));


	await(exec,promise.QueryInterface < cppcomponents::IFuture<void>>());
	easy.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, nullptr);

	Curl::DefaultMulti().Add(easy, nullptr);

	exec.MakeLoopExit();
	return 0;

}

int main(){
	//cppcomponents::use<IMulti> multi = cppcomponents_libcurl_libuv::Curl::DefaultMulti();
	auto am = cppcomponents::resumable(async_main);
	am();
	exec.Loop();
	return 0;
}



