#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "librdkafka/rdkafka.h"


static volatile sig_atomic_t run = 1;

static void stop(int sig) {
    run = 0;
    fclose(stdin);
}
// функция доставлено/недоставлено
static void dr_msg_cb(rd_kafka_t* rk, const rd_kafka_message_t* rkmessage, void* opaque) {
    if (rkmessage->err)
        fprintf(stderr, "%% Message delivery failed: %s\n",
            rd_kafka_err2str(rkmessage->err));
    else
        fprintf(stderr,
            "%% Message delivered (%zd bytes, "
            "partition %" PRId32 ")\n",
            rkmessage->len, rkmessage->partition);
    
}


int main(int argc, char** argv) {
    rd_kafka_t* rk;        
    rd_kafka_conf_t* conf; 
    char errstr[512];      
    char buf[512];         
    const char* brokers;   
    const char* topic;  
    if (argc != 3) {
        fprintf(stderr, "%% Usage: %s <broker> <topic>\n", argv[0]);
        return 1;
    }

    brokers = argv[1];
    topic = argv[2];


    /*
     * Create Kafka client configuration place-holder
     */
    conf = rd_kafka_conf_new();

    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr,
        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", errstr);
        return 1;
    }
    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

    rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!rk) {
        fprintf(stderr, "%% Failed to create new producer: %s\n",
            errstr);
        return 1;
    }

    signal(SIGINT, stop);

    fprintf(stderr,
        "%% Type some text and hit enter to produce message\n"
        "%% Or just hit enter to only serve delivery reports\n"
        "%% Press Ctrl-C or Ctrl-D to exit\n");

    while (run && fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        rd_kafka_resp_err_t err;

        if (buf[len - 1] == '\n') 
            buf[--len] = '\0';

        if (len == 0) {
            rd_kafka_poll(rk, 0 /*non-blocking */);
            continue;
        }
    retry:
        err = rd_kafka_producev(
            rk,
            RD_KAFKA_V_TOPIC(topic),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_VALUE(buf, len),
            RD_KAFKA_V_OPAQUE(NULL),
            RD_KAFKA_V_END);

        if (err) {
            fprintf(stderr,
                "%% Failed to produce to topic %s: %s\n", topic,
                rd_kafka_err2str(err));

            if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                rd_kafka_poll(rk,
                    1000);
                goto retry;
            }
        }
        else {
            fprintf(stderr,
                "%% Enqueued message (%zd bytes) "
                "for topic %s\n",
                len, topic);
        }
        rd_kafka_poll(rk, 0);
    }


    fprintf(stderr, "%% Flushing final messages..\n");
    rd_kafka_flush(rk, 10 * 1000 );

    
    if (rd_kafka_outq_len(rk) > 0)
        fprintf(stderr, "%% %d message(s) were not delivered\n",
            rd_kafka_outq_len(rk));

    rd_kafka_destroy(rk);

    return 0;
}