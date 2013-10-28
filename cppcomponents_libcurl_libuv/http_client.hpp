#pragma once
#ifndef INCLUDE_GUARD_CPPCOMPONENTS_LIBCURL_LIBUV_HTTP_CLIENT_HPP_10_21_2013_
#define INCLUDE_GUARD_CPPCOMPONENTS_LIBCURL_LIBUV_HTTP_CLIENT_HPP_10_21_2013_


#include "cppcomponents_libcurl_libuv.hpp"

namespace cppcomponents_libcurl_libuv{


	struct Request{

		std::string Url;
		std::string Method;
		std::vector<std::pair<std::string, std::string>> Headers;
		std::string Body;
		std::string Username;
		std::string Password;
		std::string AuthMode;
		float ConnectTimeout;
		float RequestTimeout;

		bool FollowRedirects;
		int MaxRedirects;
		std::string UserAgent;
		bool UseGzip;
		std::string NetworkInterface;
		std::string ProxyHost;
		int ProxyPort;
		std::string ProxyUsername;
		std::string ProxyPassword;
		bool AllowNowStandardMethods;
		bool ValidateCert;
		std::string CACerts;
		bool AllowIPv6;
		std::string ClientKey;
		std::string ClientCert;


		cppcomponents::Channel<cppcomponents::use<cppcomponents::IBuffer>> StreamingChannel;
		cppcomponents::Channel<cppcomponents::use<cppcomponents::IBuffer>> HeaderChannel;
		cppcomponents::Channel<std::tuple<double, double, double, double>> ProgressChannel;



		Request(){ Initialize(); }
		Request(const std::string& url) :Url{ url }{ Initialize(); }
		Request(const char* url) :Url{ url }{ Initialize(); }

	private:
		void Initialize(){
			CACerts = "cacert.pem";
		}
	};
	struct HttpClient{
	private:

		cppcomponents::use<IMulti> multi_;
		Easy easy_;

		Response response_;

		void HandleMethod(const Request& req){
			// Default to GET
			if (req.Method.size() == 0 || req.Method == "GET"){
				easy_.SetInt32Option(Constants::Options::CURLOPT_HTTPGET, 1);
			}
			else if (req.Method == "PUT"){
				auto body = req.Body;
				easy_.SetInt32Option(Constants::Options::CURLOPT_UPLOAD, 1);
				easy_.SetInt32Option(Constants::Options::CURLOPT_INFILESIZE, body.size());
				std::size_t pos = 0;
				easy_.SetFunctionOption(Constants::Options::CURLOPT_READFUNCTION,
					cppcomponents::make_delegate<Callbacks::ReadFunction>([body, pos](void* ptr, std::size_t size,
					std::size_t nmemb)mutable -> std::size_t{
					if (pos >= body.size()){
						return 0;
					}
					auto cptr = static_cast<char*>(ptr);
					auto count = size*nmemb;
					auto remaining = body.size() - pos;
					auto begin = &body[0];
					auto end = begin + body.size();;
					if (remaining <= count){
						std::copy(begin + pos, end, cptr);
						pos = body.size();
						return remaining;
					}
					else{
						std::copy(begin + pos, begin + pos + count, cptr);
						pos += count;
						return count;
					}

				}));
			}

			else if (req.Method == "POST"){
				auto body = req.Body;
				easy_.SetInt32Option(Constants::Options::CURLOPT_POST, 1);
				easy_.SetInt32Option(Constants::Options::CURLOPT_POSTFIELDSIZE, body.size());
				char* begin = 0;
				if (body.size()){
					begin = const_cast<char*>(&body[0]);
				}
				easy_.SetPointerOption(Constants::Options::CURLOPT_COPYPOSTFIELDS, begin);


			}
			else if (req.Method == "DELETE"){
				easy_.SetPointerOption(Constants::Options::CURLOPT_CUSTOMREQUEST, "DELETE");


			}
			else if (req.Method == "HEAD"){
				easy_.SetInt32Option(Constants::Options::CURLOPT_HTTPGET, 1);
				easy_.SetInt32Option(Constants::Options::CURLOPT_NOBODY, 1);


			}
			else{
				throw cppcomponents::error_invalid_arg();
			}
		}
		void HandleCertificates(const Request& req){
			easy_.SetPointerOption(Constants::Options::CURLOPT_CAINFO, const_cast<char*>(req.CACerts.c_str()));

		}
		static void CleanupCallbacks(cppcomponents::use<IEasy> easy){
			easy.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, nullptr);
			easy.SetFunctionOption(Constants::Options::CURLOPT_READFUNCTION, nullptr);
			easy.SetFunctionOption(Constants::Options::CURLOPT_HEADERFUNCTION, nullptr);
			easy.SetFunctionOption(Constants::Options::CURLOPT_PROGRESSFUNCTION, nullptr);
		}
	public:
		HttpClient(cppcomponents::use<IMulti> m) :multi_{ m }, easy_{}, response_{ easy_ }
		{}
		HttpClient() :HttpClient{ Curl::DefaultMulti() }
		{}


