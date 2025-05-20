// #include <zephyr/kernel.h>
// #include <zephyr/net/net_if.h>
// #include <zephyr/net/net_event.h>
// #include <zephyr/net/wifi_mgmt.h>
// #include <zephyr/net/socket.h>
// #include <zephyr/net/net_ip.h>
// #include <zephyr/net/mqtt.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/sys/printk.h>
//  #include <zephyr/net/mqtt.h>
// #include <zephyr/random/random.h>
// #include <string.h>
// #include <stdio.h>

// LOG_MODULE_REGISTER(influx_example, LOG_LEVEL_INF);

// #define SSID "Telstra1B216F"
// #define PSK "cs62spe9xx"

// #define INFLUXDB_HOST "192.168.0.30"
// #define INFLUXDB_PORT 8086
// #define INFLUXDB_PATH "/api/v2/write?org=uq&bucket=seminar&precision=s"
// #define INFLUXDB_TOKEN "7onhN-UGxoD6qHWxs9XXXGoScZ7vCtSs7nlIv5Btg-KhEUnf_gdIZDYGiO-J1MG00s6LEjLssYGOuIDWIjYiCQ=="

// #define MQTT_BROKER_ADDR "192.168.0.30"
// #define MQTT_BROKER_PORT 1883

// K_THREAD_STACK_DEFINE(mqtt_stack_area, 2048);
// static struct k_thread mqtt_thread_data;
// static K_SEM_DEFINE(wifi_connected, 0, 1);

// static struct net_mgmt_event_callback wifi_cb;

// static struct mqtt_client client;
// static struct sockaddr_storage broker;
// static uint8_t rx_buffer[256];
// static uint8_t tx_buffer[256];
// static uint8_t payload_buf[256];
 
// void mqtt_loop(void);
 
// static bool is_network_up(void)
// {
//     struct net_if *iface = net_if_get_default();
//     return net_if_is_up(iface) && net_if_ipv4_addr_lookup(NULL, &iface);
// }


// static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
// {
//     const struct wifi_status *status = cb->info;
//     if (status->status == 0) {
//         printk("Wi-Fi connected.\n");
//         k_sem_give(&wifi_connected);
//     } else {
//         printk("Wi-Fi connection failed: %d\n", status->status);
//     }
// }

// static void wifi_event_handler(struct net_mgmt_event_callback *cb,
//                                uint32_t mgmt_event, struct net_if *iface)
// {
//     if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
//         handle_wifi_connect_result(cb);
//     }
// }

// void wifi_connect(void)
// {
//     struct net_if *iface = net_if_get_default();

//     struct wifi_connect_req_params wifi_params = {
//         .ssid = SSID,
//         .ssid_length = strlen(SSID),
//         .psk = PSK,
//         .psk_length = strlen(PSK),
//         .channel = WIFI_CHANNEL_ANY,
//         .security = WIFI_SECURITY_TYPE_PSK,
//         .band = WIFI_FREQ_BAND_2_4_GHZ,
//         .mfp = WIFI_MFP_OPTIONAL
//     };

//     printk("Connecting to WiFi: %s\n", SSID);
//     int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
//                        &wifi_params, sizeof(wifi_params));

//     if (ret) {
//         printk("WiFi connect failed: %d\n", ret);
//     }
// }
 
// void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt)
// {
//     switch (evt->type) {
//     case MQTT_EVT_CONNACK:
//         if (evt->result == 0) {
//             printk("MQTT client connected!\n");
//         } else {
//             printk("MQTT connect failed (%d)\n", evt->result);
//         }
//         break;
//     case MQTT_EVT_DISCONNECT:
//         printk("MQTT client disconnected (%d)\n", evt->result);
//         break;
//     case MQTT_EVT_PUBACK:
//         printk("Message published successfully (ID: %u)\n", evt->param.puback.message_id);
//         break;
//     default:
//         break;
//     }
// } 
 
// void mqtt_connect_and_publish(void)
// {
// //  

 
 
//     memset(&broker, 0, sizeof(broker)); // Ensure broker is zeroed
//     struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
//     broker4->sin_family = AF_INET;
//     broker4->sin_port = htons(MQTT_BROKER_PORT);
//     net_addr_pton(AF_INET, "192.168.0.30", &broker4->sin_addr);

