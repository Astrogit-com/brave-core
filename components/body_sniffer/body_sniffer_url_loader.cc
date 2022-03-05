/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/body_sniffer/body_sniffer_url_loader.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/body_sniffer/body_sniffer_throttle.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace body_sniffer {

BodySnifferURLLoader::BodySnifferURLLoader(
    base::WeakPtr<body_sniffer::BodySnifferThrottle> throttle,
    const GURL& response_url,
    mojo::PendingRemote<network::mojom::URLLoaderClient>
        destination_url_loader_client,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : throttle_(throttle),
      response_url_(response_url),
      destination_url_loader_client_(std::move(destination_url_loader_client)),
      task_runner_(task_runner),
      body_consumer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             task_runner),
      body_producer_watcher_(FROM_HERE,
                             mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                             std::move(task_runner)) {}

BodySnifferURLLoader::~BodySnifferURLLoader() = default;

void BodySnifferURLLoader::Start(
    mojo::PendingRemote<network::mojom::URLLoader> source_url_loader_remote,
    mojo::PendingReceiver<network::mojom::URLLoaderClient>
        source_url_client_receiver,
    mojo::ScopedDataPipeConsumerHandle body) {
  source_url_loader_.Bind(std::move(source_url_loader_remote));
  source_url_client_receiver_.Bind(std::move(source_url_client_receiver),
                                   task_runner_);
  if (body) {
    OnStartLoadingResponseBody(std::move(body));
  }
}

void BodySnifferURLLoader::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {}

void BodySnifferURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr response_head,
    mojo::ScopedDataPipeConsumerHandle body) {
  // OnReceiveResponse() shouldn't be called because BodySnifferURLLoader is
  // created by WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void BodySnifferURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr response_head) {
  // OnReceiveRedirect() shouldn't be called because BodySnifferURLLoader is
  // created by WillProcessResponse(), which is equivalent
  // to OnReceiveResponse().
  NOTREACHED();
}

void BodySnifferURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  destination_url_loader_client_->OnUploadProgress(current_position, total_size,
                                                   std::move(ack_callback));
}

void BodySnifferURLLoader::OnReceiveCachedMetadata(mojo_base::BigBuffer data) {
  destination_url_loader_client_->OnReceiveCachedMetadata(std::move(data));
}

void BodySnifferURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  destination_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void BodySnifferURLLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  VLOG(2) << __func__ << " " << response_url_;
  state_ = State::kLoading;
  body_consumer_handle_ = std::move(body);
  body_consumer_watcher_.Watch(
      body_consumer_handle_.get(),
      MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&BodySnifferURLLoader::OnBodyReadable,
                          base::Unretained(this)));
  body_consumer_watcher_.ArmOrNotify();
}

void BodySnifferURLLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  DCHECK(!complete_status_.has_value());
  switch (state_) {
    case State::kWaitForBody:
      // An error occured before receiving any data.
      DCHECK_NE(net::OK, status.error_code);
      state_ = State::kCompleted;
      if (!throttle_) {
        Abort();
        return;
      }
      throttle_->Resume();
      destination_url_loader_client_->OnComplete(status);
      return;
    case State::kLoading:
    case State::kSending:
      complete_status_ = status;
      return;
    case State::kCompleted:
      destination_url_loader_client_->OnComplete(status);
      return;
    case State::kAborted:
      NOTREACHED();
      return;
  }
  NOTREACHED();
}

void BodySnifferURLLoader::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  // BodySnifferURLLoader starts handling the request after
  // OnReceivedResponse(). A redirect response is not expected.
  NOTREACHED();
  source_url_loader_->FollowRedirect(removed_headers, modified_headers, modified_cors_exempt_headers, new_url);
}

void BodySnifferURLLoader::SetPriority(net::RequestPriority priority,
                                       int32_t intra_priority_value) {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->SetPriority(priority, intra_priority_value);
}

void BodySnifferURLLoader::PauseReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->PauseReadingBodyFromNet();
}

void BodySnifferURLLoader::ResumeReadingBodyFromNet() {
  if (state_ == State::kAborted)
    return;
  source_url_loader_->ResumeReadingBodyFromNet();
}

bool BodySnifferURLLoader::CheckBufferedBody(uint32_t readBufferSize) {
  size_t start_size = buffered_body_.size();
  uint32_t read_bytes = readBufferSize;
  buffered_body_.resize(start_size + read_bytes);

  auto result = body_consumer_handle_->ReadData(
      &buffered_body_[0] + start_size, &read_bytes, MOJO_READ_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      return true;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // Reading is finished.
      buffered_body_.resize(start_size);
      CompleteLoading(std::move(buffered_body_));
      break;
    case MOJO_RESULT_SHOULD_WAIT:
      body_consumer_watcher_.ArmOrNotify();
      break;
    default:
      NOTREACHED();
  }
  return false;
}

void BodySnifferURLLoader::CompleteLoading(std::string body) {
  DCHECK_EQ(State::kLoading, state_);
  state_ = State::kSending;

  if (!throttle_) {
    Abort();
    return;
  }

  buffered_body_ = std::move(body);
  bytes_remaining_in_buffer_ = buffered_body_.size();

  throttle_->Resume();
  mojo::ScopedDataPipeConsumerHandle body_to_send;
  MojoResult result =
      mojo::CreateDataPipe(nullptr, body_producer_handle_, body_to_send);
  if (result != MOJO_RESULT_OK) {
    Abort();
    return;
  }
  // Set up the watcher for the producer handle.
  body_producer_watcher_.Watch(
      body_producer_handle_.get(),
      MOJO_HANDLE_SIGNAL_WRITABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::BindRepeating(&BodySnifferURLLoader::OnBodyWritable,
                          base::Unretained(this)));

  // Send deferred message.
  destination_url_loader_client_->OnStartLoadingResponseBody(
      std::move(body_to_send));

  DCHECK(bytes_remaining_in_buffer_);
  if (bytes_remaining_in_buffer_) {
    SendReceivedBodyToClient();
    return;
  }

  CompleteSending();
}

void BodySnifferURLLoader::SendReceivedBodyToClient() {
  DCHECK_EQ(State::kSending, state_);
  // Send the buffered data first.
  DCHECK_GT(bytes_remaining_in_buffer_, 0u);
  size_t start_position = buffered_body_.size() - bytes_remaining_in_buffer_;
  uint32_t bytes_sent = bytes_remaining_in_buffer_;
  MojoResult result =
      body_producer_handle_->WriteData(buffered_body_.data() + start_position,
                                       &bytes_sent, MOJO_WRITE_DATA_FLAG_NONE);
  switch (result) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // The pipe is closed unexpectedly. |this| should be deleted once
      // URLLoaderPtr on the destination is released.
      Abort();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
      body_producer_watcher_.ArmOrNotify();
      return;
    default:
      NOTREACHED();
      return;
  }
  bytes_remaining_in_buffer_ -= bytes_sent;
  body_producer_watcher_.ArmOrNotify();
}

void BodySnifferURLLoader::Abort() {
  VLOG(2) << __func__ << " " << response_url_;
  state_ = State::kAborted;
  body_consumer_watcher_.Cancel();
  body_producer_watcher_.Cancel();
  source_url_loader_.reset();
  source_url_client_receiver_.reset();
  destination_url_loader_client_.reset();
  // |this| should be removed since the owner will destroy |this| or the owner
  // has already been destroyed by some reason.
}

}  // namespace body_sniffer
