#include "publisher.h"
#include <librdkafka/rdkafkacpp.h>
Publisher::Publisher()
{
    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    std::string errstr;
    conf->set("bootstrap.servers", "localhost:9092", errstr);
    conf->set("client.id", "my-client", errstr);
    RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
}
