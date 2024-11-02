#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <limits>
#include <vector>
#include <iostream>
#include "evdi.hpp"

Evdi::LogHandler Evdi::log_handler = [](const std::string&) {};

unsigned char Evdi::sample_edid[] = {
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4e, 0x8d, 0x54, 0x24,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x16, 0x01, 0x04, 0xa5, 0x66, 0x39, 0x78,
  0x22, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0xbf,
  0xef, 0x80, 0x81, 0x80, 0x81, 0x40, 0x71, 0x4f, 0x81, 0x00, 0xb3, 0x00,
  0x95, 0x00, 0x95, 0x0f, 0xa9, 0x40, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,
  0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0xfa, 0x3c, 0x32, 0x00, 0x00, 0x1e,
  0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1b, 0x30, 0x40, 0x70, 0x36, 0x00,
  0xfa, 0x3c, 0x32, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x38,
  0x4b, 0x1e, 0x51, 0x11, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x00, 0x00, 0x00, 0xfc, 0x00, 0x41, 0x74, 0x68, 0x65, 0x6e, 0x61, 0x44,
  0x50, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x01, 0x35, 0x02, 0x03, 0x12, 0xc1,
  0x45, 0x90, 0x1f, 0x04, 0x13, 0x03, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01,
  0x00, 0x00, 0x02, 0x3a, 0x80, 0xd0, 0x72, 0x38, 0x2d, 0x40, 0x10, 0x2c,
  0x45, 0x80, 0xfa, 0x3c, 0x32, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72,
  0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xfa, 0x3c, 0x32, 0x00,
  0x00, 0x1e, 0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28,
  0x55, 0x40, 0xfa, 0x3c, 0x32, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x9d
};
Evdi::Evdi()
{
    evdi_set_logging({ .function = &Evdi::dispatch_log, .user_data = this });
    handle = evdi_open_attached_to(NULL);
}

Evdi::Evdi(int devnum)
{
    evdi_set_logging({ .function = &Evdi::dispatch_log, .user_data = this });
    handle = evdi_open(devnum);
}

Evdi::Evdi(const char* parent)
{
    evdi_set_logging({ .function = &Evdi::dispatch_log, .user_data = this });
    handle = evdi_open_attached_to(parent);
}

Evdi::~Evdi() { evdi_close(handle); }

Evdi::operator bool() const { return handle != EVDI_INVALID_HANDLE; }

int Evdi::get_event_source() const { return evdi_get_event_ready(handle); }

void Evdi::handle_events(evdi_event_context* context) const
{
    evdi_handle_events(handle, context);
}

void Evdi::enable_cursor_events() const
{
    evdi_enable_cursor_events( handle, true);
}

void Evdi::connect(const unsigned char* edid,
    const unsigned edid_length) const
{
    printf("\n%s\n",__FUNCTION__);
    if (edid) {
        evdi_connect(handle, edid, edid_length,
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max());
    } else {
        evdi_connect(handle, sample_edid, 256,
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max());
    }
}

void Evdi::disconnect() const { evdi_disconnect(handle); }

void Evdi::register_buffer(evdi_buffer buffer) const
{
    evdi_register_buffer(handle, buffer);
}

void Evdi::unregister_buffer(int buffer_id) const
{
    evdi_unregister_buffer(handle, buffer_id);
}

bool Evdi::request_update(int buffer_id) const
{
    return evdi_request_update(handle, buffer_id);
}

void Evdi::grab_pixels(evdi_rect* rects, int* num_rects) const
{
    evdi_grab_pixels(handle, rects, num_rects);
}

void Evdi::dispatch_log(void* user_data, const char* fmt, ...)
{
    constexpr size_t MAX_LEN = 255;
    static std::vector<char> buffer(MAX_LEN, '\0');

    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer.data(), MAX_LEN, fmt, args);
    va_end(args);

    log_handler(std::string(buffer.data()));
}