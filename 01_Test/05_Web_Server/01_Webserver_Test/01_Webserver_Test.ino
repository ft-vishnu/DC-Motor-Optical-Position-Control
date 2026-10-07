// OPQC
// Web Server Test 1
// ESP32-S3 Web Interface
// UI + Virtual Rotary Dial

#include <WiFi.h>
#include <WebServer.h>

const char* SSID = "OPQC_Motor";
const char* PASSWORD = "12345678";

WebServer server(80);

const char MAIN_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>OPQC Motor Control</title>

<style>

* {
    box-sizing: border-box;
}

body {
    margin: 0;
    font-family: Arial, Helvetica, sans-serif;
    background: #0b1120;
    color: #e5e7eb;
}

.container {
    width: 100%;
    max-width: 900px;
    margin: auto;
    padding: 24px;
}

/* Header */

.header {
    margin-bottom: 24px;
}

.header h1 {
    margin: 0;
    font-size: 30px;
    font-weight: 600;
    letter-spacing: 1px;
}

.header p {
    margin-top: 6px;
    color: #94a3b8;
    font-size: 14px;
}

/* Main grid */

.dashboard {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
}

.card {
    background: #111827;
    border: 1px solid #1e293b;
    border-radius: 16px;
    padding: 22px;
}

/* Position */

.position-card {
    text-align: center;
}

.label {
    color: #94a3b8;
    font-size: 13px;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.angle-value {
    margin-top: 8px;
    font-size: 42px;
    font-weight: 600;
}

.count-value {
    margin-top: 6px;
    color: #94a3b8;
    font-size: 16px;
}

/* Virtual dial */

.dial-container {
    display: flex;
    justify-content: center;
    margin-top: 20px;
}

.dial {
    position: relative;

    width: 280px;
    height: 280px;

    border-radius: 50%;

    background:
        radial-gradient(
            circle,
            #1e293b 0%,
            #111827 62%,
            #0f172a 63%,
            #0f172a 100%
        );

    border: 8px solid #334155;

    box-shadow:
        inset 0 0 30px rgba(0,0,0,0.5),
        0 10px 35px rgba(0,0,0,0.35);
}

/* Dial marks */

.mark {
    position: absolute;

    width: 2px;
    height: 10px;

    background: #64748b;

    left: 50%;
    top: 8px;

    transform-origin: 50% 132px;
}

.mark.major {
    width: 3px;
    height: 16px;

    background: #cbd5e1;
}

/* Degree labels */

.degree {
    position: absolute;

    font-size: 12px;
    color: #94a3b8;
}

.degree.zero {
    top: 22px;
    left: 50%;

    transform: translateX(-50%);
}

.degree.ninety {
    right: 22px;
    top: 50%;

    transform: translateY(-50%);
}

.degree.oneeighty {
    bottom: 22px;
    left: 50%;

    transform: translateX(-50%);
}

.degree.twoseventy {
    left: 18px;
    top: 50%;

    transform: translateY(-50%);
}

/* Dial pointer */

.pointer {
    position: absolute;

    width: 4px;
    height: 105px;

    left: 50%;
    bottom: 50%;

    transform-origin: bottom center;

    transform:
        translateX(-50%)
        rotate(0deg);

    background: #38bdf8;

    border-radius: 4px;

    box-shadow:
        0 0 12px rgba(56,189,248,0.7);

    transition:
        transform 0.15s linear;
}

.dial-center {
    position: absolute;

    width: 28px;
    height: 28px;

    left: 50%;
    top: 50%;

    transform:
        translate(-50%, -50%);

    background: #e2e8f0;

    border-radius: 50%;

    border: 5px solid #0f172a;

    z-index: 5;
}

/* Command */

.command-card h2 {
    margin-top: 0;

    font-size: 18px;
    font-weight: 500;
}

.input-row {
    display: flex;

    gap: 10px;

    margin-top: 15px;
}

input {
    flex: 1;

    min-width: 0;

    padding: 13px;

    border-radius: 8px;

    border: 1px solid #334155;

    background: #0f172a;

    color: #f8fafc;

    font-size: 17px;

    text-align: center;

    outline: none;
}

input:focus {
    border-color: #38bdf8;
}

/* Buttons */

button {
    border: none;

    border-radius: 8px;

    padding: 12px 15px;

    font-size: 14px;

    font-weight: 500;

    cursor: pointer;

    transition:
        transform 0.1s,
        opacity 0.1s;
}

button:active {
    transform: scale(0.96);
}

/* Rotate */

.primary {
    background: #0284c7;
    color: white;
}

.primary:hover {
    background: #0369a1;
}

/* Quick angles */

.quick-grid {
    display: grid;

    grid-template-columns:
        repeat(3, 1fr);

    gap: 8px;

    margin-top: 16px;
}

.quick {
    background: #1e293b;

    color: #e2e8f0;
}

.quick:hover {
    background: #334155;
}

/* Stop */

.stop {
    width: 100%;

    margin-top: 18px;

    background: #dc2626;

    color: white;
}

.stop:hover {
    background: #b91c1c;
}

/* Home */

.home {
    width: 100%;

    margin-top: 10px;

    background: #0f766e;

    color: white;
}

.home:hover {
    background: #115e59;
}

/* Status */

.status-card {
    grid-column: 1 / -1;

    display: flex;

    justify-content: space-between;

    align-items: center;
}

.status-name {
    color: #94a3b8;

    font-size: 13px;

    text-transform: uppercase;

    letter-spacing: 1px;
}

.status-value {
    color: #22c55e;

    font-size: 15px;

    font-weight: 600;
}

/* Mobile */

@media (max-width: 700px) {

    .dashboard {
        grid-template-columns: 1fr;
    }

    .status-card {
        grid-column: auto;
    }

    .dial {
        width: 250px;
        height: 250px;
    }

    .quick-grid {
        grid-template-columns:
            repeat(2, 1fr);
    }

}

</style>

</head>


<body>

<div class="container">

    <!-- HEADER -->

    <div class="header">

        <h1>
            OPQC Motor Control
        </h1>

        <p>
            Optical Position Control
        </p>

    </div>


    <div class="dashboard">


        <!-- POSITION CARD -->

        <div class="card position-card">

            <div class="label">
                Current Position
            </div>


            <div class="angle-value">

                <span id="angle">
                    0.0
                </span>

                <span>
                    &deg;
                </span>

            </div>


            <div class="count-value">

                Encoder Count:
                <span id="count">
                    0
                </span>

            </div>


            <!-- VIRTUAL DIAL -->

            <div class="dial-container">

                <div class="dial">


                    <div class="degree zero">
                        0&deg;
                    </div>


                    <div class="degree ninety">
                        90&deg;
                    </div>


                    <div class="degree oneeighty">
                        180&deg;
                    </div>


                    <div class="degree twoseventy">
                        270&deg;
                    </div>


                    <div
                        class="pointer"
                        id="pointer">
                    </div>


                    <div class="dial-center">
                    </div>


                </div>

            </div>

        </div>


        <!-- COMMAND CARD -->

        <div class="card command-card">


            <h2>
                Position Command
            </h2>


            <div class="input-row">

                <input
                    type="number"
                    id="angleInput"
                    step="18"
                    placeholder="Angle">


                <button
                    class="primary"
                    onclick="rotate()">

                    ROTATE

                </button>

            </div>


            <h2 style="margin-top:28px;">

                Quick Angles

            </h2>


            <div class="quick-grid">


                <button
                    class="quick"
                    onclick="move(-180)">

                    -180&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(-90)">

                    -90&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(-54)">

                    -54&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(-36)">

                    -36&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(-18)">

                    -18&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(18)">

                    +18&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(36)">

                    +36&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(54)">

                    +54&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(90)">

                    +90&deg;

                </button>


                <button
                    class="quick"
                    onclick="move(180)">

                    +180&deg;

                </button>


            </div>


            <!-- STOP -->

            <button
                class="stop"
                onclick="stopMotor()">

                STOP

            </button>


            <!-- HOME -->

            <button
                class="home"
                onclick="homeMotor()">

                HOME

            </button>


        </div>


        <!-- STATUS -->

        <div class="card status-card">


            <div class="status-name">

                System Status

            </div>


            <div
                class="status-value"
                id="status">

                READY

            </div>


        </div>


    </div>

</div>


<script>


function rotate()
{
    let angle =
        document
        .getElementById("angleInput")
        .value;

    if (angle === "")
        return;

    fetch(
        "/move?angle=" + angle
    );
}


function move(angle)
{
    fetch(
        "/move?angle=" + angle
    );
}


function stopMotor()
{
    fetch("/stop");
}


function homeMotor()
{
    fetch("/home");
}


function updateDial(angle)
{
    let pointer =
        document.getElementById(
            "pointer"
        );

    pointer.style.transform =
        "translateX(-50%) rotate(" +
        angle +
        "deg)";
}


function updateStatus()
{
    fetch("/status")

    .then(
        response =>
            response.json()
    )

    .then(
        data =>
        {
            let angle =
                data.angle;


            document
                .getElementById("count")
                .innerText =
                data.count;


            document
                .getElementById("angle")
                .innerText =
                angle.toFixed(1);


            document
                .getElementById("status")
                .innerText =
                data.status;


            updateDial(angle);
        }
    );
}


setInterval(
    updateStatus,
    100
);


updateStatus();


</script>

</body>

</html>

)rawliteral";


