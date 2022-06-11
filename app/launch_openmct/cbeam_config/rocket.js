var app = require('../../cbeam-telemetry-server/server/app');

var void_av = new app.Dictionary("Void AV Bay", 'void_av');
void_av.addMeasurement('altitude', 'av_altitude', [
    {
        units: 'meters',
        format: 'float',
        min: 0,
        max: 35000
    }
]);

void_av.addMeasurement('latitude', 'av_latitude', [
    {
        units: 'degrees',
        format: 'float',
        min: 90,
        max: 90
    }
]);

void_av.addMeasurement('longitude', 'av_longitude', [
    {
        units: 'degrees',
        format: 'float',
        min: -180,
        max: 180
    }
]);

var void_payload = new app.Dictionary("Void Payload", 'void_payload');
void_payload.addMeasurement('altitude', 'payload_altitude', [
    {
        units: 'meters',
        format: 'float',
        min: 0,
        max: 35000
    }
]);

void_payload.addMeasurement('latitude', 'payload_latitude', [
    {
        units: 'degrees',
        format: 'float',
        min: 90,
        max: 90
    }
]);

void_payload.addMeasurement('longitude', 'payload_longitude', [
    {
        units: 'degrees',
        format: 'float',
        min: -180,
        max: 180
    }
]);

var server = new app.Server({
    host: process.env.HOST || 'localhost',
    port: process.env.PORT || 8080,
    wss_port: process.env.WSS_PORT || 8082,
    broker: process.env.MSGFLO_BROKER || 'mqtt://localhost',
    dictionaries: [void_av, void_payload],
    history: {
        host: process.env.INFLUX_HOST || 'localhost',
        db: process.env.INFLUX_DB || 'void'
    }
});

server.start(function (err) {
    if (err) {
        console.error(err);
        process.exit(1);
    }
    console.log('Server listening in ' + server.config.port);
});