//     mqtt_client_init(&client);
//     client.broker = &broker;
//     client.evt_cb = mqtt_evt_handler;
//     static uint8_t client_id_buf[32];  

//     snprintf(client_id_buf, sizeof(client_id_buf), "basenode-%08x", sys_rand32_get());
//     client.client_id.utf8 = client_id_buf;
//     client.client_id.size = strlen(client_id_buf); 
//     client.protocol_version = MQTT_VERSION_3_1_1;

//     client.rx_buf = rx_buffer;
//     client.rx_buf_size = sizeof(rx_buffer);
//     client.tx_buf = tx_buffer;
//     client.tx_buf_size = sizeof(tx_buffer);
  
    

//     int rc = mqtt_connect(&client);
//     if (rc != 0) {
//         printk("mqtt_connect failed: %d\n", rc);
//         return;
//     }

//     // Prepare JSON payload
//     snprintf(payload_buf, sizeof(payload_buf),
//              "{\"temperature\":%.2f,\"humidity\":%.2f}", 23.1, 45.6);

//     struct mqtt_publish_param param = {
//         .message.topic.topic.utf8 = "sensors/env1",
//         .message.topic.topic.size = strlen("sensors/env1"),
//         .message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
//         .message.payload.data = payload_buf,
//         .message.payload.len = strlen(payload_buf),
//         .message_id = sys_rand32_get(),
//         .dup_flag = 0,
//         .retain_flag = 0,
//     };

//     mqtt_publish(&client, &param);

//     // Start MQTT loop thread
//     k_thread_create(&mqtt_thread_data, mqtt_stack_area, K_THREAD_STACK_SIZEOF(mqtt_stack_area),
//                     (k_thread_entry_t)mqtt_loop, NULL, NULL, NULL,
//                     7, 0, K_NO_WAIT);
// }
 

// void mqtt_loop(void)
// {
//     while (1) {
//         mqtt_input(&client);
//         mqtt_live(&client);
//         k_sleep(K_MSEC(100));
//     }
// }

// int main(void)
// {
//     // while (1) {
//     //     {
//     // printk("Hello from M5Stack Core2!\n");
//     // k_sleep(K_SECONDS(1));
//     //     }
//     // }
//     printk("Starting basenode\n");

//     net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
//                                  NET_EVENT_WIFI_CONNECT_RESULT);
//     net_mgmt_add_event_callback(&wifi_cb);

//     wifi_connect();
   
//     if (k_sem_take(&wifi_connected, K_SECONDS(10)) == 0) {
//         printk("Wi-Fi ready.\n");
     

//         mqtt_connect_and_publish(); 
//     } else {
//         printk("Wi-Fi connection timed out.\n");
    
// }
//     return 0;
// }

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(influx_example, LOG_LEVEL_INF);

// Wi-Fi credentials
#define SSID "jeremy phone"
#define PSK "Habib1234"

// MQTT broker config
#define MQTT_BROKER_ADDR "172.20.10.4"
#define MQTT_BROKER_PORT 1883

// Thread and buffers
K_THREAD_STACK_DEFINE(mqtt_stack_area, 2048);
static struct k_thread mqtt_thread_data;
static K_SEM_DEFINE(wifi_connected, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct mqtt_client client;
static struct sockaddr_storage broker;

static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];
static uint8_t payload_buf[256];
static bool mqtt_ready = false;

void mqtt_loop(void);

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = cb->info;
    if (status->status == 0) {
        printk("Wi-Fi connected.\n");
        k_sem_give(&wifi_connected);
    } else {
        printk("Wi-Fi connection failed: %d\n", status->status);
    }
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        handle_wifi_connect_result(cb);
    }
}

void wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_connect_req_params wifi_params = {
        .ssid = SSID,
        .ssid_length = strlen(SSID),
        .psk = PSK,
        .psk_length = strlen(PSK),
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_PSK,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp = WIFI_MFP_OPTIONAL
    };

    printk("Connecting to WiFi: %s\n", SSID);
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                       &wifi_params, sizeof(wifi_params));

    if (ret) {
        printk("WiFi connect failed: %d\n", ret);
    }
}

