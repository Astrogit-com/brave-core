/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_

#include <memory>
#include <utility>

#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

#include "brave/browser/translate/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"

class BraveTranslateIconView;

// The purpose of this subclass is to repurpose the translate bubble to install
// google translate extension. Functions are only overridden by the subclass if
// extensions are enabled and ENABLE_BRAVE_TRANSLATE (go-translate) is not
// enabled.
class BraveTranslateBubbleView : public TranslateBubbleView {
 public:
  BraveTranslateBubbleView(views::View* anchor_view,
                           std::unique_ptr<TranslateBubbleModel> model,
                           translate::TranslateErrors::Type error_type,
                           content::WebContents* web_contents);
  ~BraveTranslateBubbleView() override;

#if !BUILDFLAG(ENABLE_BRAVE_TRANSLATE) && BUILDFLAG(ENABLE_EXTENSIONS)
  // views::BubbleDialogDelegateView methods.
  void Init() override;
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::View methods.
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
#endif

 private:
#if !BUILDFLAG(ENABLE_BRAVE_TRANSLATE) && BUILDFLAG(ENABLE_EXTENSIONS)
  views::View* BraveCreateViewBeforeTranslate();
  void InstallGoogleTranslate();
#endif

  DISALLOW_COPY_AND_ASSIGN(BraveTranslateBubbleView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_
