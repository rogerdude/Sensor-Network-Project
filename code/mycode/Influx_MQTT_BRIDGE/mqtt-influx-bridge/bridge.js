 // bridge.js
const mqtt = require('mqtt');
const { InfluxDB, Point } = require('@influxdata/influxdb-client');

const mqttClient = mqtt.connect('mqtt://localhost:1883');
const influx = new InfluxDB({ 
  url: 'http://172.20.10.4:8086', 
  token: '7onhN-UGxoD6qHWxs9XXXGoScZ7vCtSs7nlIv5Btg-KhEUnf_gdIZDYGiO-J1MG00s6LEjLssYGOuIDWIjYiCQ==' 
});
const writeApi = influx.getWriteApi('uq', 'project');

mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('sensors/env1');
});

mqttClient.on('message', (topic, message) => {
  console.log(`Received on ${topic}: ${message.toString()}`);
  try {
    const data = JSON.parse(message.toString());
    const point = new Point('environment')
      .tag('device', 'basenode')
      .floatField('temperature', data.temperature)
      .floatField('humidity', data.humidity);
    writeApi.writePoint(point);
  } catch (e) {
    console.error('Failed to parse message or write to InfluxDB:', e);
  }
});


//  // bridge.js
// const mqtt = require('mqtt');
// const { InfluxDB, Point } = require('@influxdata/influxdb-client');
// const options = {
//   host: 'ce9e85e72cce495f9b5ee973cb737f5e.s1.eu.hivemq.cloud',
//   port: 8883,
//   protocol: 'mqtts',
//   username: 'coolguy',
//   password: 'cCSSE4011',
// }


// const mqttClient = mqtt.connect(options);
// mqttClient.on('error', (err) => {
// console.error('MQTT connection error:', err.message);
// });
// const influx = new InfluxDB({ 
//   url: 'http://192.168.0.27:8086', 
//   token: '7onhN-UGxoD6qHWxs9XXXGoScZ7vCtSs7nlIv5Btg-KhEUnf_gdIZDYGiO-J1MG00s6LEjLssYGOuIDWIjYiCQ==' 
// });
// const writeApi = influx.getWriteApi('uq', 'project');

// mqttClient.on('connect', () => {
//   console.log('Connected to MQTT broker');
//   mqttClient.subscribe('sensors/env1');
// });

// mqttClient.on('message', (topic, message) => {
//   console.log(`Received on ${topic}: ${message.toString()}`);
//   try {
//     const data = JSON.parse(message.toString());
//     const point = new Point('environment')
//       .tag('device', 'basenode')
//       .floatField('temperature', data.temperature)
//       .floatField('humidity', data.humidity);
//     writeApi.writePoint(point);
//   } catch (e) {
//     console.error('Failed to parse message or write to InfluxDB:', e);
//   }
// });
