/*
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "WebViewImplementationNative.h"
#include "JNIHelpers.h"
#include <AK/Random.h>
#include <LibGfx/Bitmap.h>
#include <LibWebView/ViewImplementation.h>
#include <android/bitmap.h>
#include <jni.h>

namespace Ladybird {

WebViewImplementationNative::WebViewImplementationNative(jobject thiz)
    : m_java_instance(thiz)
{
    // NOTE: m_java_instance's global ref is controlled by the JNI bindings
    initialize_tab(Web::HTML::VisibilityState::Visible);

    on_ready_to_paint = [this]() {
        JavaEnvironment env(global_vm);
        env.get()->CallVoidMethod(m_java_instance, invalidate_layout_method);
    };

    on_load_start = [this]() {
        JavaEnvironment env(global_vm);
        auto url_string = env.jstring_from_ak_string(url().to_string());
        env.get()->CallVoidMethod(m_java_instance, on_load_start_method, url_string, JNI_FALSE);
        env.get()->DeleteLocalRef(url_string);
    };
}

void WebViewImplementationNative::paint_into_bitmap(void* android_bitmap_raw, AndroidBitmapInfo const& info)
{
    // Software bitmaps only for now!
    VERIFY((info.flags & ANDROID_BITMAP_FLAGS_IS_HARDWARE) == 0);
    VERIFY(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);

    auto* dst_bytes = static_cast<u8*>(android_bitmap_raw);
    auto fill_background = [&](Gfx::Color color) {
        u32 rgba = static_cast<u32>(color.red())
            | (static_cast<u32>(color.green()) << 8)
            | (static_cast<u32>(color.blue()) << 16)
            | (static_cast<u32>(color.alpha()) << 24);
        for (u32 y = 0; y < info.height; ++y) {
            auto* row = reinterpret_cast<u32*>(dst_bytes + y * info.stride);
            for (u32 x = 0; x < info.width; ++x)
                row[x] = rgba;
        }
    };

    fill_background(page_background_color());

    auto frame = backing_stores().displayed_frame();
    if (!frame.has_value() || !frame->shared_image_buffer)
        return;

    auto bitmap = frame->shared_image_buffer->bitmap_if_present();
    if (!bitmap)
        return;

    auto copy_width = min<u32>(info.width, static_cast<u32>(min(bitmap->width(), frame->size.width())));
    auto copy_height = min<u32>(info.height, static_cast<u32>(min(bitmap->height(), frame->size.height())));

    for (u32 y = 0; y < copy_height; ++y) {
        auto const* src_row = reinterpret_cast<u32 const*>(bitmap->scanline_u8(y));
        auto* dst_row = reinterpret_cast<u32*>(dst_bytes + y * info.stride);
        for (u32 x = 0; x < copy_width; ++x) {
            u32 bgra = src_row[x];
            u32 b = (bgra >> 0) & 0xFF;
            u32 g = (bgra >> 8) & 0xFF;
            u32 r = (bgra >> 16) & 0xFF;
            u32 a = (bgra >> 24) & 0xFF;
            dst_row[x] = r | (g << 8) | (b << 16) | (a << 24);
        }
    }
}

void WebViewImplementationNative::set_viewport_geometry(int w, int h)
{
    m_viewport_size = { w, h };
    handle_resize();
}

void WebViewImplementationNative::set_device_pixel_ratio(double f)
{
    m_device_pixel_ratio = f;
    handle_resize();
}

void WebViewImplementationNative::set_zoom_level(double f)
{
    m_zoom_level = f;
    update_zoom();
}

void WebViewImplementationNative::mouse_event(Web::MouseEvent::Type event_type, float x, float y, float raw_x, float raw_y)
{
    Gfx::IntPoint position = { x, y };
    Gfx::IntPoint screen_position = { raw_x, raw_y };
    auto event = Web::MouseEvent {
        event_type,
        position.to_type<Web::DevicePixels>(),
        screen_position.to_type<Web::DevicePixels>(),
        Web::UIEvents::MouseButton::Primary,
        Web::UIEvents::MouseButton::Primary,
        Web::UIEvents::KeyModifier::Mod_None,
        0,
        0,
        0,
        0,
        Web::WheelDeltaPrecision::Discrete,
        Web::ScrollGesturePhase::None,
        0,
        nullptr
    };

    enqueue_input_event(move(event));
}

}
