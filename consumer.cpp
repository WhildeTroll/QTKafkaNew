#include "consumer.h"
#include <librdkafka/rdkafkacpp.h>

consumer::consumer()
{
    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    std::string errstr;
    conf->set("bootstrap.servers", "localhost:9092", errstr);
    conf->set("client.id", "my-client", errstr);
    RdKafka::Consumer *consumer = RdKafka::Consumer::create(conf, errstr);
}
