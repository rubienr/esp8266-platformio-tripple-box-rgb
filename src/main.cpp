#include <Arduino.h>
#include <Countdown.h>
#include <Display.h>
#include <KeyEventReceiver.h>
#include <Keyboard.h>
#include <OperatingMode.h>
#include <PixelRing.h>
#include <StatusBar.h>
#include <WebService.h>
#include <WifiConfigOrFallbackAccesspointManager.h>
#include <pinutils.h>

// TODO fix issue with 16 led angular view
// TODO fix issue with color mode R/G/B: only 1st 16 leds follow the scene, not all
using PixelRing_t = PixelRing<300, D3, NEO_GRB + NEO_KHZ800>;

//----------------------------------------------------------------------------------------------

class KeyEventHandler : public KeyEventReceiver
{
    PixelRing_t &strip;
    OperatingMode &operating_mode;
    Countdown &lights_off_timer;


public:
    explicit KeyEventHandler(PixelRing_t &pixel_ring, OperatingMode &operating_mode, Countdown &lights_off_timer)
    : strip(pixel_ring), operating_mode(operating_mode), lights_off_timer(lights_off_timer)
    {
    }

    bool take(KeyEvent e) override
    {
        Serial.print("KeyEventHandler::take: key=");
        Serial.print(std::underlying_type<KeyEvent::Key>::type(e.key));
        Serial.print(" type=");
        Serial.print(std::underlying_type<KeyEvent::Type>::type(e.type));
        Serial.print(" repeated=");
        Serial.println(e.repeated);

        bool consumed = false;


        // on double pressed
        if(!consumed && e.type == KeyEvent::Type::DoublePressed)
        {
            switch(e.key)
            {
            case KeyEvent::Key::Key1:
            case KeyEvent::Key::Key3:
            case KeyEvent::Key::Key4:
            case KeyEvent::Key::Key5:
            case KeyEvent::Key::Key6:
            case KeyEvent::Key::Key7:
                break;
            case KeyEvent::Key::Key0:
            case KeyEvent::Key::Key2:
            case KeyEvent::Key::Key8:
            case KeyEvent::Key::Key10:
                Serial.println("max brightness");
                strip.maxBrightness();
                consumed = true;
                break;
            case KeyEvent::Key::Key9:
            case KeyEvent::Key::Key11:
                //ESP.restart();
                break;
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        // on long or very long pressed
        if(!consumed && e.type == KeyEvent::Type::Repeated)
        {
            const uint8_t long_press = 6;
            //const uint8_t extra_long_press = 2 * long_press;

            switch(e.key)
            {
            case KeyEvent::Key::Key1:
            case KeyEvent::Key::Key3:
            case KeyEvent::Key::Key4:
            case KeyEvent::Key::Key5:
            case KeyEvent::Key::Key6:
            case KeyEvent::Key::Key7:
                break;
            case KeyEvent::Key::Key0:
            case KeyEvent::Key::Key2:
            case KeyEvent::Key::Key8:
            case KeyEvent::Key::Key10:
                if(e.repeated >= long_press)
                {
                    Serial.println("full width");
                    strip.fullWidth();
                    strip.on();
                    consumed = true;
                }
                break;
            case KeyEvent::Key::Key9:
            case KeyEvent::Key::Key11:
            case KeyEvent::Key::None:
            case KeyEvent::Key::LastEnumeration:
                break;
            }
        }

        if(!consumed && (e.type == KeyEvent::Type::Pressed || e.type == KeyEvent::Type::Repeated))
        {
            lights_off_timer.reset();

            switch(e.key)
            {
            case KeyEvent::Key::Key8:
                Serial.println("white");
                strip.on();
                strip.process(PixelRing_t::SceneMode::White);
                consumed = true;
                break;
            case KeyEvent::Key::Key0:
                Serial.println("red");
                strip.on();
                strip.process(PixelRing_t::SceneMode::Red);
                consumed = true;
                break;
            case KeyEvent::Key::Key10:
                Serial.println("green");
                strip.on();
                strip.process(PixelRing_t::SceneMode::Green);
                consumed = true;
                break;
            case KeyEvent::Key::Key2:
                Serial.println("blue");
                strip.process(PixelRing_t::SceneMode::Blue);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key3:
                Serial.println("++");
                strip.incrementBrightness(20);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key7:
                Serial.println("--");
                strip.incrementBrightness(-20);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key11:
                Serial.println("toggle on/off");
                if(!strip.toggleOnOff())
                {
                    lights_off_timer.enable();
                }
                consumed = true;
                break;
            case KeyEvent::Key::Key1:
                Serial.println("shift++");
                strip.shift(1);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key9:
                Serial.println("shift--");
                strip.shift(-1);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key4:
                Serial.println("width++");
                strip.incrementWidth(1);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key6:
                Serial.println("width--");
                strip.incrementWidth(-1);
                strip.on();
                consumed = true;
                break;
            case KeyEvent::Key::Key5:
                Serial.println("next scene");
                strip.nextScene();
                strip.on();
                break;
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
        }
    } __;

    // ---------------------------------------------------------------------------------------------

    Countdown lights_off_timer{ 60 * 60 * 6 };
    OperatingMode operating_mode;
    PixelRing_t strip;

    Display display;
    StatusBar status_bar{ operating_mode, display };

    WebService http_service{ 80 };

    Keyboard keyboard{25, 50};
    KeyEventHandler event_handler{strip, operating_mode, lights_off_timer};

    // ---------------------------------------------------------------------------------------------

    void setup()
    {
        strip.setup();

        lights_off_timer.disable();
        lights_off_timer.reset();

        display.setup();
        display.reset();
        status_bar.update();

        display.printf("keyboard ");
        if(keyboard.setup(&event_handler, 0x5B)) // short GND to 1 on the kbd. board
            display.printf("ok\n");
        else
            display.printf("err\n");


        WiFi.mode(WIFI_AP);
        WifiConfigOrFallbackAccesspointManager _(display);

        http_service.setup();

        lights_off_timer.enable();
    }

    // ---------------------------------------------------------------------------------------------

    void lightsOff()
    {
        Serial.println("Ressources::onTimeout");
        strip.off();
    }

    // ---------------------------------------------------------------------------------------------

    void process()
    {
        strip.process();
        status_bar.update();
        keyboard.process();
        http_service.process();
        lights_off_timer.process([this]() { this->lightsOff(); });
    }
} r;

// -------------------------------------------------------------------------------------------------

void setup() { r.setup(); }

// -------------------------------------------------------------------------------------------------

void loop() { r.process(); }
