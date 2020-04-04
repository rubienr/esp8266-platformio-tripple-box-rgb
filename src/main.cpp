#include <Arduino.h>
#include <Display.h>
#include <WifiConfigOrFallbackAccesspointManager.h>
#include <WebService.h>

//--------------------------------------------------------------------------------------------------
struct Ressources
{

    //----------------------------------------------------------------------------------------------

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

    //----------------------------------------------------------------------------------------------

    Display display;
    WebService http_service{80};

    //----------------------------------------------------------------------------------------------

    void setup()
    {
        display.setup();

        WiFi.mode(WIFI_AP);
        WifiConfigOrFallbackAccesspointManager _(display);

        http_service.setup();
    }

    //----------------------------------------------------------------------------------------------

    void process() {
        http_service.process();
    }
} r;

//----------------------------------------------------------------------------------------------

void setup() { r.setup(); }

//----------------------------------------------------------------------------------------------

void loop() { r.process(); }
