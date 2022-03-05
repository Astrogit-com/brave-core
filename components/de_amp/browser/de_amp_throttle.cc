/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_throttle.h"

#include <utility>

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "brave/components/de_amp/browser/de_amp_service.h"
#include "brave/components/de_amp/browser/de_amp_url_loader.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/navigation_entry.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace de_amp {

// static
std::unique_ptr<DeAmpThrottle> DeAmpThrottle::MaybeCreateThrottleFor(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    network::ResourceRequest request,
    const content::WebContents::Getter& wc_getter) {
  if (!service->IsEnabled()) {
    return nullptr;
  }
  return std::make_unique<DeAmpThrottle>(task_runner, service, request,
                                         wc_getter);
}

DeAmpThrottle::DeAmpThrottle(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    DeAmpService* service,
    network::ResourceRequest request,
    const content::WebContents::Getter& wc_getter)
    : task_runner_(task_runner),
      service_(service),
      request_(request),
      wc_getter_(wc_getter) {}

DeAmpThrottle::~DeAmpThrottle() = default;

void DeAmpThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  VLOG(2) << "deamp throttling: " << response_url;
  *defer = true;

  mojo::PendingRemote<network::mojom::URLLoader> new_remote;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_receiver;
  mojo::PendingRemote<network::mojom::URLLoader> source_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> source_client_receiver;
  DeAmpURLLoader* de_amp_loader;
  std::tie(new_remote, new_receiver, de_amp_loader) =
      DeAmpURLLoader::CreateLoader(weak_factory_.GetWeakPtr(), response_url,
                                   task_runner_, service_);
  InterceptAndStartLoader(
      std::move(source_loader), std::move(source_client_receiver),
      std::move(new_remote), std::move(new_receiver), de_amp_loader);
}

void DeAmpThrottle::Redirect(const GURL& new_url) {
  delegate_->CancelWithError(net::ERR_ABORTED);

  auto* contents = wc_getter_.Run();

  if (!contents)
    return;

  auto* entry = contents->GetController().GetVisibleEntry();
  if (!entry)
    return;

  content::OpenURLParams params(
      new_url,
      content::Referrer::SanitizeForRequest(new_url, entry->GetReferrer()),
      contents->GetMainFrame()->GetFrameTreeNodeId(),
      WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_CLIENT_REDIRECT,
      false);

  params.initiator_origin = request_.request_initiator;
  params.user_gesture = request_.has_user_gesture;

  // NavigationHandle will include all redirects that happened on the way to the
  // the current page in its redirect chain, including the current page itself
  // as the last entry. However OpenURLParams's redirect chain should only
  // include redirects that occurred before the current page. We need to remove
  // the last entry from the redirect chain when initializing the
  // OpenURLParams.
  auto redirect_chain = request_.navigation_redirect_chain;
  DCHECK(redirect_chain.size());
  redirect_chain.pop_back();
  params.redirect_chain = std::move(redirect_chain);

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents)
                         return;
                       web_contents->OpenURL(params);
                     },
                     contents->GetWeakPtr(), std::move(params)));
}

}  // namespace de_amp
