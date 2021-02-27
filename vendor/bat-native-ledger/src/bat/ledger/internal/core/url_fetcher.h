/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_

#include <string>
#include <utility>

#include "base/values.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

class URLRequest {
 public:
  void SetBody(const std::string& content, const std::string& content_type);

  void SetBody(const base::Value& value);

  void AddHeader(const std::string& name, const std::string& value);

  // TODO(zenparsing): Flags, logging options

  inline static URLRequest Get(const std::string& url) {
    return URLRequest(mojom::UrlMethod::GET, url);
  }

  inline static URLRequest Post(const std::string& url) {
    return URLRequest(mojom::UrlMethod::POST, url);
  }

  inline static URLRequest Delete(const std::string& url) {
    return URLRequest(mojom::UrlMethod::DEL, url);
  }

  inline static URLRequest Put(const std::string& url) {
    return URLRequest(mojom::UrlMethod::PUT, url);
  }

  const mojom::UrlRequest& req() const { return req_; }

 private:
  URLRequest(mojom::UrlMethod method, const std::string& url);

  mojom::UrlRequest req_;
};

class URLResponse {
 public:
  explicit URLResponse(mojom::UrlResponsePtr response);
  ~URLResponse();

  URLResponse(const URLResponse& other) = delete;
  URLResponse& operator=(const URLResponse& other) = delete;

  URLResponse(URLResponse&& other);
  URLResponse& operator=(URLResponse&& other);

  int status_code() const { return resp_->status_code; }

  bool Succeeded() const;

  base::Value ReadBodyAsJSON() const;

  std::string ReadBodyAsText() const;

 private:
  mojom::UrlResponsePtr resp_;
};

// Allows fetching of URLs from the network.
//
// Example:
//   context()
//       .Get<URLFetcher>()
//       .Fetch(url_request)
//       .Then(base::BindOnce(...));
class URLFetcher : public BATLedgerContext::Object {
 public:
  static const char kContextKey[];

  // TODO(zenparsing): Revisit the need for this structure. How are these
  // options supposed to be used anyway? Can they just be on URLRequest?
  struct FetchOptions {
    bool log_response_body = false;
    bool disable_logging = false;
  };

  Future<URLResponse> Fetch(const URLRequest& request);

  Future<URLResponse> Fetch(const URLRequest& request, FetchOptions options);

  template <typename T, typename... Args>
  auto FetchEndpoint(Args&&... args) {
    using R =
        std::invoke_result_t<decltype(&T::MapResponse), T*, const URLResponse&>;

    auto request = context().Get<T>().MapRequest(std::forward<Args>(args)...);

    return Future<R>::Create([&](auto resolver) {
      Fetch(request).Then(base::BindOnce(
          [](base::WeakPtr<BATLedgerContext> context,
             decltype(resolver) resolver, URLResponse response) {
            if (context) {
              resolver.Complete(context->Get<T>().MapResponse(response));
            }
          },
          context().GetWeakPtr(), resolver));
    });
  }

 protected:
  virtual Future<URLResponse> FetchImpl(const URLRequest& request,
                                        FetchOptions options);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_
