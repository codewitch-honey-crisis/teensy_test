#include <Arduino.h>
#include <Wire.h>
#include <tft_io.hpp>
#include <ili9341.hpp>
#include <gfx_cpp14.hpp>
#include "Telegrama.hpp"
using namespace arduino;
using namespace gfx;
#ifdef CORE_TEENSY
using bus_t = tft_spi<0,10>;
using lcd_t = ili9341<8,9,7,bus_t,3,true,400,200>;
#elif defined(ESP32)
// ESP_WROVER_KIT 4.1
using bus_t = tft_spi_ex<VSPI,22,23,25,19>;
using lcd_t = ili9341<21,18,5,bus_t,1,false,400,200>;
#endif

using color_t = color<typename lcd_t::pixel_type>;
lcd_t lcd;
uint32_t fps_ts;
int fps_count;
// true to draw a gradient, false for
// a checkerboard
const bool gradient = true;
// true to use async batching
const bool async = true;
// the text
const char* text = "hello world!";
// the text height in pixels
const uint16_t text_height = 30;
// the font
const open_font& text_font = Telegrama;

// globals
float hue;
srect16 text_rect;
float text_scale;

void setup() {
  Serial.begin(115200);
  // premeasure the text and center it
  // so we don't have to every time
  text_scale = text_font.scale(text_height);
  text_rect = text_font.measure_text(ssize16::max(),
                                    spoint16::zero(),
                                    text,
                                    text_scale).
                                      bounds().
                                      center((srect16)lcd.
                                                      bounds());
  fps_count = 0;
  fps_ts = millis()+1000;
}


void loop() {
  ++fps_count;
  // current background color
  hsv_pixel<24> px(true,hue,1,1);
  
  // use batching here
  auto ba = (async)?draw::batch_async(lcd,lcd.bounds()):
                  draw::batch(lcd,lcd.bounds());
  
  if(gradient) {
    // draw a gradient
    for(int y = 0;y<lcd.dimensions().height;++y) {
      px.template channelr<channel_name::S>(((double)y)/lcd.bounds().y2);
      for(int x = 0;x<lcd.dimensions().width;++x) {
        px.template channelr<channel_name::V>(((double)x)/lcd.bounds().x2);
        ba.write(px);
      }
    }
  } else {
    // draw a checkerboard pattern
    for(int y = 0;y<lcd.dimensions().height;y+=16) {
      for(int yy=0;yy<16;++yy) {
        for(int x = 0;x<lcd.dimensions().width;x+=16) {
          for(int xx=0;xx<16;++xx) {
            if (0 != ((x + y) % 32)) {
              ba.write(px);
            } else {
              ba.write(color_t::white);
            }
          }
        }
      }
    }
  }
  // commit what we wrote
  ba.commit();
  // offset the hue by half the total hue range
  float hue2 = hue+.5;
  if(hue2>1.0) {
    hue2-=1.0;
  }
  px=hsv_pixel<24>(true,hue2,1,1);
  // convert the HSV pixel to RGBA because HSV doesn't antialias
  // and so we can alpha blend
  rgba_pixel<32> px2,px3;
  convert(px,&px2);
  // set the alpha channel tp 75%
  px2.channelr<channel_name::A>(.75);
  // draw the text
  draw::text(lcd,text_rect,spoint16::zero(),text,text_font,text_scale,px2);
  // increment the hue
  hue+=.1;
  if(hue>1.0) {
    hue = 0.0;
  }
  if(millis()>=fps_ts) {
    fps_ts = millis()+1000;
    Serial.print("FPS: ");
    Serial.println(fps_count);
    fps_count = 0;
  }
}