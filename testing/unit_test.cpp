#include <cppcomponents_libcurl_libuv/cppcomponents_libcurl_libuv.hpp>
#include <cppcomponents_async_coroutine_wrapper/cppcomponents_resumable_await.hpp>
#include <cppcomponents_libuv/cppcomponents_libuv.hpp>

#include <iostream>

	using namespace cppcomponents_libcurl_libuv;

int async_main(cppcomponents::awaiter await){
	cppcomponents_libuv::LoopExiter exiter{};

	auto promise = cppcomponents::make_promise<void>();
	Easy easy;
	cppcomponents::use<IEasy> ieasy = easy;
	auto writer_func = [ieasy](char* p, std::size_t n, std::size_t nmemb) -> std::size_t{
		std::string s(p, p + (n*nmemb));
		std::cout << s;
		return s.size();
	};

	auto done_func = [promise](cppcomponents::use<IEasy>, std::int32_t ec)mutable{
		promise.Set();
	};
	easy.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, cppcomponents::make_delegate<Callbacks::WriteFunction>(writer_func));
	easy.SetPointerOption(Constants::Options::CURLOPT_URL, "http://www.google.com/");
	Curl::DefaultMulti().Add(easy, cppcomponents::make_delegate<Callbacks::CompletedFunction>(done_func));


	await(promise.QueryInterface < cppcomponents::IFuture<void>>());

	return 0;

}

int main(){
	//cppcomponents::use<IMulti> multi = cppcomponents_libcurl_libuv::Curl::DefaultMulti();
	auto am = cppcomponents::resumable(async_main);
	am();
	cppcomponents_libuv::Uv::DefaultExecutor().Loop();
	return 0;
}



