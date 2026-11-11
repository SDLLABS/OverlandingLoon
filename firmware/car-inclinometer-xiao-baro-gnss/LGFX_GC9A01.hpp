#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI _bus;

public:

    LGFX(void)
    {
        {
            auto cfg = _bus.config();

            cfg.spi_host = SPI2_HOST;

            cfg.spi_mode = 0;

            cfg.freq_write = 40000000; // Conservative clock for XIAO SPI GPIO matrix
            cfg.freq_read  = 16000000;

            cfg.spi_3wire = false;

            cfg.use_lock = true;

            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.pin_sclk = D8;  // XIAO ESP32-S3 GPIO7
            cfg.pin_mosi = D10; // XIAO ESP32-S3 GPIO9
            cfg.pin_miso = -1; // GC9A01 write-only; D9 is available for microSD

            cfg.pin_dc = D3;   // XIAO ESP32-S3 GPIO4

            _bus.config(cfg);

            _panel.setBus(&_bus);
        }

        {
            auto cfg = _panel.config();

            cfg.pin_cs = D1;   // XIAO ESP32-S3 GPIO2
            cfg.pin_rst = -1;   // Reset not exposed to a dedicated XIAO GPIO
            cfg.pin_busy = -1;

            cfg.panel_width = 240;
            cfg.panel_height = 240;

            cfg.offset_x = 0;
            cfg.offset_y = 0;

            cfg.offset_rotation = 0;

            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;

            cfg.readable = false;
            cfg.invert = true;

            cfg.rgb_order = false;

            cfg.dlen_16bit = false;

            cfg.bus_shared = true; // Round Display SPI lines are shared with microSD

            _panel.config(cfg);
        }

        setPanel(&_panel);
    }
};