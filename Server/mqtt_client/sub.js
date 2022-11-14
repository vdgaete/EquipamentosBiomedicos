const mqtt = require('mqtt')
const client  = mqtt.connect([{ host: 'localhost', port: 1883 }])

client.on('connect', function () {
  client.subscribe('Devices/BO', function (err) {
    if (!err) {
      client.publish('presence', 'Hello mqtt')
    }
  })
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log("["+topic.toString()+"]:"+message.toString())
  client.end()
})