void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt)
{
    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result == 0) {
            printk("MQTT client connected!\n");
            mqtt_ready = true;
        } else {
            printk("MQTT connect failed (%d)\n", evt->result);
        }
        break;
    case MQTT_EVT_DISCONNECT:
        printk("MQTT client disconnected (%d)\n", evt->result);
        mqtt_ready = false;
        break;
    case MQTT_EVT_PUBACK:
        printk("Message published (ID: %u)\n", evt->param.puback.message_id);
        break;
    default:
        break;
    }
}

void mqtt_loop(void)
{
    while (1) {
        int rc = mqtt_input(&client);
        if (rc != 0) {
            printk("mqtt_input error: %d\n", rc);
        }

        rc = mqtt_live(&client);
        if (rc != 0) {
            printk("mqtt_live error: %d\n", rc);
        }

        k_sleep(K_MSEC(100));
    }
}

void mqtt_connect_and_publish(void)
{
    struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
    memset(broker4, 0, sizeof(struct sockaddr_in));

    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(MQTT_BROKER_PORT);
    int ret = net_addr_pton(AF_INET, MQTT_BROKER_ADDR, &broker4->sin_addr);
    if (ret < 0) {
        printk("Failed to parse broker IP address\n");
        return;
    }

    mqtt_client_init(&client);
    client.broker = &broker;
    client.evt_cb = mqtt_evt_handler;
    client.client_id.utf8 = "basenode";
    client.client_id.size = strlen(client.client_id.utf8);
    client.protocol_version = MQTT_VERSION_3_1_1;

    client.rx_buf = rx_buffer;
    client.rx_buf_size = sizeof(rx_buffer);
    client.tx_buf = tx_buffer;
    client.tx_buf_size = sizeof(tx_buffer);

    client.transport.type = MQTT_TRANSPORT_NON_SECURE;

    struct mqtt_sec_config *tls = &client.transport.tls.config;
    memset(tls, 0, sizeof(*tls));  
 

    int sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if (sock < 0) {
    printk("Socket creation failed: %d\n", errno);
} else {
    ret = zsock_connect(sock, (struct sockaddr *)broker4, sizeof(struct sockaddr_in));
    if (ret < 0) {
        printk("TCP connect test failed: %d (errno: %d)\n", ret, errno);
    } else {
       
        printk("TCP connect test succeeded!\n");
       

int fd = client.transport.tcp.sock;
zsock_close(fd);  
    }
}


    ret = mqtt_connect(&client);
    if (ret != 0) {
        printk("mqtt_connect failed: %d\n", ret);
        return;
    }

    printk("Connected to MQTT broker, publishing message...\n");

    snprintf(payload_buf, sizeof(payload_buf),
             "{\"temperature\":%.2f,\"humidity\":%.2f}", 23.1, 45.6);

    struct mqtt_publish_param param = {
        .message.topic.topic.utf8 = "sensors/env1",
        .message.topic.topic.size = strlen("sensors/env1"),
        .message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
        .message.payload.data = payload_buf,
        .message.payload.len = strlen(payload_buf),
        .message_id = sys_rand32_get(),
        .dup_flag = 0,
        .retain_flag = 0,
    };

    ret = mqtt_publish(&client, &param);
    if (ret) {
        printk("Failed to publish message: %d\n", ret);
    }

    // Start MQTT thread to keep connection alive
    k_thread_create(&mqtt_thread_data, mqtt_stack_area, K_THREAD_STACK_SIZEOF(mqtt_stack_area),
                    (k_thread_entry_t)mqtt_loop, NULL, NULL, NULL,
                    7, 0, K_NO_WAIT);
}


int main(void)
{
    printk("Starting BaseNode MQTT example\n");

    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    wifi_connect();

    if (k_sem_take(&wifi_connected, K_SECONDS(10)) == 0) {
        printk("Wi-Fi ready.\n");
        mqtt_connect_and_publish();
    } else {
        printk("Wi-Fi connection timed out.\n");
    }

    return 0;
}
