/*
 * Copyright (c) 2023, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/XMLHttpRequestUploadPrototype.h>
#include <LibWeb/XHR/XMLHttpRequestUpload.h>

namespace Web::XHR {

XMLHttpRequestUpload::XMLHttpRequestUpload(JS::Realm& realm)
    : XMLHttpRequestEventTarget(realm)
{
}

XMLHttpRequestUpload::~XMLHttpRequestUpload() = default;

JS::ThrowCompletionOr<void> XMLHttpRequestUpload::initialize(JS::Realm& realm)
{
    MUST_OR_THROW_OOM(Base::initialize(realm));
    set_prototype(&Bindings::ensure_web_prototype<Bindings::XMLHttpRequestUploadPrototype>(realm, "XMLHttpRequestUpload"));

    return {};
}

}