void handleRoot()
{
    server.send(
        200,
        "text/html",
        MAIN_PAGE
    );
}


void handleStatus()
{
    String json = "{";

    json += "\"count\":0,";
    json += "\"angle\":0,";
    json += "\"status\":\"READY\"";

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}


void handleMove()
{
    server.send(
        200,
        "text/plain",
        "Move command received"
    );
}


void handleStop()
{
    server.send(
        200,
        "text/plain",
        "Stop command received"
    );
}


void handleHome()
{
    server.send(
        200,
        "text/plain",
        "Home command received"
    );
}


void setup()
{
    Serial.begin(115200);

    delay(1000);


    Serial.println();

    Serial.println("==============================");

    Serial.println(
        "OPQC - Web Server Test 1"
    );

    Serial.println("==============================");


    WiFi.mode(WIFI_AP);


    WiFi.softAP(
        SSID,
        PASSWORD
    );


    Serial.println();


    Serial.print(
        "Wi-Fi SSID: "
    );

    Serial.println(
        SSID
    );


    Serial.print(
        "IP Address: "
    );

    Serial.println(
        WiFi.softAPIP()
    );


    server.on(
        "/",
        handleRoot
    );


    server.on(
        "/status",
        handleStatus
    );


    server.on(
        "/move",
        handleMove
    );


    server.on(
        "/stop",
        handleStop
    );


    server.on(
        "/home",
        handleHome
    );


    server.begin();


    Serial.println();

    Serial.println(
        "Web server started."
    );
}


void loop()
{
    server.handleClient();
}