		cppcomponents::use<IEasy> GetEasy(){
			return easy_;
		}

		cppcomponents::Future<cppcomponents::use<IResponse>> Fetch(){
			auto promise = cppcomponents::make_promise<cppcomponents::use<IResponse>>();
			auto easy = easy_;
			cppcomponents::use<IResponse> response = response_;
			auto completed = [easy, promise, response](cppcomponents::use<IEasy>, std::int32_t ec)mutable{
				try{
					CleanupCallbacks(easy);
					if (ec != Constants::Errors::CURLE_OK){
						auto rw = response.QueryInterface<IResponseWriter>();
						rw.SetError(ec);
						
					}
					promise.Set(response);
				}
				catch (std::exception& e)
				{
					//try{
						promise.SetError(cppcomponents::error_mapper::error_code_from_exception(e));
					//}
					//catch (...){}
				}
			};
			multi_.Add(easy_, cppcomponents::make_delegate<Callbacks::CompletedFunction>(completed))
				.Then([promise,easy](cppcomponents::Future<void> f)mutable{
				if (f.ErrorCode() < 0){
					CleanupCallbacks(easy);
					promise.SetError(f.ErrorCode());
				}
			});

			return promise.QueryInterface < cppcomponents::IFuture<cppcomponents::use<IResponse>> >();

		}

		cppcomponents::Future<cppcomponents::use<IResponse>> Fetch(const Request& req){
			easy_.Reset();
			if (!req.Url.size()){ throw cppcomponents::error_invalid_arg(); }
			easy_.SetPointerOption(Constants::Options::CURLOPT_URL, const_cast<char*>(req.Url.c_str()));
			cppcomponents::use<Callbacks::WriteFunction> writer_func;
			if (req.StreamingChannel){
				auto chan = req.StreamingChannel;
				 auto func = [chan](char* p, std::size_t n, std::size_t nmemb) mutable -> std::size_t{
					auto sz = n*nmemb;
					auto buffer = cppcomponents::Buffer::Create(sz);
					buffer.SetSize(sz);
					std::copy(p, p + sz, buffer.Begin());
					chan.Write(buffer);
					return sz;
				};

				 writer_func = cppcomponents::make_delegate<Callbacks::WriteFunction>(func);

			}
			else{
				auto response_writer = response_.as<IResponseWriter>();
				 auto func = [response_writer](char* p, std::size_t n, std::size_t nmemb) mutable -> std::size_t{
					auto sz = n*nmemb;
					response_writer.AddToBody(p, p + sz);
					return sz;
				};
				 writer_func = cppcomponents::make_delegate<Callbacks::WriteFunction>(func);
			}

			easy_.SetFunctionOption(Constants::Options::CURLOPT_WRITEFUNCTION, writer_func);
			HandleCertificates(req);
			return Fetch();
		}



	};

}


#endif