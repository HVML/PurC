#ifndef LOGGING_h
#define LOGGING_h

#ifndef LOG_CHANNEL_PREFIX
#define LOG_CHANNEL_PREFIX Log
#endif // LOG_CHANNEL_PREFIX


#define PURCFETCHER_LOG_CHANNELS(M) \
     M(SQLDatabase) \
     M(NetworkCache) \
     M(NetworkCacheStorage) \
     M(Network) \
     M(NotYetImplemented) \
     M(MessagePorts) \

#undef DECLARE_LOG_CHANNEL
#define DECLARE_LOG_CHANNEL(name) \
    PURCFETCHER_EXPORT extern \
    WTFLogChannel JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, name);

PURCFETCHER_LOG_CHANNELS(DECLARE_LOG_CHANNEL)


#endif // LOGGING_h
