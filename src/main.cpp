#include <Arduino.h>
#include <Countdown.h>
#include <DallasTemperature.h>
#include <Display.h>
#include <KeyEventReceiver.h>
#include <Keyboard.h>
#include <OneWire.h>
#include <OperatingMode.h>
#include <PixelRing.h>
#include <StatusBar.h>
#include <WebService.h>
#include <WifiConfigOrFallbackAccesspointManager.h>
#include <pinutils.h>

//----------------------------------------------------------------------------------------------

using PixelRing_t = PixelRing<300, D3, NEO_GRB + NEO_KHZ800>;

//----------------------------------------------------------------------------------------------

class KeyEventHandler : public KeyEventReceiver
{
    PixelRing_t &strip;
    OperatingMode &operating_mode;

public:
    // ---------------------------------------------------------------------------------------------

    explicit KeyEventHandler(PixelRing_t &pixel_ring, OperatingMode &operating_mode)
    : strip(pixel_ring), operating_mode(operating_mode)
    {
    }

    // ---------------------------------------------------------------------------------------------

    bool take(KeyEvent e) override
    {
        /*Serial.print("KeyEventHandler::take: key=");
        Serial.print(std::underlying_type<KeyEvent::Key>::type(e.key));
        Serial.print(" type=");
        Serial.print(std::underlying_type<KeyEvent::Type>::type(e.type));
        Serial.print(" repeated=");
        Serial.println(e.repeated);*/

        bool consumed = false;
        const uint16_t long_press = 10;
        // const uint16_t extra_long_press = 5 * long_press;

        // on double stroke (double click alike)
        if(!consumed && e.type == KeyEvent::Type::DoublePressed)
        {
            switch(e.key)
            {
            case KeyEvent::Key::Key0:
            case KeyEvent::Key::Key2:
            case KeyEvent::Key::Key8:
            case KeyEvent::Key::Key10:
                strip.maxBrightness();
                consumed = true;
                break;
            case KeyEvent::Key::Key5:
                strip.nextScene();
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key3:
                strip.incrementBrightness(3);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key7:
                strip.incrementBrightness(-3);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key11:
                strip.toggleOnOff();
                consumed = true;
                break;
            case KeyEvent::Key::Key1:
            case KeyEvent::Key::Key4:
            case KeyEvent::Key::Key6:
            case KeyEvent::Key::Key9:
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        // on long or very long pressed
        if(!consumed && e.type == KeyEvent::Type::Repeated)
        {
            switch(e.key)
            {
            case KeyEvent::Key::Key5:
                if(e.repeated >= long_press)
                {
                    strip.process(PixelRing_t::SceneMode::Rainbow);
                    strip.on();
                    consumed = true;
                }
                break;
            case KeyEvent::Key::Key0:
            case KeyEvent::Key::Key2:
            case KeyEvent::Key::Key8:
            case KeyEvent::Key::Key10:
                if(e.repeated >= long_press)
                {
                    strip.fullWidth();
                    strip.on();
                    consumed = true;
                }
                break;
            case KeyEvent::Key::Key11:
            case KeyEvent::Key::Key1:
            case KeyEvent::Key::Key3:
            case KeyEvent::Key::Key4:
            case KeyEvent::Key::Key6:
            case KeyEvent::Key::Key7:
            case KeyEvent::Key::Key9:
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        // on single keystroke
        if(!consumed && e.type == KeyEvent::Type::Pressed)
        {
            switch(e.key)
            {
            case KeyEvent::Key::Key0:
                strip.process(PixelRing_t::SceneMode::White);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key2:
                strip.process(PixelRing_t::SceneMode::Red);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key8:
                strip.process(PixelRing_t::SceneMode::Green);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key10:
                strip.process(PixelRing_t::SceneMode::Blue);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key5:
                strip.nextScene();
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key11:
                strip.toggleOnOff();
                consumed = true;
                break;
            case KeyEvent::Key::Key1:
            case KeyEvent::Key::Key3:
            case KeyEvent::Key::Key7:
            case KeyEvent::Key::Key9:
            case KeyEvent::Key::Key6:
            case KeyEvent::Key::Key4:
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        // on pressed or repeated
        if(!consumed && (e.type == KeyEvent::Type::Pressed || e.type == KeyEvent::Type::Repeated))
        {
            switch(e.key)
            {
            case KeyEvent::Key::Key3:
                strip.incrementBrightness(3);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key7:
                strip.incrementBrightness(-3);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key1:
                strip.incrementWidth(4);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key9:
                strip.incrementWidth(-4);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key6:
                strip.shift(-4);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key4:
                strip.shift(4);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key0:
            case KeyEvent::Key::Key2:
            case KeyEvent::Key::Key5:
            case KeyEvent::Key::Key8:
            case KeyEvent::Key::Key10:
            case KeyEvent::Key::Key11:
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        return consumed;
    }
};

// -------------------------------------------------------------------------------------------------

struct Ressources
{
    // ---------------------------------------------------------------------------------------------

    struct EarlyInit
    {
        EarlyInit()
        {
            Serial.begin(230400);
            while(!Serial)
                delay(10);
            Serial.print("\n\n\n");
            Serial.println("Setup::setup");

            wifi_status_led_uninstall();
            pinMode(LED_BUILTIN, OUTPUT);
            digitalWrite(LED_BUILTIN, HIGH);
        }
    } __;

    // ---------------------------------------------------------------------------------------------

    Countdown lights_off_timer{ 60 * 60 * 6 };
    Countdown display_off_timer{ 60 * 30 };
    Countdown temperature_refresh_timer{ 30 };

    OperatingMode operating_mode{OperatingMode::Mode::Wifi};
    PixelRing_t strip;

    Display display;
    StatusBar status_bar{ operating_mode, display };

    WebService http_service{ 80 };

    Keyboard keyboard{ 5, 250 };
    KeyEventHandler event_handler{ strip, operating_mode };

    Countdown::Callback_t on_display_off = [this]() { this->displayOff(); };
    Countdown::Callback_t on_lights_off = [this]() { this->lightsOff(); };
    Countdown::Callback_t on_temperature_refresh = [this]() {
        measureTemperature();
        status_bar.data.temperature_celsius = getTemperature();
        temperature_refresh_timer.reset();
    };

    OneWire ow{ D4 };
    DallasTemperature temp_sensors{ &ow };
    uint8_t temperature_sensor_address{ 0xff };

    // ---------------------------------------------------------------------------------------------

    void setup()
    {
        strip.setup();

        display_off_timer.disable();
        display_off_timer.reset();
        lights_off_timer.disable();
        lights_off_timer.reset();
        temperature_refresh_timer.disable();
        temperature_refresh_timer.reset();

        display.setup();
        display.dim(false);
        display.reset();

        display.printf("keyboard ");
        if(keyboard.setup(&event_handler, 0x5B)) // short GND to 1 on the kbd. board
            display.printf("ok\n");
        else
            display.printf("err\n");


        WiFi.mode(WIFI_AP);
        WifiConfigOrFallbackAccesspointManager _(display);

        http_service.setup();

        lights_off_timer.enable();
        display_off_timer.enable();
        temperature_refresh_timer.enable();

        temp_sensors.begin();
        temp_sensors.getAddress(&temperature_sensor_address, 0);
        status_bar.data.enable_temperature = true;
        measureTemperature();
        status_bar.data.temperature_celsius = getTemperature();
        status_bar.update();
    }

    // ---------------------------------------------------------------------------------------------

    void measureTemperature()
    {
        temp_sensors.requestTemperaturesByAddress(&temperature_sensor_address);
        digitalWrite(LED_BUILTIN, HIGH);
    }

    // ---------------------------------------------------------------------------------------------

    float getTemperature()
    {
        float temperature = temp_sensors.getTempC(&temperature_sensor_address);
        digitalWrite(LED_BUILTIN, HIGH);
        return temperature;
    }

    // ---------------------------------------------------------------------------------------------

    void displayOff()
    {
        Serial.println("Ressources::displayOff");
        ;
        display.off();
        display_off_timer.reset();
    }

    // ---------------------------------------------------------------------------------------------

    void displayOn()
    {
        Serial.println("Ressources::displayOn");
        display.on();
        display_off_timer.reset();
    }

    // ---------------------------------------------------------------------------------------------

    void lightsOff()
    {
        Serial.println("Ressources::lightsOff");
        strip.off();
        lights_off_timer.reset();
    }

    // ---------------------------------------------------------------------------------------------

    void process()
    {
        strip.process();
        status_bar.update();
        if(keyboard.process())
        {
            displayOn();
            display_off_timer.reset();
            lights_off_timer.reset();
        }
        http_service.process();

        display_off_timer.process(on_display_off);
        lights_off_timer.process(on_lights_off);
        temperature_refresh_timer.process(on_temperature_refresh);
    }
} r;

// -------------------------------------------------------------------------------------------------

void setup() { r.setup(); }

// -------------------------------------------------------------------------------------------------

void loop() { r.process(); }
